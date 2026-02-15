import initModule from './libresdet.mjs'
const Module = await initModule();

const resdet_error_str = Module.cwrap('resdet_error_str', 'string', ['number']);
const resdet_methods = Module.cwrap('resdet_methods', 'number', []);
const resdet_get_method = Module.cwrap('resdet_get_method', 'number', ['string']);
const resdet_alloc_default_parameters = Module.cwrap('resdet_alloc_default_parameters', 'number', ['string']);
const resdet_parameters_set_range = Module.cwrap('resdet_parameters_set_range', 'number', ['number', 'number']);
const resdet_parameters_set_threshold = Module.cwrap('resdet_parameters_set_threshold', 'number', ['number', 'number']);
const resdet_parameters_set_compression_filter = Module.cwrap('resdet_parameters_set_compression_filter', 'number', ['number', 'number']);
const resdetect = Module.cwrap('resdetect', 'number', ['number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']);
const resdet_create_analysis = Module.cwrap('resdet_create_analysis', 'number', ['number', 'number', 'number', 'number','number']);
const resdet_analyze_image = Module.cwrap('resdet_analyze_image', 'number', ['number', 'number']);
const resdet_analysis_results = Module.cwrap('resdet_analysis_results', 'number', ['number', 'number', 'number', 'number', 'number']);
const resdet_destroy_analysis = Module.cwrap('resdet_destroy_analysis', undefined, ['number']);

const libVersion = Module.cwrap('resdet_libversion', 'string', []);
const defaultRange = Module.cwrap('resdet_default_range', 'number', []);

class OutOfMemoryError extends Error {
	constructor() {
		super("Out of memory.");
	}
}

class RDError extends Error {
	rderror: number

	constructor(rderror: number) {
		super(resdet_error_str(rderror));
		this.rderror = rderror;
	}
}

class Method {
	name: string
	threshold: number
	rdmethod: number

	constructor(name: string, threshold: number, rdmethod: number) {
		this.name = name;
		this.threshold = threshold;
		this.rdmethod = rdmethod;
	}

	static fromRDMethod(rdmethod: number) {
		return new Method(
			Module.UTF8ToString(Module.getValue(rdmethod,'i8*')),
			Module.getValue(rdmethod+8,'float'),
			rdmethod
		)
	}

	static get(name: string) {
		const rdmethod = resdet_get_method(name)
		if(!rdmethod)
			return undefined;
		return Method.fromRDMethod(rdmethod)
	}
}

class Resolution {
	index: number
	confidence: number

	constructor(index: number, confidence: number) {
		this.index = index;
		this.confidence = confidence;
	}

	static fromRDResolution(rdresolution: number) {
		return new Resolution(
			Module.getValue(rdresolution,'i32'),
			Module.getValue(rdresolution+4,'float')
		)
	}
}

interface Parameters {
	threshold: number;
	range: number;
	compression_filter: number;
};

interface Resolutions {
	widths: Resolution[];
	heights: Resolution[];
}

const analysisFinalizationRegistry = new FinalizationRegistry((rdanalysis: number) => {
	resdet_destroy_analysis(rdanalysis);
})

class Analysis {
	rdanalysis: number

	constructor(width: number, height: number, method: Method | null = null, parameters: Partial<Parameters> = {}) {
		const params = parametersFromObj(parameters)
		const errp = Module._malloc(4);
		if(!errp)
			throw new OutOfMemoryError();

		this.rdanalysis = resdet_create_analysis(method?.rdmethod, width, height, params, errp);

		Module._free(params);
		const err = Module.getValue(errp,'i32');
		Module._free(errp);
		if(err)
			throw new RDError(err);

		analysisFinalizationRegistry.register(this,this.rdanalysis)
	}

	analyzeImage(float32Array: Float32Array) {
		const buf = Module._malloc(float32Array.length*4);
		if(!buf)
			throw new OutOfMemoryError();
		Module.HEAPF32.set(float32Array,buf/4);

		const err = resdet_analyze_image(this.rdanalysis, buf);

		Module._free(buf);
		if(err)
			throw new RDError(err);
	}

	analysisResults(): Resolutions {
		const countwp = Module._malloc(4);
		const counthp = Module._malloc(4);
		const reswp = Module._malloc(4);
		const reshp = Module._malloc(4);
		if(!(countwp && counthp && reswp && reshp))
			throw new OutOfMemoryError();

		const err = resdet_analysis_results(this.rdanalysis,reswp,countwp,reshp,counthp);

		if(err) {
			Module._free(reswp);
			Module._free(reshp);
			Module._free(countwp);
			Module._free(counthp);
			throw new RDError(err);
		}

		const countw = Module.getValue(countwp,'i32');
		const counth = Module.getValue(counthp,'i32');
		const reswval = Module.getValue(reswp,'*');
		const reshval = Module.getValue(reshp,'*');

		const resw = Array.from({ length: countw }, (_,i) => Resolution.fromRDResolution(reswval+i*8))
		const resh = Array.from({ length: counth }, (_,i) => Resolution.fromRDResolution(reshval+i*8))

		Module._free(reswval);
		Module._free(reshval);
		Module._free(reswp);
		Module._free(reshp);
		Module._free(countwp);
		Module._free(counthp);

		return { 'widths': resw, 'heights': resh };
	}
}

function methods(): Method[] {
	const methodArray = [];
	for(let rdmethod = resdet_methods(); Module.getValue(rdmethod,'*'); rdmethod += 12)
		methodArray.push(Method.fromRDMethod(rdmethod));
	return methodArray;
}


function parametersFromObj(parameters: Partial<Parameters>) {
	if(Object.keys(parameters).length === 0)
		return null;

	const unrecognized = Object.keys(parameters).filter(key => !['range', 'threshold', 'compression_filter'].includes(key));
	if(unrecognized.length)
		throw new Error(`Unrecognized parameters: ${unrecognized}`);

	const params = resdet_alloc_default_parameters();
	if(!params)
		throw new OutOfMemoryError();

	let err;
	if('range' in parameters)
		err = resdet_parameters_set_range(params,parameters['range']);
	if('threshold' in parameters)
		err = resdet_parameters_set_threshold(params,parameters['threshold']);
	if('compression_filter' in parameters)
		err = resdet_parameters_set_compression_filter(params,parameters['compression_filter']);
	if(err)
		throw new RDError(err);

	return params;
}

function resDetect(float32Array: Float32Array, nimages: number, width: number, height: number, method: Method | null = null, parameters: Partial<Parameters> = {}) {
	const params = parametersFromObj(parameters)

	const buf = Module._malloc(float32Array.length*4);
	if(!buf)
		throw new OutOfMemoryError();
	Module.HEAPF32.set(float32Array,buf/4);

	const countwp = Module._malloc(4);
	const counthp = Module._malloc(4);
	const reswp = Module._malloc(4);
	const reshp = Module._malloc(4);
	if(!(countwp && counthp && reswp && reshp))
		throw new OutOfMemoryError();

	const err = resdetect(buf, nimages, width, height, reswp, countwp, reshp, counthp, method?.rdmethod, params);
	Module._free(params);
	Module._free(buf);
	if(err) {
		Module._free(reswp);
		Module._free(reshp);
		Module._free(countwp);
		Module._free(counthp);
		throw new RDError(err);
	}

	const countw = Module.getValue(countwp,'i32');
	const counth = Module.getValue(counthp,'i32');
	const reswval = Module.getValue(reswp,'*');
	const reshval = Module.getValue(reshp,'*');

	const resw = Array.from({ length: countw }, (_,i) => Resolution.fromRDResolution(reswval+i*8))
	const resh = Array.from({ length: counth }, (_,i) => Resolution.fromRDResolution(reshval+i*8))

	Module._free(reswval);
	Module._free(reshval);
	Module._free(reswp);
	Module._free(reshp);
	Module._free(countwp);
	Module._free(counthp);

	return { 'widths': resw, 'heights': resh };
}

export { RDError, Method, Resolution, Analysis, libVersion, defaultRange, methods, resDetect };
