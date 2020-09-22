.PHONY: docker
.PHONY: docker-build
.PHONY: docker-litmus
.PHONY: docker-unittests

docker-build:
	./docker/docker_build.sh litmus -d .

docker-unittests: docker-build
	./docker/docker_run.sh unittests

docker-litmus: docker-build
	./docker/docker_run.sh litmus

docker: docker-build docker-unittests