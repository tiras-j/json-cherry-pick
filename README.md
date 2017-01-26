# JSON Cherry-Pick
JSON Cherry-Pick - Efficiently extract sub-values from large JSON blobs.<br/>

## Installation
Install with `pip install json-cherry-pick`

Wheels are available for most python versions, under the *manylinux1_x86_64* platform, as well as source distribution. Importantly, building from source will not properly resolve python symbols during link-time if *python-dev* is not available on the platform. In such a case the package will default to the python implementation, which is 100% slower than the c. So if possible use the wheel packages or ensure *python-dev* is installed on the host platform.
Note: Installation will still work, but an `ImportError` is raised if the symbols are not resolvable.

## Usage
JSON Cherry-Pick works by scanning the input JSON and building a flat hashmap of indices to key/value pairs using the `mapper.MarkerMap` type. This object supports read-only Python Mapping Protocol, where accessing a key will lazily JSON load the associated value. Key existence is supported without requiring any JSON loading.

Example:
```python
>>> obj = json_cherry_pick.loads('{"hello": "there"}')
>>> 'hello' in obj
True               #(Note: The value "there" has not been loaded yet)
>>> obj['hello']
u'there'           #(Here the value is JSON decoded)
```
This is of course a trivial example.

JSON Cherry-Pick also flattens nested dictionaries, such that checking for subkeys only requires the subkey itself and not the parents:
```python
>>> obj = json_cherry_pick.loads('{"hello": {"are": {"you": "there?"}}}')
>>> 'you' in obj
True
```

## Performance
JSON Cherry-Pick performance advantage increases with the size of the objects being processed. An anecdotal example processing ~3.5GB of objects between 10KB->1MB shows a 10x performance increase for selectively rejecting objects based on existence of a subset of keys. However, the performance advantage decreases with the percentage of the object that is loaded, approaching a negative advantage if *every* key is loaded (since this results in 100% object creation + 2x input data scan)

