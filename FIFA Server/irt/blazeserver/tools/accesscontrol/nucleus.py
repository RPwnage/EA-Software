import urllib2
from xml.dom.minidom import parse, parseString
import logging
from errors import NetworkError

class Nucleus:
    def __init__(self, url):
        self.url = url
        
    def get_nucleus_id(self, user):
        if user is None:
            return None
        
        url = '%s/1/users?email=%s' % (self.url, user)
        req = urllib2.Request(url)
        req.add_header('Nucleus-RequestorId', 'GOS-Blaze-Tools')
        try:
            res = urllib2.urlopen(req).read()
            dom = parseString(res)
            elements = dom.getElementsByTagName('userUri')
            if len(elements) > 0:
                return elements[0].childNodes[0].nodeValue.split('/')[2]
            else:
                return None
        except:
            raise NetworkError('Failed to communicate to host %s' % self.url)
            
    def get_nucleus_name(self, userid):
        if userid is None:
            return None
        url = '%s/1/users/%s' % (self.url, userid)
        req = urllib2.Request(url)
        req.add_header('Nucleus-RequestorId', 'GOS-Blaze-Tools')
        try:
            res = urllib2.urlopen(req).read()
            dom = parseString(res)
            elements = dom.getElementsByTagName('email')
            if len(elements) > 0:
                return elements[0].childNodes[0].nodeValue
            else:
                return None
        except:
            raise NetworkError('Failed to communicate to host %s' % self.url)
