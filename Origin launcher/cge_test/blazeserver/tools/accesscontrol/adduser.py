#!/usr/bin/env python
"""
Add a user to a Blaze permission group.
"""
import config
from redirector import Redirector
from nucleus import Nucleus
from accesscontrol import BlazeAccessControl
import sys
from optparse import OptionParser
import logging
from errors import NetworkError, AccessControlError

logging.basicConfig(level=logging.INFO)

parser = OptionParser()
parser.add_option("-e", "--env", dest="env",
                    help="Environment to connect to [DEV, STEST, SCERT, PROD]")
parser.add_option("-i", "--instances", dest="instances",
                    help="Blaze IDs to add users to.")
parser.add_option("-b", "--hosts", dest="hosts",
                    help="Specific Blaze hosts to add users to.")
parser.add_option("-u", "--users", dest="users",
                    help="Users to add.")
parser.add_option("-g", "--group", dest="group",
                    help="Group to add to.")
parser.add_option("-a", "--admin", dest="admin",
                    help="Admin username.")
parser.add_option("-p", "--password", dest="password",
                    help="Admin password.")
parser.add_option("-c", "--clienttype", dest="clienttype",
                    help="Client Type [CONSOLE_USER, WEB_ACCESS_LAYER, DEDICATED_SERVER].")
(options, args) = parser.parse_args()

if not options.env or not options.env.upper() in config.environment:
    logging.error('Environment %s not found.' % options.env)
    sys.exit(0);

if not options.users:
    logging.error('Users must be defined')
    sys.exit(0);
    
if not options.instances and not options.hosts:
    logging.error('You must specify Blaze IDs or specific host:port combinations')
    sys.exit(0);
    
if not options.group:
    logging.error('You must specify Blaze group')
    sys.exit(0);
    
if not options.admin:
    logging.error('You must specify an administrative user')
    sys.exit(0);

if not options.password:
    options.password = raw_input('Please specify a password for user %s ]> ' % options.admin)

if not options.clienttype or not options.clienttype.upper() in config.clienttypes:
    logging.error('Client type %s not found.' % options.clienttype)
    sys.exit(0);

env = config.environment[options.env.upper()]
clienttype = config.clienttypes[options.clienttype.upper()]
users = options.users.split(',')
nuke = Nucleus(env['nucleus'])
uids = []
for user in users:
    uid = nuke.get_nucleus_id(user)
    if uid is None:
        logging.error('Could not find user %s' % user)
    else:
        uids.append(uid)

hosts = []
if options.instances:
    rdirurl = env['redirector']
    rdir = Redirector(rdirurl)
    rdir.initServerList()

    if options.instances == '*':
        for server in rdir.servers.keys():
            host = rdir.find_http_xml_server(server)
            if host is not None:
                hosts.append(host)
    else:
        blazeids = options.instances.split(',')
        for b in blazeids:
            host = rdir.find_http_xml_server(b)
            if host is None:
                logging.error('Could not find blaze instance %s' % b)
            else:
                hosts.append(host)
else:
    hosts = options.hosts.split(',')

for host in hosts:
    blaze = BlazeAccessControl(host)
    try:
        blaze.auth(options.admin, options.password)
        if blaze.connected():
            for uid in range(0,len(uids)):
                try:
                    blaze.grant_user(uids[uid], options.group, clienttype)
                    logging.info('Added user %s (%s) to group %s on host %s' % (users[uid], uids[uid], options.group, host))
                except AccessControlError, ae:
                    logging.error(ae)
    except NetworkError, ne:
        logging.error(ne)
