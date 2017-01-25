import unittest
import json
import json_cherry_pick as jcp

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

class TestBasicExtraction(unittest.TestCase):

    def test_basic_extraction(self):
        sut = jcp.loads(json.dumps(ascii_basic))

        for k, v in ascii_basic.items():
            self.assertEqual(sut[k], v)

    def test_get(self):
        sut = jcp.loads(json.dumps(ascii_basic))

        self.assertEqual(sut.get('test_str'), ascii_basic.get('test_str'))
        self.assertEqual(sut.get('blah'), None)
        self.assertEqual(sut.get('blah', 1), 1)

class TestReification(unittest.TestCase):
    
    def test_multiple_reassignment(self):
        sut = jcp.loads(json.dumps(ascii_basic))

        sut['test_obj']['item1'] = 'Blah'
        sut['test_obj']['item2'] = 'Bleh'

        self.assertEqual(sut['test_obj']['item1'], 'Blah')
        self.assertEqual(sut['test_obj']['item2'], 'Bleh')

        sut['test_obj']['item1'] = 'NewBlah'
        sut['test_obj']['item2'] = 'NewBleh'
        
        self.assertEqual(sut['test_obj']['item1'], 'NewBlah')
        self.assertEqual(sut['test_obj']['item2'], 'NewBleh')
