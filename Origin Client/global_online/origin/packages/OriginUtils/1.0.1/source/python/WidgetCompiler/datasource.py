class DataSource(object):
    """Base class for data sources

    All DataSource subclasses must implement read()"""

    def read(self):
        """Return the entire content of the data source"""
        pass

    def file_path(self):
        """Return the path to the file backing this source"""
        return None

class StringDataSource(DataSource):
    """Data source backed by a string"""
    def __init__(self, string):
        self.string = string

    def read(self):
        """Return the content of the string we were constructed with"""
        return self.string


class FileDataSource(DataSource):
    """Data source backed by a file"""
    def __init__(self, path):
        self.path = path

    def file_path(self):
        return self.path

    def read(self):
        return open(self.path, 'rb').read()
