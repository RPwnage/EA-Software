#!/usr/bin/env python

import os
import sys
import web
sys.path.append(os.path.join(os.path.dirname(__file__), 'modules'))

from directory import directoryop
from file import fileop
from keyevent import keyeventop
from screencapture import screencaptureop
from shell import shellop
from sysinfo import sysinfoop
from util import utilop

web.config.debug = False

urls = (
    '/capture', screencaptureop,
    '/directory', directoryop,
    '/file', fileop,
    '/keyevent', keyeventop,
    '/shell', shellop,
    '/sysinfo', sysinfoop,
    '/util', utilop,
    '/stop', 'stop'
)

class stop:
    def GET(self):
        app.stop()
        return 'ok'

app = web.application(urls, locals())