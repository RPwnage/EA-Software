#!/bin/env python

"""
Usage: python poll_health.py --host <ip>:<port> --component <component> --interval <interval>

NOTE: for unbuffered output use python -u.  This is necessary in the LT environment
if you want to see the output with tee or tail on redirect.

* host -- IP:port combination of Blaze HTTP XML interface
* component -- Which component to pull health information for
* interval -- interval seconds to run.

Output:
datetime,key,...,keyN
<timestamp>,value,...,valueN 
"""
from optparse import OptionParser
import urllib2
from xml.dom.minidom import parse, parseString
import time

URL_TEMPLATE = 'http://%s/blazecontroller/getStatus'

def getNodeValue(node, tag):
    e = node.getElementsByTagName(tag)
    if len(e) > 0 and len(e[0].childNodes[0].nodeValue) > 0:
        return e[0].childNodes[0].nodeValue
    else:
        return None

def getElementValue(node):
    if len(node.childNodes) > 0 and len(node.childNodes[0].nodeValue) > 0:
        return node.childNodes[0].nodeValue
    else:
        return ''


def getValues(url):
    headers = ['datetime']
    values = []

    values.append(time.strftime('%m/%d/%Y %H:%M:%S', time.localtime()))
    req = urllib2.Request(url)
    res = urllib2.urlopen(req).read()
    dom = parseString(res)
    elements = dom.getElementsByTagName('componentstatus')
    for e in elements:
        c = getNodeValue(e, 'componentname')
        if options.component == c:
            info = e.getElementsByTagName('entry')
            for i in info:
                headers.append(i.getAttribute('key'))
                values.append(getElementValue(i))
    return (headers, values)

parser = OptionParser()
parser.add_option("-s", "--host", dest="host",
                    help="Blaze host to query", metavar="HOST")
parser.add_option("-c", "--component", dest="component",
                    help="Component to retrieve health from", metavar="COMP")
parser.add_option("-i", "--interval", dest="interval", default='60',
                    help="Interval (secs) to poll", metavar="INTERVAL")
(options, args) = parser.parse_args()

url = URL_TEMPLATE % options.host
interval = int(options.interval)

(headers,values) = getValues(url)

print ','.join(headers)
print ','.join(values)

while True:
    time.sleep(interval)
    (headers,values) = getValues(url)
    print ','.join(values)

