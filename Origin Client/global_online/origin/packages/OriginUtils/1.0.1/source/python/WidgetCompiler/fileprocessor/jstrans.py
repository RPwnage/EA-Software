import os
import sys
import re
import json
from string import Template

from html5lib import treebuilders

from htmlwalker import HTMLWalker
from fileprocessor.base import FileProcessor
from datasource import StringDataSource
from localization import localized_package_path 

# The name of the per-locale strings database
# This uses widget localization to pick the correct file at runtime
LOCALIZED_STRINGS_FILE = "origin/scripts/strings.js"

# The locale's strings we use as a last resort
# These string will also be used if the widget is loaded outside of a W3C 
# widget-aware user agent (ie a browser)
FALLBACK_LOCALE = 'en-us'

# Name of the master translator file
# This contains the fallback strings and tr() implementation
TRANSLATOR_FILE = "origin/scripts/translator.js"

def _add_javascript_include(node, source):
    """Add a JavaScript include to the passed node"""
    scriptEl = treebuilders.simpletree.Element("script")
    scriptEl.attributes["type"] = "text/javascript"
    scriptEl.attributes["src"] = source
    node.childNodes.insert(0, scriptEl)

def _extract_source_strings(source):
    """Extract a set of strings marked for translation from JavaScript source"""

    # Strip comments
    source = re.sub(r'//.*', '', source)
    # Hack for Python 2.6
    cstyleRe = re.compile(r'/\*.*?\*/', re.DOTALL)
    source = re.sub(cstyleRe, '', source)

    # Our base regex pattern
    double_quote_pattern = r'\btr\(\s*"(([^"\\]|\\.)*)"\s*\)'
    single_quote_pattern = double_quote_pattern.replace('"', "'")
    
    raw_strings = []

    for pattern in [single_quote_pattern, double_quote_pattern]:
        for match in re.finditer(pattern, source):
            raw_strings.append(match.group(1))

    strings = [raw_string.decode("string_escape") for raw_string in raw_strings]

    return set(strings)

class JSTranslationProcessor(FileProcessor):
    """File processor to support JavaScript translations"""

    def __init__(self, translations, string_notifier):
        self.translations = translations
        self.extracted_strings = set()    
        self.string_notifier = string_notifier

    def process(self, package_path, data_source):
        base_name = os.path.basename(package_path).lower()

        if base_name.endswith(".html") or base_name.endswith(".htm"):
            # Inject the script tag
            injector = ScriptInjector()
            injected_source = injector.walk_source(data_source)

            # Return the modified HTML
            return [(package_path, injected_source)]

        if base_name.endswith(".js"):
            # Extract all the strings
            source = data_source.read()
            self.extracted_strings |= _extract_source_strings(source)

        # Return
        return [(package_path, data_source)]

    def support_files(self):
        """Return a list of JavaScript translation files"""
        output_files = {}

        # Load our JS templates
        trans_template = Template(open(sys.path[0] + "/templates/translator.js", 'rb').read())
        strings_template = Template(open(sys.path[0] + "/templates/strings.js", 'rb').read())

        # Figure out which strings are actually used by JS
        used_translations = {}

        for locale, all_strings in self.translations.items():
            used_translations[locale] = {}
            for string_id in self.extracted_strings:
                if string_id in all_strings:
                    used_translations[locale][string_id] = all_strings[string_id]
                elif locale == FALLBACK_LOCALE:
                    self.string_notifier.string_missing(string_id)

        # Find our fallback locale strings
        try:
            fallback_strings = used_translations[FALLBACK_LOCALE]
        except:
            # If the fallback locale isn't there use an empty string table
            fallback_strings = {}

        for locale, strings in used_translations.items():
            # Remove the strings identical to the fallback strings to save space
            locale_strings = dict((k, strings[k]) for k 
                in strings
                if k in fallback_strings 
                    and fallback_strings[k] != strings[k])

            # Expand our template
            js_source = strings_template.substitute(strings=json.dumps(locale_strings))

            # Write out the strings file
            localized_path = localized_package_path(LOCALIZED_STRINGS_FILE, locale)
            output_files[localized_path] = StringDataSource(js_source)

        # Write out a stub strings file for browsers
        js_source = strings_template.substitute(strings=json.dumps({}))
        output_files[LOCALIZED_STRINGS_FILE] = StringDataSource(js_source)

        # Write out the translator
        js_source = trans_template.substitute(fallbackStrings=json.dumps(fallback_strings))
        output_files[TRANSLATOR_FILE] = StringDataSource(js_source)

        return output_files

class ScriptInjector(HTMLWalker):
    """Add our translation script to the document"""

    def _visit_node(self, node):
        """Append scripts to any head tag""" 
        if node.name == 'head':
            _add_javascript_include(node, LOCALIZED_STRINGS_FILE)
            _add_javascript_include(node, TRANSLATOR_FILE)
        else:
            self._visit_children(node)

