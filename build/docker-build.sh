#!/bin/bash

set -e

IMAGE="minetest-map-analyzer-build-env"

docker build -t ${IMAGE} ./build

docker run --name="minetest-map-analyzer-build-env" \
  --interactive --rm --tty \
  --user $(id -u ${USER}):$(id -g ${USER}) \
  --mount type=bind,target=/source,src=$(pwd) \
  ${IMAGE} /source/build/project-build.sh
