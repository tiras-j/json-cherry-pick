#!/bin/bash

echo $PWD
# This script will run benchmarks to analyze performance differences between
# HEAD and the currently non-commited changes changes made to the project. If there
# are no changes, the script exits without running benchmarks

if git diff-index --quiet HEAD -- ; then
    echo "No changes present, exiting..."
    exit 0
fi

headrev=$(git rev-parse --short HEAD)

echo Stashing pending changes
# Run the benchmarks against HEAD. Stash the current changes
git stash -q

echo Setting up benchmark environment
# Build the venv for python 2.7
virtualenv -q -p python2.7 benchmark/venv
source benchmark/venv/bin/activate
pip install -q wheel 

echo Building and running against $headrev
# Build the wheel into benchmark/ directory
python setup.py -q install --force

base=$(python benchmark/benchmark.py all)

# unpop our stash
git stash pop -q

echo Popping stashed changes
echo Building and running against tip
python setup.py -q install --force

new=$(python benchmark/benchmark.py all)

# print results
echo; echo; echo;
echo "Results comparing $headrev to staged changes"
echo "$base::$new" | python benchmark/benchmark.py result

# clean up everything
rm -rf benchmark/venv
exit 0
