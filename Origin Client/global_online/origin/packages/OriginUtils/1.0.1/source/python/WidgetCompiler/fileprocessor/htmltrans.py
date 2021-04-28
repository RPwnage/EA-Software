import os
import sys

from htmlwalker import HTMLWalker
from fileprocessor.base import FileProcessor
from datasource import StringDataSource
from localization import localized_package_path 

STRING_ID_ATTRIBUTE = "data-string-id"

class HTMLTranslationProcessor(FileProcessor):
    """File processor to support static translations of HTML documents"""
    def __init__(self, translations, string_notifier):
        self.translations = translations
        self.string_notifier = string_notifier

    def process(self, package_path, data_source):
        base_name = os.path.basename(package_path).lower()

        # TODO: Support XHTML
        if not (base_name.endswith(".html") or base_name.endswith(".htm")):
            return [(package_path, data_source)]

        # Keep the original as a fallback
        output_files = {package_path: data_source}

        for locale, strings in self.translations.items():
            # Translate it
            translator = NodeTranslator(locale, strings, self.string_notifier)
            translated_source = translator.walk_source(data_source)

            # Add the localized file on
            localized_path = localized_package_path(package_path, locale)
            output_files[localized_path] = translated_source

        return output_files

class NodeTranslator(HTMLWalker):
    """Translate marked document nodes using a string translation table"""

    def __init__(self, locale, strings, string_notifier):
        self.locale = locale
        self.strings = strings
        self.string_notifier = string_notifier

        HTMLWalker.__init__(self)

    def _visit_node(self, node):
        """Translate a simpletree node""" 
        if node.name == 'html' and \
                node.type == 5 and \
                node.parent.parent is None:
            # Mark our locale in the lang attribute
            node.attributes["lang"] = self.locale

        try:
            # Attempt to translate the element by replace all its children a
            # single text node containing the translated test
            string_id = node.attributes[STRING_ID_ATTRIBUTE]

            if string_id in self.strings:
                # Remove all children
                node.childNodes = []

                # Add the translation on
                node.insertText(self.strings[string_id])

                # Remove the attribute
                del node.attributes[STRING_ID_ATTRIBUTE]
            else:
                self.string_notifier.string_missing(string_id)

        except (AttributeError, KeyError) as e:
            # No data-string-id, parse recursively
            self._visit_children(node)
