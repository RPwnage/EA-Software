#!/usr/bin/env python

environment = {
    'DEV': {
        'nucleus':'http://integration.nucleus.ea.com',
        'redirector':'http://internal.gosredirector.online.ea.com:42125'
    },
    'STEST': {
        'nucleus':'http://integration.nucleus.ea.com',
        'redirector':'http://web.internal.gosredirector.stest.ea.com:42125'
    },
    'SCERT': {
        'nucleus':'http://integration.nucleus.ea.com',
        'redirector':'http://internal.gosredirector.scert.ea.com:42125'
    },
    'PROD': {
        'nucleus':'http://nucleus.ea.com',
        'redirector':'http://internal.gosredirector.ea.com:42125'
    },
}

clienttypes = {
    'CONSOLE_USER':0,
    'WEB_ACCESS_LAYER':1,
    'DEDICATED_SERVER':2,
}
