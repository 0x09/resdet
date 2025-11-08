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
RUN CFLAGS='-O0 -mtune=generic' ./configure
RUN make

FROM base AS runtime

COPY --from=builder /workdir/resdet /usr/local/bin/resdet

# check for missing libraries
RUN <<LDD
missing=$(ldd -- /usr/local/bin/resdet | grep "not found")
[ -z "$missing" ] || { printf "%s\n" "$missing"; exit 1; }
LDD

ENTRYPOINT [ "/usr/local/bin/resdet" ]
