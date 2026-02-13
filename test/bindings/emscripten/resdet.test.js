import { expect, test } from 'vitest'
import * as resdet from '../../../bindings/emscripten/resdet.mjs'

const test_image = new Float32Array([1, 0, 0, 1]);
const test_resolution_dict =  {
	'widths': [new resdet.Resolution(2, -1.0)],
	'heights': [new resdet.Resolution(2, -1.0)]
}

test('test libVersion', () => {
	expect(resdet.libVersion()).toMatch(/\d+\.\d+\.\d+(\+[a-f0-9]+)?/)
})

test('methods', () => {
	const methodNames = resdet.methods().map(method => method.name)
	expect(methodNames).toStrictEqual(['sign','mag','orig','zerox'])
})

test('gets method', () => {
	const method = resdet.Method.get('zerox')
	expect(method.name).toBe('zerox')
})

test('detects resolutions', () => {
	const resolutions = resdet.resDetect(test_image, 1, 2, 2);
	expect(resolutions).toStrictEqual(test_resolution_dict);
})

test('sets parameters', () => {
	const image = new Float32Array([0,1,0,1])
	let resolutions = resdet.resDetect(image, 1, 4, 1);
	expect(resolutions['widths'].length).toBe(1);

	resolutions = resdet.resDetect(image, 1, 4, 1, null, { 'range': 1, 'threshold': 0 });
	expect(resolutions['widths'].length).toBe(3);

	resolutions = resdet.resDetect(image, 1, 4, 1, null, { 'range': 1, 'threshold': 0, 'compression_filter': 4 });
	expect(resolutions['widths'].length).toBe(2);
})

test('raises invalid image error', () => {
	expect(() => resdet.resDetect(new Float32Array([1]),1, 0, 0)).toThrowError(expect.objectContaining({
		message: 'Invalid image',
	}));
})

test('raises no images error', () => {
	expect(() => resdet.resDetect(new Float32Array([1]), 0, 1, 1)).toThrowError(expect.objectContaining({
		message: 'No images were analyzed',
	}));
})

test('raises with unrecognized parameter', () => {
	expect(() => resdet.resDetect(new Float32Array([1]), 1, 1, 1, null, { 'unrecognized': 1 })).toThrowError(expect.objectContaining({
		message: 'Unrecognized parameters: unrecognized',
	}));
})

test('analyzes resolutions', () => {
	const analysis = new resdet.Analysis(2, 2);
	analysis.analyzeImage(test_image);
	const resolutions = analysis.analysisResults();
	expect(resolutions).toStrictEqual(test_resolution_dict);
})
