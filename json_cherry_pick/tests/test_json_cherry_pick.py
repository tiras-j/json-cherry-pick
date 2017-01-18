# -*- coding: utf-8 -*-
from __future__ import unicode_literals
from __future__ import print_function

import json
import pytest
import sys

# I feel a little bad that I have absolutely no idea
# what these extractions mean. I've taken them from
# jpf.go.jp (japan foundation) and BBC arabic

unicode_basic = {
    'test_string': u'国際交流基',
    'test_num': 12334.0,
    'test_arr': [u'言葉を超えた共感', '123', u'また', 'four'],
    'test_obj': {'this': u'سيتبعها الرئيس الأمريكي المنتخب،', u'غرينتش': 1000},
    'test_empty_arr': [],
    'test_empty_obj': {}
}

ascii_basic = {
    "test_string": "abc123",
    "test_num": 12334.0,
    "test_arr": ["abc", "123", "four", "five"],
    "test_obj": {"this": "is a very basic", "object": 1000},
    "test_empty_arr": [],
    "test_empty_obj": {}
}


@pytest.fixture(params=[ascii_basic, unicode_basic], ids=["ascii", "unicode"])
def testdata(request):
    return json.dumps(request.param), request.param

@pytest.fixture(
    params=['json_cherry_pick.py_jcp', 'json_cherry_pick.c_jcp'],
    ids=['PyJCP', 'CJCP']
)
def jcp(request):
    mod = __import__(request.param)
    for submod in request.param.split('.')[1:]:
        mod = getattr(mod, submod)
    return mod

class TestExtraction(object):

    def test_extract_string(self, testdata, jcp):
        base, expected = testdata
        assert jcp.extract(base, 'test_string') == expected['test_string']

    def test_extract_num(self, testdata, jcp):
        base, expected = testdata
        assert jcp.extract(base, 'test_num') == expected['test_num']

    def test_extract_arr(self, testdata, jcp):
        base, expected = testdata
        assert jcp.extract(base, 'test_arr') == expected['test_arr']

    def test_pluck_list(self, testdata, jcp):
        base, expected = testdata
        assert jcp.pluck_list(base, 'test_arr', 2) == expected['test_arr'][2]

    def test_pluck_end_of_list(self, testdata, jcp):
        base, expected = testdata
        idx = len(expected['test_arr']) - 1
        assert jcp.pluck_list(base, 'test_arr', idx) == expected['test_arr'][idx]

    def test_extract_obj(self, testdata, jcp):
        base, expected = testdata
        assert jcp.extract(base, 'test_obj') == expected['test_obj']

    def test_extract_tokenized(self, testdata, jcp):
        base, expected = testdata
        assert jcp.extract(base, 'test_obj.this') == expected['test_obj']['this']

    def test_extract_empty_arr(self, testdata, jcp):
        base, expected = testdata
        assert jcp.extract(base, 'test_empty_arr') == expected['test_empty_arr']

    def test_extract_empty_obj(self, testdata, jcp):
        base, expected = testdata
        assert jcp.extract(base, 'test_empty_obj') == expected['test_empty_obj']

    def test_extract_unicode_key(self, jcp):
        base, expected = json.dumps(unicode_basic), unicode_basic
        s = u'غرينتش'.encode("utf-8") if sys.version_info < (3, 0) else u'غرينتش'
        print(json.dumps(u'غرينتش'))
        assert jcp.extract(base, s) == expected['test_obj'][u'غرينتش']


class TestExceptions(object):

    def test_key_error(self, testdata, jcp):
        base = testdata[0]
        with pytest.raises(KeyError):
            jcp.extract(base, 'invalid_key')

    def test_tokenized_key_error(self, testdata, jcp):
        base = testdata[0]
        with pytest.raises(KeyError):
            jcp.extract(base, 'test_obj.test_empty_obj')

    def test_index_error(self, testdata, jcp):
        base = testdata[0]
        with pytest.raises(IndexError):
            jcp.pluck_list(base, 'test_arr', 5)

    def test_edge_case_index_error(self, testdata, jcp):
        base = testdata[0]
        with pytest.raises(IndexError):
            jcp.pluck_list(base, 'test_empty_arr', 0)

    def test_reverse_index_exc(self, testdata, jcp):
        base = testdata[0]
        with pytest.raises(IndexError):
            jcp.pluck_list(base, 'test_arr', -1)
