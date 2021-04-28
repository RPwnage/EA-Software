import sys

class MissingStringNotifier(object):
    """De-duplicates missing string warnings during a translation run"""
    def __init__(self):
        self.missing_strings = set()

    def string_missing(self, string):
        self.missing_strings.add(string)

    def output_missing_strings(self):
        for string in self.missing_strings:
            sys.stderr.write("Unable to lookup string: " + string + "\n")
