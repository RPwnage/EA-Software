import unittest
import StringIO
import os

from lib.resolvedepends import *
from lib.parsedepends import parse_dependfile
    
FIXTURE_DIR = os.path.join(os.path.dirname(__file__), 'testcomponents')

def _depend_list_for_string(depend_string):
    filelike = StringIO.StringIO(
        depend_string
    )

    depend_set = parse_dependfile(filelike)
    return resolve_depends('test', depend_set, FIXTURE_DIR)

class TestResolveDepends(unittest.TestCase):

    def test_no_depends(self):
        depend_string = (
            ''
        )

        depend_list = _depend_list_for_string(depend_string)
        self.assertEqual(depend_list, [])
    
    def test_missing_direct_depend(self):
        depend_string = (
            'this-does-not-exist'
        )

        with self.assertRaises(ComponentNotFoundError):
            _depend_list_for_string(depend_string)
    
    def test_missing_recursive_depend(self):
        depend_string = (
            'test-missingcomponent'
        )

        with self.assertRaises(ComponentNotFoundError):
            _depend_list_for_string(depend_string)
    
    
    def test_non_recursive_depend(self):
        depend_string = (
            'test-base'
        )

        depend_list = _depend_list_for_string(depend_string)
        self.assertEqual(depend_list, ['test-base'])
    
    def test_simple_recursive_depends(self):
        depend_string = (
            'test-childdepend'
        )

        depend_list = _depend_list_for_string(depend_string)
        # Make sure test-base is first
        self.assertEqual(depend_list, ['test-base', 'test-childdepend'])
    
    def test_dependency_deduplication(self):
        # test-base is included both by use and childdepend
        depend_string = (
            'test-childdepend\n'
            'test-base\n'
        )

        depend_list = _depend_list_for_string(depend_string)
        # Make sure test-base is first
        self.assertEqual(depend_list, ['test-base', 'test-childdepend'])
    
    def test_dependency_deduplication(self):
        # test-base is included both by use and childdepend
        depend_string = (
            'test-childdepend\n'
            'test-base\n'
        )

        depend_list = _depend_list_for_string(depend_string)
        self.assertEqual(depend_list, ['test-base', 'test-childdepend'])
    
    def test_complex_depend(self):
        depend_string = (
            'test-grandchilddepend-1\n'
            'test-grandchilddepend-2\n'
        )
        
        depend_list = _depend_list_for_string(depend_string)
        self.assertEqual(depend_list, ['test-base', 'test-childdepend', 'test-grandchilddepend-1', 'test-grandchilddepend-2'])
