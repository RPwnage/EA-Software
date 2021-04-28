import os
import shutil
from packagewriter.base import PackageWriter
from xml.dom.minidom import getDOMImplementation

class OutputPathExistsError(Exception):
    pass

class QrcPackageWriter(PackageWriter):
    def __init__(self, output_path):
        self.output_path = output_path
        self.added_paths = []

        if os.path.exists(output_path):
            # We blow away the directory on error so make sure nothing
            # exists we might harm
            raise OutputPathExistsError

        # Be conservative and error out if the output's parent directory
        # doesn't exist
        os.mkdir(output_path)

        # We use our widget name in a couple of placs
        self.widget_name = os.path.basename(self.output_path)

    def add_file(self, package_path, data_source):
        """Add a data source to the Qt resource bundle"""
        dest_path = os.path.join(self.output_path, package_path)

        # Make sure the parent dir exists
        parent_dir = os.path.dirname(dest_path)

        try:
            os.makedirs(parent_dir)
        except(OSError):
            pass

        if data_source.file_path():
            # Do a high-level copy
            shutil.copyfile(data_source.file_path(), dest_path)
        else:
            # Write the source's content string
            with open(dest_path, 'wb') as f:
                f.write(data_source.read())

        # Track this for our qrc file
        self.added_paths.append(package_path)


    def finalize(self):
        """Write out the qrc file"""
        resource_path = os.path.join(self.output_path, self.widget_name + '.qrc')

        # This is where the widget appears in Qt's resource virtual filesystem
        resource_prefix = '/compiledWidgets/' + self.widget_name

        # Build the document
        impl = getDOMImplementation()

        doctype = impl.createDocumentType('RCC', None, None)
        doc = impl.createDocument(None, 'RCC', doctype)

        # Set up the RCC element
        rootEl = doc.documentElement
        rootEl.setAttribute('version', '1.0')

        qresourceEl = doc.createElement("qresource")
        qresourceEl.setAttribute('prefix', resource_prefix)
        rootEl.appendChild(qresourceEl)

        for package_path in self.added_paths:
            fileEl = doc.createElement("file")

            fileTextNode = doc.createTextNode(package_path)
            fileEl.appendChild(fileTextNode)

            qresourceEl.appendChild(fileEl)

        with open(resource_path, 'wb') as f:
            f.write(doc.toxml('utf-8'))

    def rollback(self):
        """Remove the partially written directory"""
        shutil.rmtree(self.output_path)
