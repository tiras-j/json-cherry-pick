#!/bin/bash
set -e -x

python setup.py test
rm -rf build/
