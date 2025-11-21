FROM docker.io/library/debian:13-slim AS base

WORKDIR /workdir

RUN <<APT
apt-get update
apt-get --no-install-recommends install -y \
    libpng16-16 \
    libjpeg62-turbo \
    libmagickwand-7.q16-10 \
    libfftw3-bin \
    libavcodec61 \
    libavformat61 \
    libavutil59 \
    libswscale8 \
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
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    # keep this line
rm -rf /var/lib/apt/lists/*
APT

COPY . .
RUN ./configure
RUN make

FROM base AS runtime

COPY --from=builder /workdir/resdet /usr/local/bin/resdet

# check for missing libraries
RUN <<LDD
missing=$(ldd -- /usr/local/bin/resdet | grep "not found")
[ -z "$missing" ] || { printf "%s\n" "$missing"; exit 1; }
LDD

ENTRYPOINT [ "/usr/local/bin/resdet" ]
