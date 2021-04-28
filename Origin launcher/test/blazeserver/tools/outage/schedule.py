#!/usr/bin/env python
"""
Schedule an outage for blaze instance
"""
import config
from redirector import Redirector
import sys
from optparse import OptionParser
import logging
from errors import NetworkError, InputError, BlazeError
from blaze import Blaze
import time
import utils
import smtplib
from email.mime.text import MIMEText

logging.basicConfig(level=logging.INFO)

DOWNTYPES = {'PARTIAL_OUTAGE':0,
    'MAINTENANCE_EVENT':1}

parser = OptionParser()
parser.add_option("-e", "--env", dest="env",
                    help="Environment to connect to [DEV, STEST, SCERT, PROD]")
parser.add_option("-i", "--instance", dest="instance",
                    help="Blaze instance to schedule.")
parser.add_option("-a", "--admin", dest="admin",
                    help="Admin username.")
parser.add_option("-p", "--password", dest="password",
                    help="Admin password.")
parser.add_option("-m", "--msgtype", dest="msgtype",
                    help="Message type")
parser.add_option("-d", "--downtype", dest="downtype",
                    help="Downtime type")
parser.add_option("-s", "--start", dest="start",
                    help="Start time YYYY-MM-DD HH:MM:SS")
parser.add_option("-t", "--duration", dest="duration",
                    help="Duration")
(options, args) = parser.parse_args()

if not options.env or not options.env.upper() in config.environment:
    logging.error('Environment %s not found.' % options.env)
    sys.exit(0)
    
if not options.instance:
    logging.error('You must specify a Blaze ID')
    sys.exit(0)
    
if not options.admin:
    logging.error('You must specify an administrative user')
    sys.exit(0)

if not options.password:
    options.password = raw_input('Please specify a password for user %s ]> ' % options.admin)

if not options.msgtype:
    logging.error('You must specify an message type')
    sys.exit(0)

if not options.downtype or not options.downtype.upper() in DOWNTYPES:
    logging.error('You must specify an downtime type (%s)' % ','.join(DOWNTYPES.keys()))
    sys.exit(0)

if not options.start:
    logging.error('You must specify an start time')
    sys.exit(0)

if not options.duration:
    logging.error('You must specify an duration')
    sys.exit(0)

try:
    start = int(time.mktime(time.strptime(options.start, '%Y-%m-%d %H:%M:%S')))
except:
    logging.error('Invalid start time (%s) format, should be in format YYYY-MM-DD HH:MM:SS' % options.start)
    sys.exit(0)

try:
    dur = utils.time_to_secs(options.duration)
except:
    logging.error('Invalid duration %s' % options.duration)
    sys.exit(0)

env = config.environment[options.env.upper()]
rdirurl = env['redirector']
rdir = Redirector(rdirurl)
rdir.initServerList()
    
host = rdir.find_http_xml_server(options.instance)
if host is None:
    logging.error('Could not find blaze instance %s' % host)
    sys.exit(0)

blaze = Blaze(env['redirector'])
try:
    blaze.auth(options.admin, options.password)
    rdir.schedule_downtime(blaze.get_token(), options.instance, options.msgtype, start, dur, options.downtype.upper())
    
    sender = config.sender
    recp = config.mailinglist
    recp.append(options.admin)
    msgstr = 'User %s has scheduled an outage for %s at %s' % (options.admin, options.instance, options.start)
    msgsub = 'Scheduled Outage %s at %s' % (options.instance, options.start)
    msg = MIMEText(msgstr)
    msg['Subject'] = msgsub
    msg['From'] = sender
    msg['To'] = recp[0]
    mail = smtplib.SMTP(host=config.mailhost)
    mail.sendmail(sender, recp, msg.as_string())
    
    print 'SUCCESS'
except NetworkError, ne:
    logging.error(ne)
except BlazeError, be:
    logging.error(be)
