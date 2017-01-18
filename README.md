# JSON Cherry-Pick
JSON Cherry Picker - Efficiently extract sub-values from large JSON blobs.<br/>

## Installation
Install with `pip install json-cherry-pick`

Wheels are available for most python versions, under the *manylinux1_x86_64* platform, as well as source distribution. Importantly, building from source will not properly resolve python symbols during link-time if *python-dev* is not available on the platform. In such a case the package will default to the python implementation, which is 100% slower than the c. So if possible use the wheel packages or ensure *python-dev* is installed on the host platform.
Note: Installation will still work, but an `ImportError` is raised if the symbols are not resolvable.

