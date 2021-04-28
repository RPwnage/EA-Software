import json
import requests
import urllib
import os
import codecs
import logging
import sys
import ConfigParser
import random
import shutil

USER_AGENT = 'Mozilla/5.0 EA Download Manager Origin/9.4.20.386'
LOGINREG_URL = 'https://accounts.int.ea.com'
EADP_CLIENT_SECRET = 'ORIGIN_PC_SECRET'
UTF_8 = 'utf-8'
CONFIG_INI = 'create_mockup_data.ini'
FEEDS = ['mostpopulargame', 'newdlc', 'merchandise', 'recfriends']

ENDPOINTS = { 'userPID' : 'https://gateway.int.ea.com/proxy/identity/pids/me',
              'userPersona' : 'https://gateway.int.ea.com/proxy/identity/pids/{0}/personas',
              'baseGameEntitlements' : 'https://qa.ecommerce2.dm.origin.com/ecommerce2/basegames/{0}?machine_hash=1',
              'catalogInfo' : 'https://cdn.qa.ecommerce2.dm.origin.com/ecommerce2/public/{0}/{1}',
              'extraContentEntitlements' : 'https://qa.ecommerce2.dm.origin.com/ecommerce2/extracontent/{0}?machine_hash=1&masterTitleId={1}',
              'catalogInfoLMD' : 'https://qa.ecommerce2.dm.origin.com/ecommerce2/offerUpdatedDate?offerIds={0}',
              "criticalCatalogInfo":"https://qa.ecommerce2.dm.origin.com/ecommerce2/public/supercat/{0}",
              'avatarUrls' : 'https://integration.avatar.dm.origin.com/avatar/user/{0}/avatars?size={1}',
              'atomUsers' : 'https://qa.atom.dm.origin.com/atom/users?userIds={0}',
              'atomGameUsage' : 'https://qa.atom.dm.origin.com/atom/users/{0}/games/{1}/usage',
              'atomGameLastPlayed' : 'https://qa.atom.dm.origin.com/atom/users/{0}/games/lastplayed',
              'settingsData' : 'https://qa.atom.dm.origin.com/atom/users/{0}/settings',
              'feedStories' : 'https://dl.qa.feeds.x.origin.com/feeds/{0}/{1}/{2}/?size={3}' }



# # Dummy adapter for when a url redirects to QRC (Qt)
class QRCAdapter(requests.adapters.BaseAdapter):
    def send(self, request, **kwargs):
        response = requests.Response()
        # don't actually load the url, just make sure the url
        # is stuffed into the response
        response.url = request.url
        return response



# # gets the auth token from identity
def get_eadp_auth_token(email, password, base_dir, logger):
    # these are the GET parameters passed to the url
    params = { 'client_id':'ORIGIN_PC',
               'response_type':'code',
               'scope':'signin basic.identity basic.persona',
               'nonce':random.randint(0, 100) }
    # create a new session object to hold the cookies
    session = requests.Session()
    # call the url to load the login page. Several redirects happen here.
    # The purpose of the redirects is to set some cookies, but the session object keeps track of the cookies.
    # So we allow it to redirect until it hits the page it wants
    base_url = session.get('{0}/connect/auth'.format(LOGINREG_URL), params=params, verify=False)
    # this is the information we submit to the form
    page_params = { 'email':email,
                    'password':password,
                    '_eventId':'submit' }
    # after the page is done the redirect path it will hit a qrc: url which is a qt protocol.
    # we created a dummy adapter above to make sure it doesn't try to load the qrc url, because it can't
    session.mount("qrc:", QRCAdapter())
    # post to the form and let it do its redirects. The last redirect is to the qrc URL
    form_submitted = session.post(base_url.url, data=page_params)
    if not form_submitted.url or not 'qrc:' in form_submitted.url:
        raise RuntimeError("Auth token failed, redirected to url:{0}".format(form_submitted.url))
    # the access token is in the query string of the final redirected link
    code = form_submitted.url.split('code=')[1]
    # now we call the token endpoint with the code we just got to generate a token
    code_params = { 'client_id':'ORIGIN_PC',
                    'grant_type':'authorization_code',
                    'code':code,
                    'client_secret':EADP_CLIENT_SECRET }
    final_request = session.post('{0}/connect/token'.format(LOGINREG_URL), data=code_params)

    file = os.path.join(base_dir, 'auth.json')
    write_pretty_json(file, final_request.json())

    return final_request.json()['access_token']



def get_write_pid(auth_token, new_pid, base_dir, logger):
    logger.info('retrieving pid and replace with ' + new_pid)

    headers = { 'Authorization':'Bearer {0}'.format(auth_token) }

    resp = requests.get(ENDPOINTS['userPID'], headers=headers, verify=False)
    resp.encoding = UTF_8
    debug_resp(resp, logger)

    if 200 == resp.status_code:
        try:
            pidId = str(resp.json()['pid']['pidId'])
            if len(pidId) > 0:
                # write new_pid instead of pidId
                toBeWritten = resp.json()
                toBeWritten['pid']['pidId'] = new_pid
                file = os.path.join(base_dir, 'pid.json')
                write_pretty_json(file, toBeWritten)

                return pidId
            else:
                logger.error('empty pidId')
        except Exception as e:
            logger.exception(str(e))

    return 'error'



def write_persona(auth_token, pid_id, new_pid, base_dir, logger):
    logger.info('retrieving persona and replace ' + pid_id + ' with ' + new_pid)

    headers = { 'Authorization':'Bearer {0}'.format(auth_token),
                'X-Expand-Results':'true',
                'User-Agent':USER_AGENT,
                'Accept':'*/*',
                'Accept-Encoding':'gzip,deflate,sdch',
                'Accept-Language':'en-US,en;q=0.8' }

    resp = requests.get(ENDPOINTS['userPersona'].format(pid_id), headers=headers, verify=False)
    resp.encoding = UTF_8
    debug_resp(resp, logger)

    if 200 == resp.status_code:
        try:
            # write new_pid instead of pidId
            toBeWritten = resp.json()
            toBeWritten['personas']['persona'][0]['pidId'] = new_pid
            file = os.path.join(base_dir, 'persona.json')
            write_pretty_json(file, toBeWritten)
        except Exception as e:
            logger.exception(str(e))



def get_write_base_game_entitlements(access_token, pid_id, base_dir, logger):
    logger.info('retrieving entitlements of pid_id: ' + pid_id)

    headers = { 'AuthToken':access_token,
                'Accept':'application/vnd.origin.v2+json',
                'Accept-Encoding':'gzip, deflate',
                'User-Agent':USER_AGENT }

    resp = requests.get(ENDPOINTS['baseGameEntitlements'].format(pid_id), headers=headers, verify=False)
    resp.encoding = UTF_8
    debug_resp(resp, logger)

    offerIds = set()

    if 200 == resp.status_code:
        try:
            file = os.path.join(base_dir, 'entitlements.json')
            write_pretty_json(file, resp.json())

            offers = resp.json()['entitlements']
            for offer in offers:
                try:
                    offerIds.add(offer['offerId'])
                except Exception as e:
                    logger.error(str(e))
        except Exception as e:
            logger.exception(str(e))

    return offerIds



def write_ecomm_supercat(locale, base_dir, logger):
    logger.info('retrieving eCommerce2 supercat')

    headers = { 'Accept':'application/json' }

    resp = requests.get(ENDPOINTS['criticalCatalogInfo'].format(locale), headers=headers, verify=False)
    resp.encoding = UTF_8
    debug_resp(resp, logger)

    if 200 == resp.status_code:
        try:
            supercatDir = os.path.join(base_dir, 'supercat')
            if not os.path.exists(supercatDir):
                os.mkdir(supercatDir)

            file = os.path.join(supercatDir, locale + '.json')
            write_pretty_json(file, resp.json())
        except Exception as e:
            logger.exception(str(e))



def get_write_extra_content_entitlements(access_token, pid_id, master_title_id, base_dir, logger) :
    logger.info('retrieving offers of masterTitleId: ' + master_title_id)

    headers = { 'AuthToken':access_token,
                'Accept':'application/vnd.origin.v2+json',
                'Accept-Encoding':'gzip, deflate',
                'Accept-Language':'en-US,*',
                'User-Agent':USER_AGENT,
                'localeInfo':'en_US',
                'X-Origin-Platform':'PCWIN',
                'X-Origin-UID':'8363792082371416094',
                'X-Origin-Client-Debug':'true' }

    resp = requests.get(ENDPOINTS['extraContentEntitlements'].format(pid_id, master_title_id), headers=headers, verify=False)
    resp.encoding = UTF_8
    debug_resp(resp, logger)

    offerIds = set()

    if 200 == resp.status_code:
        try:
            masterTitleIdDir = os.path.join(base_dir, 'masterTitleId')
            if not os.path.exists(masterTitleIdDir):
                os.mkdir(masterTitleIdDir)

            file = os.path.join(masterTitleIdDir, master_title_id + '.json')
            write_pretty_json(file, resp.json())

            offers = resp.json()['entitlements']
            for offer in offers:
                try:
                    offerIds.add(offer['offerId'])
                except Exception as e:
                    logger.error(str(e))
        except Exception as e:
            logger.exception(str(e))

    return offerIds



def get_write_catalog_info(access_token, offer_id, locale, base_dir, logger):
    logger.info('retrieving offer: ' + offer_id)

    headers = { 'AuthToken':access_token,
                'Accept':'application/json',
                'Accept-Encoding':'gzip, deflate',
                'User-Agent':USER_AGENT }

    resp = requests.get(ENDPOINTS['catalogInfo'].format(urllib.quote(offer_id, ''), locale), headers=headers, verify=False)
    resp.encoding = UTF_8
    debug_resp(resp, logger)

    if 200 == resp.status_code:
        try:
            offerBaseDir = os.path.join(base_dir, 'offer')
            if not os.path.exists(offerBaseDir):
                os.mkdir(offerBaseDir)

            offerDir = os.path.join(offerBaseDir, urllib.quote(offer_id, ''))
            if not os.path.exists(offerDir):
                os.mkdir(offerDir)

            file = os.path.join(offerDir, locale + '.json')
            write_pretty_json(file, resp.json())

            return resp.json()
        except Exception as e:
            logger.exception(str(e))

    return {}



def write_catalog_LMD(access_token, offer_id, base_dir, logger):
    logger.info('retrieving offer: ' + offer_id)

    headers = { 'AuthToken':access_token,
                'Accept':'application/json' }

    resp = requests.get(ENDPOINTS['catalogInfoLMD'].format(urllib.quote(offer_id, '')), headers=headers, verify=False)
    resp.encoding = UTF_8
    debug_resp(resp, logger)

    if 200 == resp.status_code:
        try:
            offerLMDDir = os.path.join(base_dir, 'offerLMD')
            if not os.path.exists(offerLMDDir):
                os.mkdir(offerLMDDir)

            file = os.path.join(offerLMDDir, urllib.quote(offer_id, '') + '.json')
            write_pretty_json(file, resp.json())
        except Exception as e:
            logger.exception(str(e))



def write_avatar_urls(access_token, pid_id, user_ids, size, base_dir, logger):
    logger.info('retrieving avatar of: ' + ','.join(user_ids) + '/size:' + size)

    headers = { 'AuthToken':access_token }

    sizeVal = 0
    if size == 'AVATAR_SZ_SMALL':
        sizeVal = 0
    elif size == 'AVATAR_SZ_MEDIUM':
        sizeVal = 1
    elif size == 'AVATAR_SZ_LARGE':
        sizeVal = 2

    resp = requests.get(ENDPOINTS['avatarUrls'].format(';'.join(user_ids), sizeVal), headers=headers, verify=False)
    resp.encoding = UTF_8
    debug_resp(resp, logger)

    if 200 == resp.status_code:
        try:
            avatarDir = os.path.join(base_dir, 'avatar')
            if not os.path.exists(avatarDir):
                os.mkdir(avatarDir)

            file = os.path.join(avatarDir, str(sizeVal) + '.xml')
            write_utf8(file, resp.content)
        except Exception as e:
            logger.exception(str(e))



def write_atom_user_info_by_user_ids(access_token, user_ids, base_dir, logger):
    logger.info('retrieving atom user: ' + ','.join(user_ids))

    headers = { 'AuthToken':access_token }

    resp = requests.get(ENDPOINTS['atomUsers'].format(','.join(user_ids)), headers=headers, verify=False)
    resp.encoding = UTF_8
    debug_resp(resp, logger)

    if 200 == resp.status_code:
        try:
            atomDir = os.path.join(base_dir, 'atom')
            if not os.path.exists(atomDir):
                os.mkdir(atomDir)

            file = os.path.join(atomDir, 'atomUserInfoByUserIds.xml')
            write_utf8(file, resp.content)
        except Exception as e:
            logger.exception(str(e))



def write_atom_game_last_played(access_token, pid_id, base_dir, logger):
    logger.info('retrieving atom game last played of pid_id: ' + pid_id)

    headers = { 'AuthToken':access_token }

    resp = requests.get(ENDPOINTS['atomGameLastPlayed'].format(pid_id), headers=headers, verify=False)
    resp.encoding = UTF_8
    debug_resp(resp, logger)

    if 200 == resp.status_code:
        try:
            atomDir = os.path.join(base_dir, 'atom')
            if not os.path.exists(atomDir):
                os.mkdir(atomDir)

            file = os.path.join(atomDir, 'atomGameLastPlayed.xml')
            write_utf8(file, resp.content)
        except Exception as e:
            logger.exception(str(e))



def write_atom_game_usage(access_token, pid_id, master_title_id, multiplayer_id, base_dir, logger):
    logger.info('retrieving atom game usage of master_title_id: ' + master_title_id + '/multiplayer_id:' + str(multiplayer_id))

    headers = { 'Accept':'*/*',
                'Accept-Encoding':'gzip,deflate,sdch',
                'Accept-Language':'en-US,en;q=0.8',
                'User-Agent':USER_AGENT,
                'AuthToken':access_token }

    if multiplayer_id is not None:
        headers['MultiplayerId'] = multiplayer_id

    resp = requests.get(ENDPOINTS['atomGameUsage'].format(pid_id, master_title_id), headers=headers, verify=False)
    resp.encoding = UTF_8
    debug_resp(resp, logger)

    if 200 == resp.status_code:
        try:
            atomDir = os.path.join(base_dir, 'atom')
            if not os.path.exists(atomDir):
                os.mkdir(atomDir)

            masterTitleDir = os.path.join(atomDir, master_title_id)
            if not os.path.exists(masterTitleDir):
                os.mkdir(masterTitleDir)

            file = os.path.join(masterTitleDir, 'usage.xml')
            write_utf8(file, resp.content)
        except Exception as e:
            logger.exception(str(e))



def write_atom_settings(access_token, pid_id, base_dir, logger):
    logger.info('retrieving atom settings of pid_id: ' + str(pid_id))

    headers = { 'AuthToken':access_token }

    resp = requests.get(ENDPOINTS['settingsData'].format(pid_id), headers=headers, verify=False)
    resp.encoding = UTF_8
    debug_resp(resp, logger)

    if 200 == resp.status_code:
        try:
            atomDir = os.path.join(base_dir, 'atom')
            if not os.path.exists(atomDir):
                os.mkdir(atomDir)

            file = os.path.join(atomDir, 'settings.xml')
            write_utf8(file, resp.content)
        except Exception as e:
            logger.exception(str(e))



def get_write_feed_story_data(feed_type, locale, base_dir, logger):
    logger.info('retrieving feed: ' + feed_type)

    headers = {  }

    resp = requests.get(ENDPOINTS['feedStories'].format(feed_type, locale, 0, 10), headers=headers, verify=False)
    resp.encoding = UTF_8
    debug_resp(resp, logger)

    offerIds = set()

    if 200 == resp.status_code:
        try:
            feedDir = os.path.join(base_dir, 'feeds')
            if not os.path.exists(feedDir):
                os.mkdir(feedDir)

            localeDir = os.path.join(feedDir, locale)
            if not os.path.exists(localeDir):
                os.mkdir(localeDir)

            file = os.path.join(localeDir, feed_type + '.json')
            write_pretty_json(file, resp.json())

            offers = resp.json()['stories']
            for offer in offers:
                try:
                    offerIds.add(offer['offerId'])
                except Exception as e:
                    logger.debug(str(e))
        except Exception as e:
            logger.exception(str(e))

    return offerIds



def write_friends(friends, base_dir, logger):
    logger.info('writing friends.json')

    friendsObj = [];
    for friend in friends:
        friendObj = { 'jid':friend + '@127.0.0.1',
                      'name':friend + '_n',
                      'personaid':friend + '_p',
                      'eaid':friend + '_e',
                      'subscription':'BOTH' }
        friendsObj.append(friendObj)

    try:
        socialDir = os.path.join(base_dir, 'social')
        if not os.path.exists(socialDir):
            os.mkdir(socialDir)

        file = os.path.join(socialDir, 'friends.json')
        write_pretty_json(file, friendsObj)
    except Exception as e:
        logger.exception(str(e))



def write_groups(base_dir, logger):
    logger.info('writing groups data')

    try:
        socialDir = os.path.join(base_dir, 'social')
        if not os.path.exists(socialDir):
            os.mkdir(socialDir)

        file = os.path.join(socialDir, 'groupListByUserId.json')
        write_pretty_json(file, '[]')

        file = os.path.join(socialDir, 'memberListByGroup.json')
        write_pretty_json(file, '[]')

        file = os.path.join(socialDir, 'roomListByGroup.json')
        write_pretty_json(file, '[]')
    except Exception as e:
        logger.exception(str(e))



def write_config_url_override_json(new_pid, base_dir, logger):
    logger.info('writing jssdkconfig.json')

    protocol = '{protocol}'
    host = '{host}'
    port = ':{port}'
    baseURL = protocol + '://' + host + port + '/data/' + new_pid
    configObj = { 'filename': 'jssdkconfig.json',
                  'urls':{ 'connectAuth':baseURL + '/auth?',
                           'userPID':baseURL + '/pid?',
                           'userPersona':baseURL + '/persona?',
                           'baseGameEntitlements':baseURL + '/entitlements?',
                           'catalogInfo':baseURL + '/offer/{productId}/{locale}?',
                           'catalogInfoPrivate':baseURL + '/offer/{productId}/{locale}?',
                           'extraContentEntitlements':baseURL + '/masterTitleId/{masterTitleId}?',
                           'catalogInfoLMD':baseURL + '/offerLMD/{productId}?',
                           'criticalCatalogInfo':baseURL + '/supercat/{locale}?',
                           'avatarUrls':baseURL + '/avatar/{size}.xml?',
                           'atomUsers':baseURL + '/atom/atomUserInfoByUserIds.xml?',
                           'atomGameUsage':baseURL + '/atom/{masterTitleId}/usage.xml?',
                           'atomGameLastPlayed':baseURL + '/atom/atomGameLastPlayed.xml?',
                           'groupList':baseURL + '/social/groupListByUserId?',
                           'membersList':baseURL + '/social/memberListByGroup?',
                           'roomList':baseURL + '/social/roomListByGroup?',
                           'settingsData':baseURL + '/atom/settings.xml?',
                           'feedStories':baseURL + '/feeds/{locale}/{feedType}?',
                           'xmppConfig':{ 'wsHost':host,
                                          'wsPort':'5291',
                                          'redirectorUrl':protocol + '://' + host,
                                          'domain':host,
                                          'wsScheme':'ws' }
                         }
                };

    try:
        file = os.path.join(base_dir, 'jssdkconfig.json')
        write_pretty_json(file, configObj)

        file = os.path.join(base_dir, 'blank.json')
        write_pretty_json(file, {})
    except Exception as e:
        logger.exception(str(e))



def print_xml(xml):
    elem = ET.fromstring(xml)
    rough_string = ET.tostring(elem, UTF_8)
    reparsed = xml.dom.minidom.parseString(rough_string)
    # print reparsed.toprettyxml(indent="\t")



def write_utf8(file, content):
    with codecs.open(file, 'w', encoding=UTF_8) as f:
        f.write(content)



def write(file, content):
    with open(file, 'w') as f:
        f.write(content)



def write_pretty_json(file, json_obj):
    with codecs.open(file, 'w', encoding=UTF_8) as f:
        json.dump(json_obj, f, indent=2, ensure_ascii=False, sort_keys=True)



def debug_resp(resp, logger):
    logger.info('  url: ' + resp.url)
    if 200 == resp.status_code:
        logger.info('  status_code: ' + str(resp.status_code))
    else:
        logger.error('  status_code: ' + str(resp.status_code))
    logger.debug('  headers:')
    for x in resp.headers:
        logger.debug('    ' + x + ': ' + resp.headers[x])



def process_offer(offer_id, offer_id_set, master_title_id_set, locales, access_token, base_dir, logger):
    if (offer_id not in offer_id_set):
        offer_id_set.add(offer_id)
        write_catalog_LMD(access_token, offer_id, base_dir, logger)
        for locale in locales:
            offer = get_write_catalog_info(access_token, offer_id, locale, base_dir, logger)
            try:
                availableExtraContents = offer['ecommerceAttributes']['availableExtraContent']
                for extraContentId in availableExtraContents:
                    process_offer(extraContentId, offer_id_set, master_title_id_set, locales, access_token, base_dir, logger)

                master_title_id_set |= get_master_title_ids(offer, logger)
            except Exception as e:
                logger.warn(str(e))



def get_offer_from_file(offer_id, base_dir, logger):
    offerPath = os.path.join(os.path.join(os.path.join(base_dir, 'offer'), urllib.quote(offer_id, '')), 'DEFAULT.json')
    try:
        with open(offerPath) as offer_data:
            offer = json.load(offer_data)
            offer_data.close()
            return offer
    except Exception as e:
        logger.exception(str(e))

    return None



def get_multiplayer_id(offer, logger):
    try:
        for software in offer['publishing']['softwareList']['software']:
            try:
                multiplayerId = software['fulfillmentAttributes']['multiPlayerId']
                if multiplayerId is not None:
                    return str(multiplayerId)
            except Exception as e:
                pass
    except Exception as e:
        logger.warn(str(e))

    return None



def get_master_title_ids(offer, logger):
    masterTitleIdSet = set()
    try:
        mdmHierarchy = offer['mdmHierarchies']['mdmHierarchy']
        for m in mdmHierarchy:
            masterTitleId = m['mdmMasterTitle']['masterTitleId']
            if masterTitleId is not None:
                masterTitleIdSet.add(str(masterTitleId))
    except Exception as e:
        logger.exception(str(e))

    return masterTitleIdSet



#########################################
# # Main
#########################################
def main(argv):

    # create logger
    logger = logging.getLogger('create_mockup_data')
    logger.setLevel(logging.DEBUG)
    # create file handler which logs even debug messages
    fh = logging.FileHandler('create_mockup_data.log')
    fh.setLevel(logging.DEBUG)
    # create console handler with a higher log level
    ch = logging.StreamHandler()
    ch.setLevel(logging.INFO)
    # create formatter and add it to the handlers
    formatter = logging.Formatter('%(asctime)s %(levelname)s - %(filename)s:%(lineno)-4d - %(message)s')
    fh.setFormatter(formatter)
    ch.setFormatter(formatter)
    # add the handlers to the logger
    logger.addHandler(fh)
    logger.addHandler(ch)

    logger.info('')
    logger.info('--BEGIN----------------------------------------------')
    logger.info('')

    config = ConfigParser.ConfigParser()
    config.read(CONFIG_INI)

    email = config.get('user', 'email')
    pswd = config.get('user', 'password')
    locales = config.get('mockup', 'locales').split(';')
    friends = config.get('mockup', 'friends').split(';')
    newPid = config.get('mockup', 'newPid')
    if '' == newPid:
        newPid = 'generated'

    if len(argv) != 1:
        logger.error('expect parameters: <output path>')
        exit(-1)

    baseDir = os.path.join(os.path.join(os.getcwd(), argv[0]), newPid)
    if not os.path.exists(baseDir):
        os.mkdir(baseDir)

    # copy create_mockup_data.ini to baseDir
    shutil.copyfile(CONFIG_INI, os.path.join(baseDir, CONFIG_INI))

    logger.info('changing to working directory: ' + baseDir)
    os.chdir(baseDir)

    # get token
    accessToken = get_eadp_auth_token(email, pswd, baseDir, logger)
    logger.info('AuthToken: ' + str(accessToken))

    # get pid
    pidId = get_write_pid(accessToken, newPid, baseDir, logger)
    logger.info('pidId: ' + str(pidId))

    write_persona(accessToken, pidId, newPid, baseDir, logger)
    # exit(0)

    # atom users
    write_atom_user_info_by_user_ids(accessToken, friends, baseDir, logger)
    write_atom_game_last_played(accessToken, pidId, baseDir, logger)
    write_atom_settings(accessToken, pidId, baseDir, logger)

    # friends for XMPP
    write_friends(friends, baseDir, logger)
    # exit(0)

    # configurloverride.json
    write_config_url_override_json(newPid, baseDir, logger)

    # avatar
    write_avatar_urls(accessToken, pidId, friends, 'AVATAR_SZ_SMALL', baseDir, logger)
    write_avatar_urls(accessToken, pidId, friends, 'AVATAR_SZ_MEDIUM', baseDir, logger)
    write_avatar_urls(accessToken, pidId, friends, 'AVATAR_SZ_LARGE', baseDir, logger)
    # exit(0)

    # eComm supercat
    for locale in locales:
        write_ecomm_supercat(locale, baseDir, logger)

    globalOfferIdSet = set()
    globalMasterTitleIdSet = set()

    offerIds = get_write_base_game_entitlements(accessToken, pidId, baseDir, logger)
    for offerId in offerIds:
        process_offer(offerId, globalOfferIdSet, globalMasterTitleIdSet, locales, accessToken, baseDir, logger)
        # atom
        offer = get_offer_from_file(offerId, baseDir, logger)
        multiplayerId = get_multiplayer_id(offer, logger)
        # use the first masterTitleId if available
        try:
            masterTitleId = next(iter(get_master_title_ids(offer, logger)))
            write_atom_game_usage(accessToken, pidId, masterTitleId, multiplayerId, baseDir, logger)
        except Exception as e:
            logger.exception(str(e))

    # globalMasterTitleIdSet.add(182555);
    logger.info(globalMasterTitleIdSet)

    for masterTitleId in globalMasterTitleIdSet:
        get_write_extra_content_entitlements(accessToken, pidId, masterTitleId, baseDir, logger)

    # get feed's offer data
    for feed in FEEDS:
        for locale in locales:
            offerIds = get_write_feed_story_data(feed, locale, baseDir, logger)
            for offerId in offerIds:
                process_offer(offerId, globalOfferIdSet, globalMasterTitleIdSet, locales, accessToken, baseDir, logger)



if __name__ == "__main__":
    main(sys.argv[1:])
