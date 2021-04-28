import requests
import codecs
from xml.etree import ElementTree
import json

schema = requests.get('https://cms.x.origin.com/schema/components', verify=False)
schema.encoding = 'utf-8'

tree = ElementTree.fromstring(schema.content)

feeds = []
feeds.append({'name' : 'home', 'url' : 'https://cms.x.origin.com/en-ca/content/home.xml'})
feeds.append({'name' : 'origin-news-tile', 'url' : 'xml/origin-news-tile.xml'})
feeds.append({'name' : 'origin-owned-game-tile', 'url' : 'xml/origin-owned-game-tile.xml'})
feeds.append({'name' : 'origin-most-popular-game-tile', 'url' : 'xml/origin-most-popular-game-tile.xml'})
for child in tree:
    url = child.attrib['schemaLocation']
    tokens = url.split('/')
    name = tokens[len(tokens) - 1]
    url = url.replace('schema/components', 'en-ca/content') + '.xml'
    url = url.replace('http:', 'https:')
    feed = {'name': name, 'url': url}
    feeds.append(feed)

with codecs.open('../app/data/test-feed.json', 'w', 'utf-8') as f:
    json.dump(feeds, f, indent=2, ensure_ascii=False, sort_keys=True)
