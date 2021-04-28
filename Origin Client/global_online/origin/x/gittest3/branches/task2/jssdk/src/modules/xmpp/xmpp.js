/*jshint strict: false */
/*jshint unused: false */
/*jshint undef: false */
define([
    'modules/xmpp/xmppBridge',
    'modules/xmpp/xmppStrophe',
    'core/logger',
], function (xmppBridge, xmppStrophe, logger) {

    //check and see if bridge exists
    if (typeof OriginSocialManager !== 'undefined') {
        logger.log('using bridge xmpp');
        xmpp = xmppBridge;
    } else {
        logger.log('using strophe xmpp');
        xmpp = xmppStrophe;
    }

    xmpp.init();

    return xmpp;
});
