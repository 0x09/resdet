FROM docker.io/library/debian:12-slim AS base

WORKDIR /workdir

RUN <<APT
apt-get update
apt-get --no-install-recommends install -y \
    libpng16-16 \
    libjpeg62-turbo \
    libmjpegutils-2.1-0 \
    libmagickwand-6.q16-6 \
    libfftw3-bin \
    # keep this line
rm -rf /var/lib/apt/lists/*
APT

FROM base AS builder

RUN <<APT
apt-get update
apt-get --no-install-recommends install -y \
    build-essential \
    libc-dev \
    libpng-dev \
    libjpeg62-turbo-dev \
    libmjpegtools-dev \
    libmagickwand-6.q16-dev \
    libfftw3-dev \
    # keep this line
rm -rf /var/lib/apt/lists/*
APT

COPY . .
RUN ./configure
RUN make

RUN <<CP
mkdir -p /dist/usr/local/bin
cp /workdir/resdet /dist/usr/local/bin
cp /lib64/ld-linux-x86-64.so.2 --parents /dist
ldd /workdir/resdet | awk 'NF == 4 { system("cp " $3 " --parents /dist") }'
CP

FROM gcr.io/distroless/static-debian12:debug AS debug

COPY --from=builder /dist/ /

# cachebust
ENV I=1

# this doesn't fail if something is missing
ENV LD_TRACE_LOADED_OBJECTS=1
RUN [ "/lib64/ld-linux-x86-64.so.2", "/usr/local/bin/resdet" ]

ENTRYPOINT [ "/usr/local/bin/resdet" ]

FROM gcr.io/distroless/static-debian12 AS runtime

COPY --from=builder /dist/ /

ENTRYPOINT [ "/usr/local/bin/resdet" ]
