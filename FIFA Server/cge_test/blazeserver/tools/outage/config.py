#!/usr/bin/env python

environment = {
    'DEV': {
        'redirector':'internal.gosredirector.online.ea.com:42101'
    },
    'STEST': {
        'redirector':'internal.gosredirector.stest.ea.com:42101'
    },
    'SCERT': {
        'redirector':'internal.gosredirector.scert.ea.com:42101'
    },
    'PROD': {
        'redirector':'internal.gosredirector.prod.ea.com:42101'
    },
}

mailhost = 'mailhost.ea.com'
sender = 'gostools@ea.com'
mailinglist = ['GOSINTERNALOPERATIONS_TIBURON@ea.com']
DOWN_EMUM = ['PARTIAL_OUTAGE','MAINTENANCE_EVENT']
