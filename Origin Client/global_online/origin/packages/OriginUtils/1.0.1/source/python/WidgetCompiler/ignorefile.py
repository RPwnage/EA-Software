import re

def ignore_line_to_regex(rule):
    """Convert a single ignore file rule line to a regular expression"""
    subdir_match = False

    if rule.endswith("/"):
        # This is a subdirectory match
        rule = rule[:-1]
        subdir_match = True

    rule = re.escape(rule)

    rule = rule.replace(re.escape("*"), "[^/]*")
    rule = rule.replace(re.escape("?"), "[^/]?")

    if (rule.startswith("\\/")):
        # Root the beginning of the rule and drop the slash
        rule = "^" + rule[2:]
    elif "\\/" in rule:
        # Rules with / are rooted also
        rule = "^" + rule

    if subdir_match:
        # Add back on our /
        rule = rule + re.escape("/")
    else:
        # Root the end of the rule
        rule = rule + "($|\\/)"

    return re.compile(rule)

class IgnoreFile(object):
    """Class implementing a subset of gitignore style files"""

    def __init__(self, ignore_string):
        # Strip out comments and empty lines
        ignore_lines = \
            [line.rstrip() for line in ignore_string.splitlines() 
                  if not line.startswith('#')
                     and len(line) > 0
                     and not line.isspace()]

        self.positive_rules = \
            [ignore_line_to_regex(line) for line in ignore_lines
                if not line.startswith("!")]
        
        self.negative_rules = \
            [ignore_line_to_regex(line[1:]) for line in ignore_lines
                if line.startswith("!")]

    def path_ignored(self, path):
        """Determine if a path should be ignored"""
        # Negative rules first
        for rule_regex in self.negative_rules:
            if rule_regex.search(path) is not None:
                return False
        
        for rule_regex in self.positive_rules:
            if rule_regex.search(path) is not None:
                return True

        # Assume unignored
        return False
