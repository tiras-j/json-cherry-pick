.PHONY: all coverage docs test tests clean install-hooks simple-benchmark

all: install-hooks test

tests: test

test:
	tox2

coverage:
	tox2 -e coverage

docs:
	tox2 -e docs

clean:
	find . -name '*.pyc' -delete
	find . -name '__pycache__' -delete
	rm -rf docs/build/*
	rm -rf .tox
	rm -rf build
	rm -rf dist

install-hooks:
	tox2 -e pre-commit -- install -f --install-hooks

simple-benchmark:
	./benchmark/run_benchmarks.sh
