class PackageWriter(object):
    """Base class for package writers"""
    
    def add_file(self, package_path, data_source):
        """Adds a file with the passed data source
        
        Subclasses must override this"""
        pass


    def finalize(self):
        """Finalize the output package upon successful completion

        No additional methods will be called after finalize"""
        pass

    def rollback(self):
        """Rollback the output package upon error

        All output files should be cleaned up if possible"""
        pass

    def __enter__(self):
        """Stub context manager handler"""
        pass

    def __exit__(self, type, value, traceback):
        """Context manager handler

        Call finialize() on success and rollback() on error"""
        if type is None:
            self.finalize()
        else:
            self.rollback()

        # Never eat the exception
        return False
