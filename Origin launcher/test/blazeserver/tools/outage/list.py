#!/usr/bin/env python
"""
List scheduled outages
"""
import config
import time
from redirector import Redirector
import sys
from optparse import OptionParser
import logging
from errors import NetworkError
import utils

logging.basicConfig(level=logging.INFO)

parser = OptionParser()
parser.add_option("-e", "--env", dest="env",
                    help="Environment to connect to [DEV, STEST, SCERT, PROD]")
parser.add_option("-i", "--instances", dest="instances",
                    help="Blaze IDs to query for.")
(options, args) = parser.parse_args()

if not options.env or not options.env.upper() in config.environment:
    logging.error('Environment %s not found.' % options.env)
    sys.exit(0);

if not options.instances:
    logging.error('You must specify Blaze IDs')
    sys.exit(0);
    
env = config.environment[options.env.upper()]
rdirurl = env['redirector']
rdir = Redirector(rdirurl)
rdir.initServerList()

hosts = []
    
blazeids = options.instances.split(',')
for b in blazeids:
    host = rdir.find_http_xml_server(b)
    if host is None:
        logging.error('Could not find blaze instance %s' % b)
    else:
        hosts.append(host)

printer = utils.TablePrinter(['Blaze ID', 'Start', 'Duration', 'Downtime Type', 'Message Type'])
for b in blazeids:
    try:
        outages = rdir.list_downtime(b)
        for o in outages:
            printer.addrow([o['servicename'], time.strftime('%Y-%m-%d %H:%M:%S %Z',time.gmtime(int(o['start']))), utils.secs_to_fractions(int(o['duration'])), config.DOWN_EMUM[int(o['downtimetype'])], o['messagetype']])
    except NetworkError, ne:
        logging.error(ne)

printer.print_table()

