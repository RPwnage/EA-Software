import unittest
from ignorefile import IgnoreFile

class TestIgnoreFile(unittest.TestCase):
    def test_empty_file(self):
        ignore = IgnoreFile("")

        self.assertFalse(ignore.path_ignored("test.py"))
        self.assertFalse(ignore.path_ignored("sub/directory"))
    
    def test_comment_line(self):
        ignore = IgnoreFile("# Hello")

        self.assertFalse(ignore.path_ignored("# Hello"))
    
    def test_whitespace_line(self):
        ignore = IgnoreFile("   ")

        self.assertFalse(ignore.path_ignored("   "))
    
    def test_unrooted_exact_match(self):
        ignore = IgnoreFile("example.cpp")
        
        self.assertTrue(ignore.path_ignored("example.cpp"))
        self.assertTrue(ignore.path_ignored("sub/example.cpp"))
        self.assertTrue(ignore.path_ignored("sub/example.cpp/file"))
    
    def test_rooted_exact_match(self):
        ignore = IgnoreFile("/example.cpp")
        
        self.assertTrue(ignore.path_ignored("example.cpp"))
        self.assertFalse(ignore.path_ignored("sub/example.cpp"))
        self.assertFalse(ignore.path_ignored("sub/example.cpp/file"))
    
    def test_unrooted_star_match(self):
        ignore = IgnoreFile("*.cpp")
        
        self.assertTrue(ignore.path_ignored("example.cpp"))
        self.assertTrue(ignore.path_ignored("sub/example.cpp"))
        self.assertTrue(ignore.path_ignored("sub/example.cpp/file"))
    
    def test_rooted_star_match(self):
        ignore = IgnoreFile("/*.cpp")
        
        self.assertTrue(ignore.path_ignored("example.cpp"))
        self.assertTrue(ignore.path_ignored("example.cpp/file"))
        self.assertFalse(ignore.path_ignored("sub/example.cpp"))

    def test_question_mark_match(self):
        ignore = IgnoreFile("foo.?")
        
        self.assertTrue(ignore.path_ignored("foo.m"))
        self.assertFalse(ignore.path_ignored("foo.mm"))
        self.assertTrue(ignore.path_ignored("sub/foo.m/file"))

    def test_trailing_slash(self):
        ignore = IgnoreFile("sub/")
        
        self.assertTrue(ignore.path_ignored("sub/file.h"))
        self.assertTrue(ignore.path_ignored("sub/another/file.h"))
        self.assertTrue(ignore.path_ignored("parent/sub/file.h"))
        self.assertFalse(ignore.path_ignored("sub"))
        self.assertFalse(ignore.path_ignored("parent/sub"))

    def test_fnmatch_style(self):
        ignore = IgnoreFile("Documentation/*.html")

        self.assertTrue(ignore.path_ignored("Documentation/git.html"))
        self.assertFalse(ignore.path_ignored("Documentation/ppc/ppc.html"))
        self.assertFalse(ignore.path_ignored("tools/perf/Documentation/perf.html"))

    def test_negative_rule(self):
        ignore = IgnoreFile("*.py\n!example.py")
        
        self.assertTrue(ignore.path_ignored("random.py"))
        self.assertFalse(ignore.path_ignored("example.py"))
