import urllib2
import urllib
import json
import os
import sys
import socket
import requests
import threading
import urlparse
import random
import boto

def is_mac():
    return sys.platform == 'darwin'

def yesno(question):
    answer = None
    while not answer:
        answer = raw_input('{0} (y/n):'.format(question))
        if answer.lower() in ['y','yes']:
            return True
        elif answer.lower() in ['n','no']:
            return False
        else:
            print 'Please use "y" or "n"'
            answer = None

def selectFromList(valueList, entity):
    value = None
    print "Select {0}:".format(entity)
    while not value:
        try:
            counter = 1
            for item in valueList:
                print '{0}. {1}'.format(counter,item)
                counter += 1
            promptno = raw_input("Please enter number of {0}:".format(entity))
            if int(promptno) > len(valueList):
                print "Not a selectable number"
            else:
                return valueList[int(promptno)-1]
        except ValueError:
            print "Not a number"


def loginreg_by_env(env):
    if env.lower()=='production':
        return 'https://loginregistration.dm.origin.com/loginregistration/authenticate'
    elif env.lower()=='fc.qa':
        return 'https://integration.loginregistration.dm.origin.com/loginregistration/authenticate'
    else:
        return '0'
    
def ecommerce_by_env(env):
    if env.lower()=='production':
        return 'https://ecommerce2.dm.origin.com/ecommerce2/consolidatedentitlements'
    elif env.lower()=='fc.qa':
        return 'https://integration.ecommerce2.dm.origin.com/ecommerce2/consolidatedentitlements'
    else:
        return '0'

ACCOUNT_MANAGER_URL = 'https://crs.dm.origin.com:8080/automation_services/{0}'


class Request(urllib2.Request):
    """
    Subclass request to prepend our account manager url
    And add support for PUT/DELETE urls
    """
    def __init__(self,method,url,data=None, headers={},
                 origin_req_host=None, unverifiable=False):
        urllib2.Request.__init__(self, url, data, headers, origin_req_host, unverifiable)
        self.method = method

    def get_method(self):
        return self.method
        
def make_request(method,url,data=None,headers={}):
    try:
        headers['User-Agent'] = 'Mozilla/5.0 EA Download Manager Origin/8.6'
        headers['Content-Type'] = 'application/x-www-form-urlencoded'
        if method=='GET' and data:
            url += '?{0}'.format(urllib.urlencode(data,True))
            data=None
        elif method!='GET' and data and isinstance(data,dict):
            data = urllib.urlencode(data)
        request = Request(method,url,headers=headers)
        return urllib2.urlopen(request,data).read()
    except urllib2.HTTPError as exp:
        myerror = 'HTTP error {2} {3} raised with {0}, {1} {4}'.format(method,url,exp.code,exp.read(),data)
        raise RuntimeError(myerror)    
        
## Returns if this machine is a cloud machine or not
def is_cloud():
    return socket.gethostname()[0:3].lower() == 'ip-'

## Returns the hostname of this machine
def get_hostname():
    if is_cloud():
        hostname = urllib2.urlopen('http://169.254.169.254/latest/meta-data/public-hostname').read()
    else:
        hostname = socket.gethostname()
    return hostname

class AccountManagerSession(requests.Session):
    def prepare_request(self, request):
        #turn off verification, too lazy to write a constructor just for this
        self.verify = False
        #first lets prepend the host
        request.url = ACCOUNT_MANAGER_URL.format(request.url)
        #then lets json encode the body if it's there
        if request.data is not None and request.data != {}:
            request.data = json.dumps(request.data)
        #also let's put the right content type in
        if request.headers == None:
            request.headers = {}
        request.headers['Content-type'] = 'text/json'
        prep_request = super(AccountManagerSession, self).prepare_request(request)
        return prep_request    

account_manager = AccountManagerSession()
account_manager.mount("https://",requests.adapters.HTTPAdapter(max_retries=5))

## This is the old way of calling the account manager,
# I moved it over to use the new requests library for legacy reasons but please use
# account_manager session to make new requests directly rather than using this helper function
def make_accountmanager_request(method,url,data=None):
    if method == 'GET':
        result = account_manager.get(url,params=data)
    elif method == 'POST':
        result = account_manager.post(url,data=data)
    elif method == 'PUT':
        result = account_manager.put(url,data=data)
    elif method == 'DELETE':
        result = account_manager.delete(url,data=data)
    else:
        raise RuntimeError("invalid method")
    if not result.content or result.content == 'None':
        return None
    json_response = json.loads(result.content)
    if 'document' in json_response:
        return json_response['document']
    return json_response

ACCESS_KEY = 'TODO - retrieve from credentials vault solution if this is still needed, otherwise leave it as it is'
SECRET_KEY = 'TODO - retrieve from credentials vault solution if this is still needed, otherwise leave it as it is'
s3_info = threading.local()

##
# Returns the bucket object from the connection
#
def get_bucket():
    if not hasattr(s3_info,'bucket'):
        s3_info.bucket = boto.connect_s3(ACCESS_KEY, SECRET_KEY).get_bucket('client-automation')
    return s3_info.bucket

##
# Takes a file and downloads it from amazon s3
#
# @param local_file: The local file name to download to (string)
# @param s3_key_prefix: The prefix to the key (probably should end in /)
# @param s3_key_name: The name to use when downloading file (if not the local filename, otherwise leave blank)
# @return: the downloaded file 
def download_file(local_file, s3_key_prefix, s3_key_name=None):
    if not s3_key_name:
        s3_key = '{0}{1}'.format(s3_key_prefix,os.path.basename(local_file))
    else:
        s3_key = '{0}{1}'.format(s3_key_prefix,s3_key_name)
    try:
        mykey = get_bucket().new_key(s3_key)
        mykey.get_contents_to_filename(local_file)
    except Exception as exp:
        print exp
    return open(local_file)

##
# Takes a file and uploads it to amazon s3
#
# @param local_file: The local file to upload from (actual file stream)
# @param s3_key_prefix: The prefix to the key (probably should end in /)
# @param s3_key_name: The name to use when uploading file (if not the local filename, otherwise leave blank) 
def upload_file(local_file, s3_key_prefix, s3_key_name=None):
    #if no key name specified, use local_filename
    if not s3_key_name:
        s3_key = '{0}{1}'.format(s3_key_prefix,os.path.basename(local_file))
    else:
        s3_key = '{0}{1}'.format(s3_key_prefix,s3_key_name)
    try:
        mykey = get_bucket().new_key(s3_key)
        mykey.set_contents_from_filename(local_file)
    except Exception as exp:
        pass

## Dummy adapter for when a url redirects to QRC (Qt)
class QRCAdapter(requests.adapters.BaseAdapter):
    def send(self,request,**kwargs):
        response = requests.Response()
        #don't actually load the url, just make sure the url
        #is stuffed into the response
        response.url = request.url
        return response
    
        
        

## gets the user id from eadp's services
def get_userid(auth_token,is_production=False):
    if is_production:
        userid_url = "https://gateway.ea.com/proxy/identity/pids/me"
    else:
        userid_url = "https://gateway.int.ea.com/proxy/identity/pids/me"
    myreq = requests.get(userid_url,headers={'Authorization':'Bearer {0}'.format(auth_token)})
    return myreq.json()['pid']['pidId']
    

## gets the auth token from identity
def get_eadp_auth_token(email,password,is_production=False):
    if is_production:
        auth_url = "https://accounts.ea.com"
        client_secret =  'TODO - retrieve from credentials vault solution if this is still needed, otherwise leave it as it is'
    else:
        auth_url = "https://accounts.int.ea.com"
        client_secret = 'TODO - retrieve from credentials vault solution if this is still needed, otherwise leave it as it is'

    #these are the GET parameters passed to the url
    params = {'client_id':'ORIGIN_PC','response_type':'code','scope':'signin basic.identity openid','nonce':random.randint(0,100)}
    #create a new session object to hold the cookies
    session = requests.Session()
    #call the url to load the login page. Several redirects happen here.
    #The purpose of the redirects is to set some cookies, but the session object keeps track of the cookies.
    #So we allow it to redirect until it hits the page it wants
    base_url = session.get('{0}/connect/auth'.format(auth_url),params=params)
    #this is the information we submit to the form
    page_params = {'email':email,'password':password,'_eventId':'submit'}
    #after the page is done the redirect path it will hit a qrc: url which is a qt protocol.
    #we created a dummy adapter above to make sure it doesn't try to load the qrc url, because it can't
    session.mount("qrc:",QRCAdapter())
    #post to the form and let it do its redirects. The last redirect is to the qrc URL
    form_submitted = session.post(base_url.url,data=page_params)
    if not form_submitted.url or not 'qrc:' in form_submitted.url:
        raise RuntimeError("Auth token failed, redirected to url:{0}".format(form_submitted.url))
    #the access token is in the query string of the final redirected link
    code = form_submitted.url.split('code=')[1]
    #now we call the token endpoint with the code we just got to generate a token
    code_params = {'client_id':'ORIGIN_PC','grant_type':'authorization_code','code':code,'client_secret':client_secret}
    final_request = session.post('{0}/connect/token'.format(auth_url),data=code_params)
    return final_request.json()['access_token']
    