.PHONY: docker
.PHONY: docker-clean
.PHONY: docker-build
.PHONY: docker-run
.PHONY: docker-test

docker-build:
	./docker/docker_build.sh litmus -d .

docker-test: docker-build
	./docker/docker_run.sh unittests "$(TESTS)" "$(BIN_ARGS)"

docker-run: docker-build
	./docker/docker_run.sh litmus

docker-interact: docker-build
	./docker/docker_run.sh interactive

docker-clean:
	./docker/docker_clean.sh

docker: docker-build docker-test