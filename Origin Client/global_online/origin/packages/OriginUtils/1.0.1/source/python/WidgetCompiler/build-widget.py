import argparse
import sys
import os.path

try:
    from Crypto.PublicKey import RSA
except:
    pass

import halloader
from compiler import WidgetCompiler
from packagewriter.zip import ZipPackageWriter
from packagewriter.qrc import QrcPackageWriter

parser = argparse.ArgumentParser(description='Build a localized Origin web widget from source')

# Location of our HAL XML files
parser.add_argument('--strings-dir',
        dest='strings_dir',
        required=True,
        help="directory containing HAL XML files")

parser.add_argument('--private-key',
        dest='private_key',
        required=False,
        help="PEM encoded private key to sign the widget with")

parser.add_argument('--passphrase',
        dest='passphrase',
        required=False,
        help="passphrase for the private key")

parser.add_argument('--keep-devel',
        '-d',
        default=False,
        action="store_true",
        dest='keep_devel',
        help="keep development tags")

parser.add_argument('--output-format',
        dest='output_format',
        choices=['wgt', 'qrc', 'auto'],
        required=False,
        default='auto',
        help="""'wgt' builds standalone widget package.
        'qrc' builds a Qt resource directory.
        'auto' will autodetect based on the output filename.""")

parser.add_argument('sourcedir',
        help="unpacked widget directory")

parser.add_argument('--output',
        '-o',
        dest='output',
        required=True,
        help="output file or directory name")

# Parse our arguments
args = parser.parse_args()

if args.output_format == 'auto':
    # Autodetect the output format`
    if (args.output.lower().endswith('.wgt') or 
            args.output.lower().endswith('.zip')):
        args.output_format = 'wgt'
    else:
        args.output_format = 'qrc'

# Load our translations
translations = halloader.load_hal_translations(args.strings_dir)

if (len(translations) == 0):
    sys.stderr.write("Warning: No translations loaded\n")

if args.private_key is not None:
    # Load our private key
    try:
        with open(args.private_key, 'r') as f:
            private_key = RSA.importKey(f.read(), args.passphrase)
    except NameError:
        sys.stderr.write("PyCrypto is required for cryptographic signature support\n")
        sys.exit(2)
else:
    private_key = None

# Make sure a configuration document exists in the source dir
# This file is required for all valid widget packages
config_doc_path = os.path.join(args.sourcedir, "config.xml")
if not os.path.exists(config_doc_path):
    sys.stderr.write("No configuration document found at " + config_doc_path + "\n")
    sys.exit(3)

# Build our package writer
if args.output_format == 'wgt':
    writer = ZipPackageWriter(args.output)
elif args.output_format == 'qrc':
    writer = QrcPackageWriter(args.output)

# Compile!
with writer:
    compiler = WidgetCompiler(\
            source_dir=args.sourcedir, \
            package_writer=writer, \
            translations=translations, \
            keep_devel=args.keep_devel, \
            private_key=private_key)

    compiler.compile()
