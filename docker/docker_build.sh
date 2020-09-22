#!/usr/bin/env bash

set -o errexit
set -o nounset
shopt -s extglob

readonly SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

source "$SCRIPTS_DIR/common.sh"

usage() {
  echo "Usage: docker_build.sh litmus"
  echo
  echo "Options:"
  echo "  -d <path>          Use <path> as the path to a local harness installation"
  echo "                     to be used in docker"
  echo "  --force            Do not check <path/to/harness> contains the source of of system-litmus-harness"
}

if [[ "$#" -lt 1 ]] ; then
  usage
  exit 1
fi


build() {
  # heuristc to check that this dir is really system-litmus-harness
  # if not give the user an opt-in way to force it.
  if [[ $HARNESS_FORCE != 1 ]] && ! [ -f litmus/main.c ] ; then
    echo "'${HARNESS_DIR}' does not appear to be a directory containing system-litmus-harness."
    echo "Run  docker_build.sh litmus -d '${HARNESS_DIR}' --force  to force using this directory"
    echo
    usage
    exit 1
  fi

  docker build \
    --tag "$DOCKER_IMAGE_NAME" \
    -f- ${HARNESS_DIR} < "$SCRIPTS_DIR/Dockerfile"
}

case "$1" in
  litmus)
    shift 1
    if [[ "$#" -lt 2 || "$1" != "-d" ]]; then
      usage
      exit 1
    fi
    readonly HARNESS_DIR="$2"
    shift 2
    if [[ "$#" -lt 1 || "$1" != "--force" ]]; then
      readonly HARNESS_FORCE=0
    elif [[ "$#" -lt 1 ]]; then
      usage
      exit 1
    else
      readonly HARNESS_FORCE=1
    fi

    build
    ;;
  *)
    usage
    exit 1
esac