from setuptools import setup
from setuptools import Extension

version = "0.3.2"

jcpmodule = Extension(
    'json_cherry_pick.mapper',
    sources=['json_cherry_pick/mapper.c', 'json_cherry_pick/marker_map.c'],
)

setup(
    name='json-cherry-pick',
    version=version,
    packages=['json_cherry_pick'],
    description='JSON Cherry Picker',
    long_description='json-cherry-pick provides quick access to sub-objects inside large JSON blobs',
    url='https://github.com/tiras-j/json-cherry-pick',
    download_url='https://github.com/tiras-j/json-cherry-pick/tarball/{0}'.format(version),
    author='Josh Tiras',
    author_email='jot@yelp.com',
    license='Apache2',
    keywords=['JSON', 'cherry', 'pick', 'extractor'],
    classifiers=[
        'Development Status :: 3 - Alpha',

        'Intended Audience :: Developers',
        'Topic :: Text Processing :: Indexing',
        'License :: OSI Approved :: Apache Software License',

        'Programming Language :: Python :: 2.6',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.3',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
    ],
    ext_modules=[jcpmodule],
    setup_requires=['pytest-runner'],
    tests_require=['pytest'],
)
