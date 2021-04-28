import re

def parse_dependfile(filelike):
    depend_set = set()

    for line in filelike:
        # Kill everything after # and remove whitespace
        cleaned_line = re.sub(r'#.*', '', line).strip()

        if cleaned_line != '':
            depend_set.add(cleaned_line)
    
    return depend_set
