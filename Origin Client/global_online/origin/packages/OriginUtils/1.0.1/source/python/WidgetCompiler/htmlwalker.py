from html5lib import HTMLParser, serializer, treewalkers

from datasource import StringDataSource

class HTMLWalker(object):
    """Base class for objects that need to iterate over HTML5 document trees"""

    def _visit_node(self, node):
        """Visits a DOM node. Must be overidden"""
        pass

    def _visit_children(self, nodes):
        for child in nodes.childNodes:
            self._visit_node(child)

    def walk_source(self, data_source):
        """Walk all the nodes in a given HTML data source"""
        parser = HTMLParser()
        simpletree = parser.parse(data_source.read())

        # Visit the root node
        self._visit_node(simpletree)

        # Create a simpletree walker
        walker = treewalkers.getTreeWalker("simpletree")
        stream = walker(simpletree)

        s = serializer.htmlserializer.HTMLSerializer(omit_optional_tags=False, quote_attr_values=True)

        output_string = ""
        for item in s.serialize(stream):
            output_string += item

        # Return a data source for the result
        return StringDataSource(output_string.encode('utf-8'))

