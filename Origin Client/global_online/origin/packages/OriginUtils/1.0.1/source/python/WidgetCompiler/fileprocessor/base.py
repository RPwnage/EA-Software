class FileProcessor(object):
    """Base class for file processor

    These are our compiler passes operating on file units"""
    def process(self, package_path, data_source):
        """Process an input file returning a list of output files
        
        The return value is a dict of package_path => data_source to write to
        the package. An empty dict will omit the file from the package 
        entirely."""
        return {package_path: source}

    def support_files(self):
        """Return a list of support files to add the the package
        
        Return as dict of package_path => data_source"""

        return {}

class FileFilter(FileProcessor):
    """Base class for file filters

    Filters are simplified procesors that can either return a modified file  
    or reject a file entirely"""
    
    def filter_file(self, package_path, data_source):
        """Filter a single file
        
        Return either a new data_source for the file's contents, True to accept
        the file or False to remove it from the package"""
        return True

    def process(self, package_path, data_source):
        result = self.filter_file(package_path, data_source)
        
        if result is True:
            # File should be passed through unmodified
            return {package_path: data_source}
        elif result is False:
            # File should be removed from the package
            return {}
        else:
            # File should remain with new content
            return {package_path: result}
