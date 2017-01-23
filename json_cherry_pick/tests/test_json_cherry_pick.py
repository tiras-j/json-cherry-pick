import unittest
import json
import json_cherry_pick as jcp


class TestBasicExtraction(unittest.TestCase):
    ascii_basic = {
        'test_str': 'hello, world',
        'test_val': 1234567.89,
        'test_arr': ['one', 'two', 'three'],
        'test_obj': {'hello': 'there'},
        'test_empty_arr': [],
        'test_empty_obj': {},
        'test_nest_arr': [[1, 2], [3, 4], [5, 6]],
        'test_nest_obj': {'outer': {'inner': 'value'}},
    }

    def test_basic_extraction(self):
        sut = jcp.loads(json.dumps(self.ascii_basic))

        for k, v in self.ascii_basic.items():
            self.assertEqual(sut[k], v)
