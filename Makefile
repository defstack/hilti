#
# This is just a wrapper around the CMake setup.
#

BUILD=build

GIT=git://git.icir.org/hilti

DOCKER_IMAGE=rsmmr/hilti
DOCKER_TMP=/xa/robin/hilti-docker-build

all: configured
	$(MAKE) -C $(BUILD) $@

clean:
	rm -rf build
	( cd doc && $(MAKE) clean )

test:
	-( cd tests && btest -j -f diag.log )
	-( cd bro/tests && btest -j -f diag.log )

tags:
	update-tags

configured:
	@test -d $(BUILD) || ( echo "Error: No build/ directory found. Did you run configure?" && exit 1 )
	@test -e $(BUILD)/Makefile || ( echo "Error: No build/Makefile found. Did you run configure?" && exit 1 )

clang-format:
	@test $$(clang-format -version | cut -d ' ' -f 3 | sed 's/\.//g') -ge 390 || (echo "Must have clang-format >= 3.9"; exit 1)
	@(clang-format -dump-config | grep -q SpacesAroundConditions) || (echo "Must have patched version of clang-format"; exit 1)
	clang-format -i $$(scripts/source-files)

docker-build:
	rm -rf $(DOCKER_TMP)
	mkdir -p $(DOCKER_TMP)
	(cd $(DOCKER_TMP) && git clone ${GIT})
	cp Makefile $(DOCKER_TMP)/hilti
	cp Dockerfile $(DOCKER_TMP)/hilti
	(cd $(DOCKER_TMP)/hilti && make docker-build-internal)

docker-build-internal:
	docker build -t ${DOCKER_IMAGE} .
	docker tag -f `docker inspect --format='{{.Id}}' ${DOCKER_IMAGE}` ${DOCKER_IMAGE}:`cat VERSION`
	docker tag -f `docker inspect --format='{{.Id}}' ${DOCKER_IMAGE}` ${DOCKER_IMAGE}:latest

docker-run:
	docker run -i -t ${DOCKER_IMAGE}

docker-push:
	docker login
	docker push ${DOCKER_IMAGE}
