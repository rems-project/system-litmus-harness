#!/usr/bin/env bash

# set -x

readonly SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

source "$SCRIPTS_DIR/common.sh"


usage() {
  echo "Usage: docker_run.sh litmus|unittests|interactive"
  echo
  echo "Creates and starts a docker container with a built version of system-litmus-harness."
  echo "litmus:  runs the litmus tests and exits."
  echo "unittests:  runs the unittests and exits."
  echo "interactice: starts an interactive session in the system-litmus-harness directory."
}

no_image() {
  echo "Could not find a '${DOCKER_IMAGE_NAME}' docker image."
  echo
  echo "Run   docker_build.sh litmus   to build it."
}

attach() {
  echo "Attaching to Container.  Use Ctrl-X to detach."
  docker attach --detach-keys="ctrl-x" $1
}

create() {
  # Run the image interactively
  cid=$(
  docker create \
    --name "${DOCKER_CONTAINER_NAME}" \
    --workdir /home/${DOCKER_USER}/system-litmus-harness \
    -i -t \
    "${DOCKER_IMAGE_NAME}"
  )
  echo "created container ${DOCKER_CONTAINER_NAME}/${cid}"

  docker start ${cid}
  echo "started container ${DOCKER_CONTAINER_NAME}/${cid}"
}

run() {
  if [ "$1"="unittests" ] ; then
    CMD="make unittests BIN_ARGS='${BIN_ARGS}' TESTS='${TESTS}' | tee unittests-output.log"
  else
    CMD="make build && ./bin/qemu_litmus.exe BIN_ARGS='${BIN_ARGS}' LITMUS_TESTS='${TESTS}' | tee litmus-output.log"
  fi

  # Run the image non-interactively
  docker run \
    --name "${DOCKER_CONTAINER_NAME}" \
    --workdir /home/${DOCKER_USER}/system-litmus-harness \
    -t -i \
    --rm \
    "${DOCKER_IMAGE_NAME}" \
    'bash' \
    '-c' \
    "$CMD"
}

if [[ "$#" -lt 0 ]] ; then
  usage;
  exit 1;
fi

if [[ -z "$(docker images --all -q --filter "reference=${DOCKER_IMAGE_NAME}")" ]] ; then
  no_image;
  exit 1;
fi

echo "Found ${DOCKER_CONTAINER_NAME}"

if [[ "$(docker container ls --all --quiet --filter "name=^${DOCKER_CONTAINER_NAME}$")" ]] ; then
  echo "Container ${DOCKER_CONTAINER_NAME} already exists.";
  read -p "Attach? (y/n/remove) " yn;
  case $yn in
    y)
      echo "attach" ;
      cid=$(docker container ls --all --quiet --filter "name=^${DOCKER_CONTAINER_NAME}$")
      attach ${cid}
      exit 0
      ;;
    n)
      exit 0
      ;;
    rm|remove)
      echo "Removing ${DOCKER_CONTAINER_NAME}"
      docker stop "${DOCKER_CONTAINER_NAME}"
      docker rm "${DOCKER_CONTAINER_NAME}"
      ;;
    *)
      echo "Unknown option ${yn}. Please enter y or n."
      exit 1
      ;;
  esac
fi

# Remove old running containers, if exist ...
# TODO: if running ASK then if Y then KILL else STOP
[[ -z "$(docker container ls --all --quiet --filter "name=^${DOCKER_CONTAINER_NAME}$")" ]] || docker rm "${DOCKER_CONTAINER_NAME}"

if [[ "$#" -lt 1 ]] ; then
  usage
  exit 1
else
  case "$1" in
    litmus)
      if [[ "$#" -lt 2 ]] ; then
        readonly TESTS="@all"
        readonly BIN_ARGS=""
      elif [[ "$#" -lt 3 ]] ; then
        readonly TESTS="@all"
        readonly BIN_ARGS="$2"
      else
        readonly TESTS="$2"
        readonly BIN_ARGS="$3"
      fi
      echo "Running litmus tests"
      run qemu_litmus.exe
      ;;
    unittests)
      if [[ "$#" -lt 2 ]] ; then
        readonly TESTS="."
        readonly BIN_ARGS=""
      elif [[ "$#" -lt 3 ]] ; then
        readonly TESTS="."
        readonly BIN_ARGS="$2"
      else
        readonly TESTS="$2"
        readonly BIN_ARGS="$3"
      fi
      echo "Running unittests"
      run qemu_unittests.exe
      ;;
    interactive)
      create

      # Overwrite system-litmus-harness/ dir
      # docker cp . ${cid}:home/rems/system-litmus-harness/
      echo "ADD source to /home/rems/system-litmus-harness/"

      # finally attach so we can use it
      attach ${cid}
      ;;
    *)
      echo "Unknown argument '$1'"
      echo "Expect -i or nothing"
      exit 1
      ;;
    esac
fi