import os

from fileprocessor.base import FileFilter
from ignorefile import IgnoreFile

class IgnoreFileFilter(FileFilter):
    """FileFilter for ignoring paths specified in an ignore file"""

    def __init__(self, ignore_string):
        self.ignore_file = IgnoreFile(ignore_string)

    def filter_file(self, package_path, data_source):
        """Exclude unwanted files from the widget package"""
        return not self.ignore_file.path_ignored(package_path)
