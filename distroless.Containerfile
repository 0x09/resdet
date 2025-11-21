FROM docker.io/library/debian:13-slim AS base

WORKDIR /workdir

RUN <<APT
apt-get update
apt-get --no-install-recommends install -y \
    libpng16-16 \
    libjpeg62-turbo \
    libmagickwand-7.q16-10 \
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
    libmagickwand-7.q16-dev \
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

FROM gcr.io/distroless/static-debian13:debug AS debug

COPY --from=builder /dist/ /

RUN [ "ln", "-s", "/busybox/sh", "/bin/sh" ]

# check for missing libraries
RUN <<LDD
missing=$(LD_TRACE_LOADED_OBJECTS=1 /lib64/ld-linux-x86-64.so.2 /usr/local/bin/resdet | grep "not found")
[ -z "$missing" ] || { printf "%s\n" "$missing"; exit 1; }
LDD

ENTRYPOINT [ "/usr/local/bin/resdet" ]

FROM gcr.io/distroless/static-debian13 AS runtime

COPY --from=builder /dist/ /

ENTRYPOINT [ "/usr/local/bin/resdet" ]
