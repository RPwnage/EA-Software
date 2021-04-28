#!/usr/bin/env python
import argparse
import json
import re
import os
import sys

from lib.parsedepends import parse_dependfile
from lib.resolvedepends import resolve_depends
from lib.aggregatecomponents import aggregate_components

def fatal_exit(message):
    sys.stderr.write(message + "\n")
    sys.exit(-1)

def assert_is_dir(dir_descr, path):
    if not os.path.isdir(path):
        if not os.path.exists(path):
            # Simply doesn't exist
            fatal_exit(dir_descr + ' "' + path + '" does not exist')
        else:
            # Isn't a directory
            fatal_exit(dir_descr + ' "' + path + '" is not a directory')

def assert_exists(file_descr, path):
    if not os.path.exists(path):
        # Simply doesn't exist
        fatal_exit(file_descr + ' "' + path + '" does not exist')

# Create our arg parser
parser = argparse.ArgumentParser()

parser.add_argument("--components-dir",
                    required=True,
                    type=str,
                    help="root of a components directory to search")

parser.add_argument("--output-dir",
                    type=str,
                    default='.',
                    help="directory to output the final assets to")

parser.add_argument('--output-base-name',
                    type=str,
                    default='uitoolkit',
                    help="base name for output directories and filenames")

parser.add_argument('--dependfile',
                    type=str,
                    default=None,
                    help="file listing the required dependency names. use '-' to read from stdin")

parser.add_argument('--all-components',
                    action='store_const',
                    const=True,
                    default=False,
                    help="aggregate all components ignoring the dependfile")


args = parser.parse_args()

# Make sure our args are sane
assert_is_dir('Components directory', args.components_dir)
assert_is_dir('Output directory', args.output_dir)

if args.all_components:
    depend_set = set()

    if args.dependfile is not None:
        fatal_exit("--dependfile with --all-components does not make sense")

    # Include all components by iterating over the JSON files
    for filename in os.listdir(args.components_dir):
        if filename.endswith('.json'):
            depend_set.add(filename[:-5])
elif args.dependfile == '-':
    # Use stdin
    depend_set = parse_dependfile(sys.stdin)
else:
    if args.dependfile is None:
        # Look for dependfile.txt in the output dir
        args.dependfile = os.path.join(args.output_dir, 'dependfile.txt')

    assert_exists('dependfile', args.dependfile)

    # Open dependfile.txt
    with open(args.dependfile, 'r') as f:
        depend_set = parse_dependfile(f)

components = resolve_depends('dependfile', depend_set, args.components_dir)
aggregate_components(components, args.components_dir, args.output_dir, args.output_base_name)
