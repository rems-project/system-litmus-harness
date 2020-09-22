#!/usr/bin/env bash

# set -x

readonly SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

source "$SCRIPTS_DIR/common.sh"


usage() {
  echo "Usage: docker_run.sh [-i]"
  echo
  echo "Creates and starts a docker container with a built version of system-litmus-harness."
  echo " with -i:    Attaches stdin/stdout to a shell in the container."
  echo " without -i: Runs @all"
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
    --name "${NAME}" \
    --workdir /home/${DOCKER_USER}/system-litmus-harness \
    -i -t \
    "${DOCKER_IMAGE_NAME}"
  )
  echo "created container ${NAME}/${cid}"

  docker start ${cid}
  echo "started container ${NAME}/${cid}"
}

run() {
  # Run the image non-interactively
  docker run \
    --name "${NAME}" \
    --workdir /home/${DOCKER_USER}/system-litmus-harness \
    -t \
    "${DOCKER_IMAGE_NAME}" \
    'bash' \
    '-c' \
    'make build && ./bin/qemu_litmus.exe @all | tee output.log'
}

if [[ "$#" -lt 0 ]] ; then
  usage;
  exit 1;
fi

readonly NAME="${DOCKER_IMAGE_NAME}-container"

if [[ -z "$(docker images --all -q --filter "reference=${DOCKER_IMAGE_NAME}")" ]] ; then
  no_image;
  exit 1;
fi

echo "Found ${NAME}"

if [[ "$(docker container ls --all --quiet --filter "name=^${NAME}$")" ]] ; then
  echo "Container ${NAME} already exists.";
  read -p "Attach? (y/n/remove) " yn;
  case $yn in
    y)
      echo "attach" ;
      cid=$(docker container ls --all --quiet --filter "name=^${NAME}$")
      attach ${cid}
      exit 0
      ;;
    n)
      exit 0
      ;;
    rm|remove)
      echo "Removing ${NAME}"
      docker stop "${NAME}"
      docker rm "${NAME}"
      ;;
    *)
      echo "Unknown option ${yn}. Please enter y or n."
      exit 1
      ;;
  esac
fi

# Remove old running containers, if exist ...
# TODO: if running ASK then if Y then KILL else STOP
[[ -z "$(docker container ls --all --quiet --filter "name=^${NAME}$")" ]] || docker rm "${NAME}"

if [[ "$#" -lt 1 ]] ; then
  echo "Running @all"
  echo " ... use ./docker_run.sh -i to run interactively"
  run
else
  case "$1" in
    -i)
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