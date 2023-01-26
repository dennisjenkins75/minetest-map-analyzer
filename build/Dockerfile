# syntax=docker/dockerfile:1
#
# Hermetic build environment.

FROM ubuntu:jammy-20220531

LABEL maintainer="dennisjenkins <dennis.jenkins.75@gmail.com>"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get upgrade -y

RUN apt-get install -yq build-essential git

RUN apt-get install -yq libcurl4-openssl-dev uuid-dev libzstd-dev zlib1g-dev \
  libabsl-dev libfmt-dev libpqxx-dev

RUN apt-get clean

RUN mkdir -p /vendor

RUN git clone https://github.com/premake/premake-core.git /vendor/premake && \
  cd /vendor/premake && \
  make -f Bootstrap.mak linux && \
  cp -av ./bin/release/premake5 /usr/local/bin/premake5 && \
  cp -av ./bin/release/*.so /usr/local/lib/

CMD ["/bin/bash", "-i"]
