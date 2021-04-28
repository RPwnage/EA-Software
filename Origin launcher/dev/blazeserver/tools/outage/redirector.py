#!/usr/bin/env python
import socket, struct
import urllib2
from xml.dom.minidom import parse, parseString
import re
from errors import NetworkError
import utils

class BlazeServer:
    def __init__(self, name):
        self.name = name
        self.master = None
        self.slaves = []

class ServerInstance:
    def __init__(self):
        self.endpoints = []
    
class Endpoint:
    def __init__(self):
        self.channel = None
        self.decoder = None
        self.encoder = None
        self.protocol = None
        self.serveraddresses = []
        
    def __str__(self):
        return '%s %s %s %s' % (self.channel, self.decoder, self.encoder, self.protocol)

class ServerAddress:
    def __init__(self, type, host, ip, port):
        self.type = type
        self.host = host
        self.ip = ip
        self.port = port
    
class BlazeMasterServer(ServerInstance):
    pass
    
class BlazeSlaveServer(ServerInstance):
    pass

class Redirector:
    def __init__(self, url):
        self.url = url
        self.servers = {}
        
    def get_data(self, url):
        data = ''
        try:
            req = urllib2.Request(url)
            data = urllib2.urlopen(req).read()
        except:
            raise NetworkError('Failed to communicate with redirector at %s' % self.url)
            
        dom = parseString(data)
        return dom
    
    @staticmethod
    def is_legal_xml_char(uchar):
        if len(uchar) == 1:
            return not \
               (
               (uchar >= u'\u0000' and uchar <= u'\u0008') or \
               (uchar >= u'\u000b' and uchar <= u'\u000c') or \
               (uchar >= u'\u000e' and uchar <= u'\u001f') or \
               # always illegal as single chars
               (uchar >= unichr(0xd800) and uchar <= unichr(0xdfff)) or \
               (uchar >= u'\ufffe' and uchar <= u'\uffff')
               )
        elif len(uchar) == 2:
            return True
        else:
            raise Exception("Must pass a single character to is_legal_xml_char")
    
    @staticmethod
    def get_child_node_value(node, tagname):
        e = node.getElementsByTagName(tagname)
        if len(e) > 0 and len(e[0].childNodes) > 0:
            return e[0].childNodes[0].nodeValue
        return None
    
    @staticmethod    
    def get_element_value(node):
        if len(node.childNodes) > 0 and len(node.childNodes[0].nodeValue) > 0:
                return node.childNodes[0].nodeValue
        else:
                return ''

    @staticmethod
    def fill_serveraddress_info(nodes, endpoint):
        list = []
        for e in nodes:
            type = Redirector.get_child_node_value(e, 'type')
            valu = e.getElementsByTagName("valu")
            if len(valu) > 0:
                if type != "2":
                    host = Redirector.get_child_node_value(valu[0], 'hostname')
                    ip = Redirector.get_child_node_value(valu[0], 'ip')
                    port = Redirector.get_child_node_value(valu[0], 'port')
                    endpoint.serveraddresses.append(ServerAddress(type, host, ip, port))
                else:
                    host = Redirector.get_child_node_value(valu[0], 'sitename')
                    ip = Redirector.get_child_node_value(valu[0], 'serviceid')
                    port = Redirector.get_child_node_value(valu[0], 'port')
                    endpoint.serveraddresses.append(ServerAddress(type, host, ip, port))
    
    @staticmethod        
    def fill_endpoint_info(nodes, server, tag):
        list = []
        for e in nodes:
            endpoint = Endpoint()
            endpoint.channel = Redirector.get_child_node_value(e, 'channel')
            endpoint.decoder = Redirector.get_child_node_value(e, 'decoder')
            endpoint.encoder = Redirector.get_child_node_value(e, 'encoder')
            endpoint.protocol = Redirector.get_child_node_value(e, 'protocol')
            
            serveraddress = e.getElementsByTagName('serveraddressinfo')
            if len(serveraddress) > 0:
                Redirector.fill_serveraddress_info(serveraddress, endpoint)
                
            server.endpoints.append(endpoint)

    @staticmethod    
    def fill_instance_info(nodes, server):
        for e in nodes:
            slave = BlazeSlaveServer()
            server.slaves.append(slave)
            endpoints = e.getElementsByTagName('serverendpointinfo')
            if len(endpoints) > 0:
                Redirector.fill_endpoint_info(endpoints, slave, 'endpoints')

    def initServerList(self):
        data = ''
        try:
            url = 'http://%s/redirector/getServerList' % self.url
            req = urllib2.Request(url)
            raw = urllib2.urlopen(req).read()
            for i, c in enumerate(raw):
                try:
                    if Redirector.is_legal_xml_char(c):    
                        data += c
                    else:
                        print 'x'
                except Exception, e:
                    pass
        except:
            raise NetworkError('Failed to communicate with redirector at %s' % self.url)
        
        dom = parseString(data)
        elements = dom.getElementsByTagName('serverinfodata')   
        for e in elements:
            server = BlazeServer(Redirector.get_child_node_value(e, 'name'))
            if server.name is not None:
                self.servers[server.name.lower()] = server
                mst = e.getElementsByTagName('masterinstance')
                if len(mst) > 1:
                    print "%s has more than one master %d" % (server.name, len(mst))
                if len(mst) > 0:
                    master = BlazeMasterServer()
                    server.master = master
                    endpoints = mst[0].getElementsByTagName('serverendpointinfo')
                    if len(endpoints) > 0:
                        Redirector.fill_endpoint_info(endpoints, master, 'masterendpoints')
                        
                slvs = e.getElementsByTagName('serverinstance')
                Redirector.fill_instance_info(slvs, server)
            
    def find_http_xml_server(self, blaze_id):
        host = None
        if blaze_id.lower() in self.servers:
            server = self.servers[blaze_id.lower()]
            for slave in server.slaves:
                for endpoint in slave.endpoints:
                    if endpoint.channel.upper() == 'TCP' and endpoint.decoder.upper() == 'HTTP' and endpoint.encoder.upper() == 'XML2' and endpoint.protocol.upper() == 'HTTPXML':
                        for address in endpoint.serveraddresses:
                            if address.type == '0':
                                host = '%s:%s' % (socket.inet_ntoa(struct.pack('!L',int(address.ip))), address.port)
        return host

    def find_servers_by_pattern(self, in_name):
        names = []
        for key in self.servers:
            if key[0:len(in_name)] == in_name:
                names.append(key);
        return names

    def get_message_types(self, locale):
        data = ''
        url = 'http://%s/redirector/getDowntimeMessageTypes?loc=%s' % (self.url, locale)
        try:
            req = urllib2.Request(url)
            data = urllib2.urlopen(req).read()
        except:
            raise NetworkError('Failed to communicate with redirector at %s' % self.url)
        
        dom = parseString(data)
        elements = dom.getElementsByTagName('downtimemessagedata')
        messages = []
        for e in elements:
            msg = {}
            msg['messagetype'] = Redirector.get_child_node_value(e, 'messagetype')
            msg['localizedmessage'] = Redirector.get_child_node_value(e, 'localizedmessage')
            messages.append(msg)
        return messages

    def cancel_downtime(self, token, blazeid, msg, start, dur, type):
        data = ''
        url = 'http://%s/redirector/cancelServerDowntime/%s?name=%s&msg=%s&strt=%i&dur=%i&dtyp=%s' % (self.url, token, blazeid, msg, start, dur, type)
        try:
            req = urllib2.Request(url)
            data = urllib2.urlopen(req).read()
        except:
            raise NetworkError('Failed to communicate with redirector at %s' % self.url)
        if data and len(data) > 0:
            dom = parseString(data)
            utils.get_blaze_error(dom)
    
    def schedule_downtime(self, token, blazeid, msg, start, dur, type):
        data = ''
        url = 'http://%s/redirector/scheduleServerDowntime/%s?name=%s&msg=%s&strt=%i&dur=%i&dtyp=%s' % (self.url, token, blazeid, msg, start, dur, type)
        try:
            req = urllib2.Request(url)
            data = urllib2.urlopen(req).read()
        except:
            raise NetworkError('Failed to communicate with redirector at %s' % self.url)
            
        dom = parseString(data)
        utils.get_blaze_error(dom)
        
    def list_downtime(self, blazeid):
        data = ''
        outages = []
        url = 'http://%s/redirector/getDowntimeList?name=%s' % (self.url, blazeid)
        dom = self.get_data(url)
        elements = dom.getElementsByTagName('downtimedata')
        for e in elements:
            outage = {}
            for child in e.childNodes:
                if child.nodeType == child.ELEMENT_NODE:
                    outage[child.tagName] = Redirector.get_element_value(child)
            outages.append(outage)
        return outages