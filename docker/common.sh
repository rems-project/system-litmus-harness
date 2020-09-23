#!/usr/bin/env bash

readonly DOCKER_IMAGE_NAME='system-litmus-harness'
readonly DOCKER_CONTAINER_NAME="${DOCKER_IMAGE_NAME}-container"

readonly DOCKER_UID=1000   # "$(id --user)"
readonly DOCKER_GID=1000   # "$(id --group)"
readonly DOCKER_USER=rems  # "$(id --user --name)"
readonly DOCKER_GROUP=rems # "$(id --group --name)"