.PHONY: docker
.PHONY: docker-clean
.PHONY: docker-build
.PHONY: docker-litmus
.PHONY: docker-unittests

docker-build:
	./docker/docker_build.sh litmus -d .

docker-unittests: docker-build
	./docker/docker_run.sh unittests "$(TESTS)" "$(BIN_ARGS)"

docker-litmus: docker-build
	./docker/docker_run.sh litmus

docker-interact: docker-build
	./docker/docker_run.sh interactive

docker-clean:
	./docker/docker_clean.sh

docker: docker-build docker-unittests