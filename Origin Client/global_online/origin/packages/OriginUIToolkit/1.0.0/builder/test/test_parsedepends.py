import unittest
import StringIO

from lib.parsedepends import parse_dependfile

class TestParseDepends(unittest.TestCase):
    def test_empty_file(self):
        filelike = StringIO.StringIO()

        # Should be no dependencies
        depends = parse_dependfile(filelike)
        self.assertEqual(depends, set())

    def test_single_line_nonewline(self):
        filelike = StringIO.StringIO(
            'depend1'
        )

        depends = parse_dependfile(filelike)
        self.assertEqual(depends, set(['depend1']))
    
    def test_single_line_unix_newline(self):
        filelike = StringIO.StringIO(
            'depend1\n'
        )

        depends = parse_dependfile(filelike)
        self.assertEqual(depends, set(['depend1']))
    
    def test_single_line_dos_newline(self):
        filelike = StringIO.StringIO(
            'depend1\r\n'
        )

        depends = parse_dependfile(filelike)
        self.assertEqual(depends, set(['depend1']))
    
    def test_commented_line(self):
        filelike = StringIO.StringIO(
            '# This is an awesome dependency\n'
            'depend1\n'
        )

        depends = parse_dependfile(filelike)
        self.assertEqual(depends, set(['depend1']))
    
    def test_trimming_whitespace(self):
        filelike = StringIO.StringIO(
            '    depend1\t   \t   '
        )

        depends = parse_dependfile(filelike)
        self.assertEqual(depends, set(['depend1']))
    
    def test_same_line_comment(self):
        filelike = StringIO.StringIO(
            'depend1 # We should be using depend2 here, its awesomer\b'
        )

        depends = parse_dependfile(filelike)
        self.assertEqual(depends, set(['depend1']))
    
    def test_blank_lines(self):
        filelike = StringIO.StringIO(
            '\n'
            'depend1\n'
            '\n'
            'depend2\n'
        )

        depends = parse_dependfile(filelike)
        self.assertEqual(depends, set(['depend1', 'depend2']))
    
    def test_duplicates(self):
        filelike = StringIO.StringIO(
            'depend1\n'
            'depend2\n'
            'depend1\n'
            'depend3\n'
        )

        depends = parse_dependfile(filelike)
        self.assertEqual(depends, set(['depend1', 'depend2', 'depend3']))
    
    def test_complex(self):
        filelike = StringIO.StringIO(
            'depend1   \n'
            '\n'
            '# This dependency is broken'
            '# depend2\n'
            '   depend1\n'
            '  depend3  # depend3 supercedes depend2'
        )

        depends = parse_dependfile(filelike)
        self.assertEqual(depends, set(['depend1', 'depend3']))
