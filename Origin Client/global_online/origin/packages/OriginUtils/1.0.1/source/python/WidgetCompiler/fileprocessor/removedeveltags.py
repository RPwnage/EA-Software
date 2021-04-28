import os
import sys

from htmlwalker import HTMLWalker
from fileprocessor.base import FileProcessor
from datasource import StringDataSource

class RemoveDevelopmentTagsProcessor(FileProcessor):
    """File processor to remove development-only HTML tags"""
    def process(self, package_path, data_source):
        base_name = os.path.basename(package_path).lower()

        # TODO: Support XHTML
        if not (base_name.endswith(".html") or base_name.endswith(".htm")):
            return [(package_path, data_source)]
        
        devel_walker = DevelopmentNodeWalker()
        processed_source = devel_walker.walk_source(data_source)

        return [(package_path, processed_source)]

class DevelopmentNodeWalker(HTMLWalker):
    """Process an HTML document to remove development-only nodes"""
    def _visit_node(self, node):
        """Remove nodes marked development-only""" 
        if node.type == 5 and "data-devel-only" in node.attributes:
            node.parent.removeChild(node)
        else:
            self._visit_children(node)
