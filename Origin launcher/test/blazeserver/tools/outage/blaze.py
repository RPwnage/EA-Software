import urllib2
from xml.dom.minidom import parse, parseString
import logging
from errors import NetworkError, BlazeError

AUTH_URL = 'http://%s/authentication/expressLogin?mail=%s&pass=%s'
ADD_USER_URL = 'http://%s/usersessions/assignUserToGroup/%s?gidf|etky=%s&agpn=%s&gidf|clty=%s'
REMOVE_USER_URL = 'http://%s/usersessions/removeUserFromGroup/%s?gidf|etky=%s&agpn=%s&gidf|clty=%s'
INFO_USER_URL = 'http://%s/usersessions/getAccessGroup/%s?gidf|etky=%s&gidf|clty=%s'
LIST_GROUPS_URL = 'http://%s/usersessions/listDefaultAccessGroup/%s'
LIST_USERS_URL = 'http://%s/usersessions/listAuthorization/%s'

class Blaze:
    def __init__(self, host, token=None):
        self.host = host
        self.token = token
    
    def auth(self, user, password):
        url = AUTH_URL % (self.host, user, password)
        req = urllib2.Request(url)
        try:
            res = urllib2.urlopen(req).read()
            dom = parseString(res)
            elements = dom.getElementsByTagName('sessionkey')
            if len(elements) > 0:
                self.token = elements[0].childNodes[0].nodeValue
        except:
            raise NetworkError('Failed to communicate to host %s' % host)
    
    def connected(self):
        if self.token:
            return True
        else:
            return False

    def disconnect(self):
        self.token = None
        
    def get_token(self):
        return self.token
    
    def grant_user(self, uid, group, ctype):
        url = ADD_USER_URL % (self.host, self.token, uid, group, ctype)
        req = urllib2.Request(url)
        try:
            res = urllib2.urlopen(req).read()
            if len(res) > 0:
                dom = parseString(res)
                errorCode = 'Unknown'
                errorName = 'Unknown'
                elements = dom.getElementsByTagName('errorCode')
                if len(elements) > 0:
                    errorCode = elements[0].childNodes[0].nodeValue
                elements = dom.getElementsByTagName('errorName')
                if len(elements) > 0:
                    errorName = elements[0].childNodes[0].nodeValue
                raise AccessControlError(errorCode, errorName, 'There was an error adding user %s to group %s on Blaze host %s' % (uid, group, self.host))
            else:
                pass
        except urllib2.URLError:
            raise NetworkError('Failed to communicate to host %s' % self.host)
    
    def revoke_user(self, uid, group, ctype):
        url = REMOVE_USER_URL % (self.host, self.token, uid, group, ctype)
        req = urllib2.Request(url)
        try:
            res = urllib2.urlopen(req).read()
            if len(res) > 0:
                dom = parseString(res)
                errorCode = 'Unknown'
                errorName = 'Unknown'
                elements = dom.getElementsByTagName('errorCode')
                if len(elements) > 0:
                    errorCode = elements[0].childNodes[0].nodeValue
                elements = dom.getElementsByTagName('errorName')
                if len(elements) > 0:
                    errorName = elements[0].childNodes[0].nodeValue
                raise AccessControlError(errorCode, errorName, 'There was an error removing user %s from group %s on Blaze host %s' % (uid, group, self.host))
        except urllib2.URLError:
            raise NetworkError('Failed to communicate to host %s' % self.host)
            
    def get_usergroup(self, uid, ctype):
        url = INFO_USER_URL % (self.host, self.token, uid, ctype)
        req = urllib2.Request(url)
        group = None
        try:
            res = urllib2.urlopen(req).read()
            if len(res) > 0:
                dom = parseString(res)
                groupname = dom.getElementsByTagName('groupname')
                if len(groupname) > 0:
                    group = groupname[0].childNodes[0].nodeValue
                else:
                    errorCode = 'Unknown'
                    errorName = 'Unknown'
                    elements = dom.getElementsByTagName('errorCode')
                    if len(elements) > 0:
                        errorCode = elements[0].childNodes[0].nodeValue
                    elements = dom.getElementsByTagName('errorName')
                    if len(elements) > 0:
                        errorName = elements[0].childNodes[0].nodeValue
                    raise AccessControlError(errorCode, errorName, 'There was an error retrieving group information for user %s on Blaze host %s' % (uid, self.host))
        except urllib2.URLError:
            raise NetworkError('Failed to communicate to host %s' % self.host)         
        return group
    
    def get_default_groups(self):
        url = LIST_GROUPS_URL % (self.host, self.token)
        req = urllib2.Request(url)
        defaultgroups = []
        try:
            res = urllib2.urlopen(req).read()
            if len(res) > 0:
                dom = parseString(res)
                clienttypegroups = dom.getElementsByTagName('clienttypeaccessgroup')
                if len(clienttypegroups) > 0:
                    for group in clienttypegroups:
                        groupname = group.getElementsByTagName('groupname')[0].childNodes[0].nodeValue
                        clienttype = group.getElementsByTagName('clienttype')[0].childNodes[0].nodeValue
                        group = {'groupname':groupname, 'clienttype':clienttype}
                        defaultgroups.append(group)
                else:
                    errorCode = 'Unknown'
                    errorName = 'Unknown'
                    elements = dom.getElementsByTagName('errorCode')
                    if len(elements) > 0:
                        errorCode = elements[0].childNodes[0].nodeValue
                    elements = dom.getElementsByTagName('errorName')
                    if len(elements) > 0:
                        errorName = elements[0].childNodes[0].nodeValue
                    
                    if errorName == 'ERR_COMMAND_NOT_FOUND':
                        return None
                    else:
                        raise AccessControlError(errorCode, errorName, 'There was an error retrieving group information Blaze host %s' % (self.host))
        except urllib2.URLError:
            raise NetworkError('Failed to communicate to host %s' % self.host)
        
        return defaultgroups
            
    def get_all_users(self):
        url = LIST_USERS_URL % (self.host, self.token)
        req = urllib2.Request(url)
        users = []
        try:
            res = urllib2.urlopen(req).read()
            if len(res) > 0:
                dom = parseString(res)
                records = dom.getElementsByTagName('authorizationrecord')
                if len(records) > 0:
                    for rec in records:
                        groupname = rec.getElementsByTagName('groupname')[0].childNodes[0].nodeValue
                        clienttype = rec.getElementsByTagName('clienttype')[0].childNodes[0].nodeValue
                        externalkey = rec.getElementsByTagName('externalkey')[0].childNodes[0].nodeValue
                        user = {'groupname':groupname, 'clienttype':clienttype, 'externalkey':externalkey}
                        users.append(user)
                else:
                    errorCode = 'Unknown'
                    errorName = 'Unknown'
                    elements = dom.getElementsByTagName('errorCode')
                    if len(elements) > 0:
                        errorCode = elements[0].childNodes[0].nodeValue
                    elements = dom.getElementsByTagName('errorName')
                    if len(elements) > 0:
                        errorName = elements[0].childNodes[0].nodeValue
                    if errorName == 'ERR_COMMAND_NOT_FOUND':
                        return None
                    else:
                        raise AccessControlError(errorCode, errorName, 'There was an error retrieving user information Blaze host %s' % (self.host))
        except urllib2.URLError:
            raise NetworkError('Failed to communicate to host %s' % self.host)
        return users
