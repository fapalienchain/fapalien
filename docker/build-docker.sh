#!/usr/bin/env bash

export LC_ALL=C

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/.. || exit

DOCKER_IMAGE=${DOCKER_IMAGE:-fapalien/fapaliend-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}

BUILD_DIR=${BUILD_DIR:-.}

rm docker/bin/*
mkdir docker/bin
cp $BUILD_DIR/src/fapaliend docker/bin/
cp $BUILD_DIR/src/fapalien-cli docker/bin/
cp $BUILD_DIR/src/fapalien-tx docker/bin/
strip docker/bin/fapaliend
strip docker/bin/fapalien-cli
strip docker/bin/fapalien-tx

docker build --pull -t $DOCKER_IMAGE:$DOCKER_TAG -f docker/Dockerfile docker
