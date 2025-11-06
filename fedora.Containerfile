FROM quay.io/fedora/fedora:42 AS base

WORKDIR /workdir

RUN <<DNF
dnf install -y \
    https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm \
    https://mirrors.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm
dnf install -y --setopt=install_weak_deps=False \
    libpng \
    libjpeg \
    mjpegtools-libs \
    ImageMagick-libs \
    fftw-libs \
    # keep this line
dnf clean all
DNF

FROM base AS builder

RUN <<DNF
dnf install -y --setopt=install_weak_deps=False \
    make \
    gcc \
    glibc-devel \
    libpng-devel \
    libjpeg-devel \
    mjpegtools-devel \
    ImageMagick-devel \
    fftw-devel \
    # keep this line
dnf clean all
DNF

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
