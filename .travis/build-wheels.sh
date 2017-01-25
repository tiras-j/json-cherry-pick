#!/bin/bash

set -e -x

# Compile wheels
for PYBIN in /opt/python/*/bin; do
    "${PYBIN}/pip" install -r /io/requirements-dev.txt
    "${PYBIN}/pip" wheel /io/ -w wheelhouse/
done

for whl in wheelhouse/*.whl; do
    auditwheel repair "$whl" -w /io/wheelhouse/
done

for PYBIN in /opt/python/*/bin/; do
    "${PYBIN}/pip" install json-cherry-pick --no-index -f /io/wheelhouse
    (cd "$HOME"; "${PYBIN}/nosetests" json_cherry_pick)
done
