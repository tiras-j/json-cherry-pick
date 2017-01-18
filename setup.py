from setuptools import setup
from setuptools import Extension

version = "0.1.2"

jcpmodule = Extension(
    'json_cherry_pick',
    sources=['json_cherry_pick/json_cherry_pick.c'],
)

setup(
    name='json-cherry-pick',
    version=version,
    description='JSON Cherry Picker',
    long_description='jcp provides quick access to sub-objects inside large JSON blobs',
    url='https://github.com/tiras-j/jcp',
    download_url='https://github.com/tiras-j/jcp/tarball/{0}'.format(version),
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
)
