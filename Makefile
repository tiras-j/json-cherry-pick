.PHONY: all test tests clean simple-benchmark

all: install-hooks test

tests: test

test:
	python setup.py test

clean:
	find . -name '*.pyc' -delete
	find . -name '__pycache__' -delete
	rm -rf docs/build/*
	rm -rf .tox
	rm -rf build
	rm -rf dist

simple-benchmark:
	./benchmark/run_benchmarks.sh
