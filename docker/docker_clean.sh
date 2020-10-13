#!/usr/bin/env bash

readonly SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

source "$SCRIPTS_DIR/common.sh"

echo "Removing ${DOCKER_CONTAINER_NAME}"
docker stop "${DOCKER_CONTAINER_NAME}" || true
docker rm "${DOCKER_CONTAINER_NAME}" || true

echo "Removing ${DOCKER_IMAGE_NAME}"
docker image rm "${DOCKER_IMAGE_NAME}" || true

echo "Run docker system prune to remove hanging images"