/*jshint strict: false */
/*jshint unused: false */

define([
    'modules/xmpp/xmppBridge',
    'modules/xmpp/xmppStrophe',
    'core/logger',
    'modules/client/client'
], function(xmppBridge, xmppStrophe, logger, client) {
    
    var xmpp = xmppStrophe;

    //check and see if bridge exists
    if (client.isEmbeddedBrowser()) {
        logger.log('using bridge xmpp');
        xmpp = xmppBridge;
    } else {
        logger.log('using strophe xmpp');
    }

    xmpp.init();
    return xmpp;
});