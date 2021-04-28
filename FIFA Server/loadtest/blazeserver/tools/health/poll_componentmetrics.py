#!/bin/env python

"""
Usage: python poll_componentmetrics.py --host <ip>:<port> --component <component> --command <command> --interval <interval>

NOTE: for unbuffered output use python -u.  This is necessary in the LT environment
if you want to see the output with tee or tail on redirect.

* host -- IP:port combination of Blaze HTTP XML interface
* component -- Name of the component to pull health information for, if empty poll all components
* command -- Name of the command to pull health information for, if empty poll all commands
* interval -- interval seconds to run.

Output:
datetime,key,...,keyN
<timestamp>,value,...,valueN 
"""
from optparse import OptionParser
import urllib2
from xml.dom.minidom import parse, parseString, Node
import time

URL_TEMPLATE = 'http://%s/blazecontroller/getComponentMetrics'

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

    req = urllib2.Request(url)
    res = urllib2.urlopen(req).read()
    dom = parseString(res)
    elements = dom.getElementsByTagName('commandmetricinfo')

    for child in elements[0].childNodes:
    if child.nodeType == Node.ELEMENT_NODE:
        headers.append(child.nodeName)

    now = time.strftime('%m/%d/%Y %H:%M:%S', time.localtime())
    for e in elements:
        c = getNodeValue(e, 'componentname')
        if options.component == c:
            value = [now]
            for child in e.childNodes:
                if child.nodeType == Node.ELEMENT_NODE:
                    value.append(child.firstChild.nodeValue)
            values.append(value)
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

for value in values:
    print ','.join(value)

while True:
    time.sleep(interval)
    (headers,values) = getValues(url)
    for value in values:
        print ','.join(value)

