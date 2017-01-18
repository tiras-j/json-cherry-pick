#!/bin/bash
set -e -x

for PYBIN in /opt/python/*/bin; do
    "${PYBIN}/pip" install -r /io/requirements-dev.txt
    "${PYBIN}/python" /io/setup.py test
    "${PYBIN}/pip" wheel /io/ -w wheelhouse/
done
