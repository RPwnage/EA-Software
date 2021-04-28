#!/usr/bin/env python
import socket, struct
import urllib2
from xml.dom.minidom import parse, parseString
import re
from errors import NetworkError

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
            url = '%s/redirector/getServerList' % self.url
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

