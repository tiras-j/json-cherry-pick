Release Notes
=============
0.3.20
------

* Fix missing traverse/clear methods on type definition

0.3.19
------

* Fix py2->3 compat on tp_free
* Support for cyclical garbage collection of json objects we hold

0.3.18
------

* Properly call tp_free on dealloc

0.3.17
------

* Fix refcount on default value from `get()`

0.3.16
------

* Add support for `get(key,[default])` operations

0.3.15
------

* Fix REFCOUNT issue with returning cached JSON objects

0.3.14
------

* Consistent build pipeline established w/ Travis CI
* Builds 0.3.3->0.3.13 testing for tagged deployment

0.3.2
-----

* Add loads wrapper

0.3.1
-----

* Tag target for build

0.3.0
-----

* Merge Project Sicily

0.2.0
-----

* Add python version for compatibility

0.1.2
-----

* Initial UTF-8 support for `extract`

0.1.1
-----

* Consolidate source files
* Fix bdist_wheel build

0.1.0
-----

* First stable minor release
* Tests against py26,py27,py34,py35
* Exposes `extract`, `extract_all`, `pluck_list`
