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
UTF_8 = 'utf-8'
CONFIG_INI = 'dump_offer.ini'
BASEAPI = 'https://dev.api1.origin.com/preview'


ENDPOINTS = { 
              'catalogInfo' : BASEAPI + '/ecommerce2/public/supercat/{0}/{1}'
            }

# # Dummy adapter for when a url redirects to QRC (Qt)
class QRCAdapter(requests.adapters.BaseAdapter):
    def send(self, request, **kwargs):
        response = requests.Response()
        # don't actually load the url, just make sure the url
        # is stuffed into the response
        response.url = request.url
        return response

def get_write_catalog_info(offer_id, locale, base_dir, logger):
    logger.info('retrieving offer: ' + offer_id)

    headers = { 'Accept':'application/json',
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

def print_xml(xml):
    elem = ET.fromstring(xml)
    rough_string = ET.tostring(elem, UTF_8)
    reparsed = xml.dom.minidom.parseString(rough_string)
    # print reparsed.toprettyxml(indent="\t")



def write_utf8(file, content):
    with codecs.open(file, 'w', encoding=UTF_8) as f:
        f.write(unicode(content, 'utf8'))



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


#########################################
# # Main
#########################################
def main(argv):

    # create logger
    logger = logging.getLogger('dump_offer')
    logger.setLevel(logging.DEBUG)
    # create file handler which logs even debug messages
    fh = logging.FileHandler('dump_offer.log')
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

    offerId = config.get('user', 'offerId')
    locales = config.get('mockup', 'locales').split(';')
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

    for locale in locales:
        get_write_catalog_info(offerId, locale, baseDir, logger)

if __name__ == "__main__":
    main(sys.argv[1:])
