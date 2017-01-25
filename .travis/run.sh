#!/bin/bash
set -e +x

# Build wheels -> ./wheelhouse/*.whl
docker run --rm -v `pwd`:/io $DOCKER_IMAGE $PRE_CMD /io/.travis/build-wheels.sh

# Build sdist into dist/
python setup.py sdist

# debug print
ls wheelhouse

# Upload to twine
if ! [[ -z "$TRAVIS_TAG" ]] ; then
    echo "Uploading to PYPI"
    twine upload wheelhouse/* -u kehtnok -p $PYPI_PW
else
    echo "No tag, not running upload"
fi

echo "./run.sh exiting"
