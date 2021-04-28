import unittest
import StringIO

from lib.parsecomponent import parse_component_depends

class TestParseComponent(unittest.TestCase):
    def test_no_depends(self):
        filelike = StringIO.StringIO(
            '{\n'
            '    "name": "otk-test",\n'
            '    "description": "Test widget",\n'
            '    "dependencies": []\n'
            '}'
        )

        depends = parse_component_depends(filelike)
        self.assertEqual(depends, set())
    
    def test_one_depends(self):
        filelike = StringIO.StringIO(
            '{\n'
            '    "name": "otk-test",\n'
            '    "description": "Test widget",\n'
            '    "dependencies": [\n'
            '        "otk-base"\n'
            '    ]\n'
            '}'
        )

        depends = parse_component_depends(filelike)
        self.assertEqual(depends, set(['otk-base']))
    
    def test_multi_depends(self):
        filelike = StringIO.StringIO(
            '{\n'
            '    "name": "otk-test",\n'
            '    "description": "Test widget",\n'
            '    "dependencies": [\n'
            '        "otk-base",\n'
            '        "otk-count-bubble"\n'
            '    ]\n'
            '}'
        )

        depends = parse_component_depends(filelike)
        self.assertEqual(depends, set(['otk-base', 'otk-count-bubble']))
