#!/usr/bin/env python
"""
View message types set in redirector
"""
import config
from redirector import Redirector
import sys
from optparse import OptionParser
import logging
from errors import NetworkError
from blaze import Blaze
import utils

logging.basicConfig(level=logging.INFO)

parser = OptionParser()
parser.add_option("-e", "--env", dest="env",
                    help="Environment to connect to [DEV, STEST, SCERT, PROD]")
parser.add_option("-l", "--locale", dest="locale",default="en_US",
                    help="Locale of message")
(options, args) = parser.parse_args()

if not options.env or not options.env.upper() in config.environment:
    logging.error('Environment %s not found.' % options.env)
    sys.exit(0);
if not options.locale:
    logging.error('Locale must be specified.')
    sys.exit(0);

env = config.environment[options.env.upper()]
rdirurl = env['redirector']
rdir = Redirector(rdirurl)

messages = rdir.get_message_types(options.locale)

printer = utils.TablePrinter(['Message Type', 'Localized Message'])
for msg in messages:
    printer.addrow([msg['messagetype'], msg['localizedmessage']])
printer.print_table()
