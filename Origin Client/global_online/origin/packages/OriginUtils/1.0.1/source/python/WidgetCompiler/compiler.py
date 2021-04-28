import os

from datasource import FileDataSource
from missingstring import MissingStringNotifier
from signature import PackageCryptographicSignature, SIGNATURE_PACKAGE_PATH

from fileprocessor.ignore import IgnoreFileFilter
from fileprocessor.htmltrans import HTMLTranslationProcessor
from fileprocessor.jstrans import JSTranslationProcessor
from fileprocessor.removedeveltags import RemoveDevelopmentTagsProcessor

class WidgetCompiler(object):
    def __init__(self, source_dir, package_writer, translations, keep_devel=False, private_key=None):
        """Create a new widget compiler"""
        self.source_dir = source_dir
        self.package_writer = package_writer

        self.string_notifier = MissingStringNotifier()
        
        # Cryptographic signing must be last or else we might sign something
        # other than the file product
        if private_key is not None:
            self.signature = PackageCryptographicSignature(private_key)
        else:
            self.signature = None
        
        # Add our processors
        self.processors = []

        try:
            with open(os.path.join(source_dir, '.wgtignore'), 'r') as f:
                self.processors.append(IgnoreFileFilter(f.read()))
        except IOError:
            # No .wgtignore
            pass

        # Put this before HTML translation so we only have to do it once on
        # the base document
        if not keep_devel:
            self.processors.append(RemoveDevelopmentTagsProcessor())
        self.processors.append(HTMLTranslationProcessor(translations, self.string_notifier))
        self.processors.append(JSTranslationProcessor(translations, self.string_notifier))

    def _package_path(self, filesystem_path):
        """Return the package path for a filesystem path"""
        return os.path.relpath(filesystem_path, self.source_dir).replace(os.sep, '/')

    def _write_files(self, files):
        """Writes a package_path => data_source dict to the package writer""" 
        for package_path, data_source in files.items():
            if self.signature is not None:
                # Sign the file
                self.signature.add_signed_file(package_path, data_source)

            self.package_writer.add_file(package_path, data_source) 

    def compile(self):
        """Compile a widget in to our package writer"""
        # Process our package files
        # Specify the path as Unicode so we Unicode paths back
        for root, dirs, files in os.walk(unicode(self.source_dir)):
            for filename in files:
                full_path = os.path.join(root, filename)
                self._compile_file(full_path)

        # Add on any support files
        for processor in self.processors:
            self._write_files(processor.support_files())

        if self.signature is not None:
            # Add our signature
            self.package_writer.add_file(SIGNATURE_PACKAGE_PATH, self.signature.package_signature())

        # Warn about any strings we couldn't lookup
        self.string_notifier.output_missing_strings()

    def _compile_file(self, filesystem_path):
        """Compile an individual file"""
        package_path = self._package_path(filesystem_path)
        data_source = FileDataSource(filesystem_path)

        # Let the user know what we're working on
        print package_path

        # This is the list of files we start with
        input_files = {package_path: data_source}

        # Run our processors
        for processor in self.processors:
            output_files = {}

            # Run for each input file we have
            # We start out with only one input file but a filter can produce
            # multiple output files for eg localization
            for package_path, input_source in input_files.items():
                output_files.update(processor.process(package_path, input_source))

            # Our output from this processor is the input for the next
            input_files = output_files

        # Write the output files out
        self._write_files(output_files)
