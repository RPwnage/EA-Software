import os
from xml import sax

ROOT_ELEMENT = "locStrings"
LOCALE_ATTRIBUTE = "locale"

STRING_ELEMENT = "locString"
STRING_ID_ATTRIBUTE = "Key"

def hal_locale_to_bcp47(locale):
    """Try to turn a locale in to a BCP 47. Identity function for BCP 47 tags"""

    if locale == "no_NO":
        # nb-no appears to be slightly more correct
        return 'nb-no'

    return locale.replace('_', '-').lower()

class HALParseError(Exception):
    pass

class HALSaxHandler(sax.handler.ContentHandler):
    """Parser for HAL XML files implemented as a SAX handler"""
    def __init__(self):
        """Create a new parser"""
        self.strings = {}
        self.locale = None

    def startElement(self, name, attrs):
        # Assume we're not interested in this element's content
        self.current_string_id = None
        self.current_content = u''

        if name == ROOT_ELEMENT:
            self.locale = hal_locale_to_bcp47(attrs[LOCALE_ATTRIBUTE])
        elif name == STRING_ELEMENT:
            self.current_string_id = attrs[STRING_ID_ATTRIBUTE]

    def characters(self, content):
        if self.current_string_id:
            # Add it to this string ID's content
            self.current_content += content

    def endElement(self, name):
        if (name == STRING_ELEMENT):
            self.strings[self.current_string_id] = self.current_content

def load_hal_file(filename):
    """Load the specify HAL file and return a (locale, strings) tuple"""
    handler = HALSaxHandler()

    sax.parse(filename, handler)

    return (handler.locale, handler.strings)

def load_hal_translations(directory):
    """Load all the HAL translations for a given directory"""
    filenames = os.listdir(directory)

    # Dictonary of locale => strings
    locale_dict = {}

    for filename in filenames:
        if filename.lower().endswith(".xml") and filename.startswith("Ebisu"):
            # Looks like a locale file
            full_path = os.path.join(directory, filename) 
            (locale, strings) = load_hal_file(full_path) 

            locale_dict[locale] = strings

    return locale_dict
