#!/bin/bash
set -e -x

# Build wheels -> ./wheelhouse/*.whl
docker run --rm -v `pwd`:/io $DOCKER_IMAGE $PRE_CMD /io/.travis/build-wheels.sh

# Build sdist into dist/
python setup.py sdist

# move all wheels into the dist/* package
mv wheelhouse/* dist/*

# Upload to twine
if ! [ -z $TRAVIS_TAG ] ; then
    twine upload dist/* -u kehtnok -p $PYPI_PW
fi
