import zipfile
import os
import time

from packagewriter.base import PackageWriter

class ZipPackageWriter(PackageWriter):
    def __init__(self, output_filename):
        self.output_filename = output_filename
        self.outfile = zipfile.ZipFile(output_filename, 'w', zipfile.ZIP_DEFLATED)

    def _string_file_info(self, package_path):
        """Return a ZipInfo instance for the given package path"""
        info = zipfile.ZipInfo(filename=package_path,
                               date_time=time.localtime(time.time())[:6])

        # Regular file with some normal permission. No execute bit
        info.external_attr = 0o100644 << 16L

        return info

    def add_file(self, package_path, data_source):
        """Add a data source to the ZIP archive"""

        if data_source.file_path():
            # Do a streaming write of the file
            self.outfile.write(data_source.file_path(), package_path)
        else:
            # Write the source's content string
            info = self._string_file_info(package_path)
            self.outfile.writestr(info, data_source.read())

    def finalize(self):
        """Close the ZIP file"""
        self.outfile.close()

    def rollback(self):
        """Close the ZIP file and remove it"""
        self.outfile.close()
        os.unlink(self.output_filename)

