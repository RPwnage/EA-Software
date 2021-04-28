/*jshint unused: false */
/*jshint strict: false */

define([
    'promise',
    'core/utils',
    'core/logger',
    'core/user',
    'core/locale',
    'core/auth',
    'core/events',
    'core/defines',
    'core/urls',
    'core/windows',
    'core/datetime',
    'core/telemetry',
    'core/performance',
    'core/beacon',
    'core/anonymoustoken',
    'core/dataManager',
    'modules/achievements/achievement',
    'modules/feeds/feeds',
    'modules/client/client',
    'modules/games/games',
    'modules/games/cart',
    'modules/games/lmd',
    'modules/games/subscription',
    'modules/games/trial',
    'modules/games/gifts',
    'modules/settings/settings',
    'modules/social/atom',
    'modules/social/avatar',
    'modules/social/friends',
    'modules/social/obfuscate',
    'modules/xmpp/xmpp',
    'modules/search/search',
    'modules/voice/voice',
    'modules/commerce/commerce',
    'modules/pin/pin',
    'modules/wishlist/wishlist',
    'modules/idobfuscate/idobfuscate',
    'social/groups',
    'patches/strophe-patch.js',
    'generated/jssdkconfig.js',
    'modules/client/clientobjectregistry'
], function(Promise, utils, logger, user, configService, auth, events, defines, urls, windows, datetime, telemetry, performance, beacon, anonymousToken, dataManager, achievements, feeds, client, games, cart, lmd, subscription, trial, gifts, settings, atom, avatar, friends, obfuscate, xmpp, search, voice, commerce, pin, wishlist, idObfuscate,  groups, strophePatch, jssdkconfig, clientObjectRegistry) {

    /**
     * @exports Origin
     */

    var jssdk = {
        /**
         * the version number of the Origin JSSDK
         * @method
         * @return {string} name the version number (X.X.X)
         */
        version: function() {
            return '0.0.1';
        },

        /**
         * initialization function for the Origin JSSDK
         * @method
         * @param {object=} overrides an object to be mixed in with the jssdk config object in order to override service endpoints
         * @return {promise} name resolved indicates the initialization succeed, reject means the intialization failed
         */
        init: function(locale) {

            /*jshint undef:false */
            var self = this;

            //init the core
            function initCore() {
                configService.setLocale(locale);

                jssdk.beacon = beacon;
                jssdk.auth = auth;
                jssdk.user = user.publicObjs;
                jssdk.events = events;
                jssdk.defines = defines;
                jssdk.locale = configService;
                jssdk.windows = windows;
                jssdk.datetime = datetime;
                jssdk.anonymousToken = anonymousToken;

                urls.init();
            }

            function initModules() {
                jssdk.achievements = achievements;
                jssdk.feeds = feeds;
                jssdk.client = client;
                jssdk.games = games;
                jssdk.cart = cart;
                jssdk.subscription = subscription;
                jssdk.trial = trial;
                jssdk.gifts = gifts;
                jssdk.settings = settings;
                jssdk.atom = atom;
                jssdk.avatar = avatar;
                jssdk.xmpp = xmpp;
                jssdk.search = search;
                jssdk.voice = voice;
                jssdk.commerce = commerce;
                jssdk.wishlist = wishlist;
                jssdk.idObfuscate = idObfuscate;
                jssdk.groups = groups;
                jssdk.friends = friends;
                jssdk.obfuscate = obfuscate;
                jssdk.pin = pin;
                jssdk.dataManager = dataManager;
            }


            initCore();

            //waits for the promise to resolve then calls init modules before return a promise for the Origin.init call
            return clientObjectRegistry.init()
                .then(lmd.markAllLMDdirty)
                .then(initModules);
        }
    };

    //we want these available even before Origin.init happens
    jssdk.log = logger.publicObjs;
    jssdk.utils = utils;
    jssdk.config = jssdkconfig;
    jssdk.telemetry = telemetry;    //does have a dependency on utils (for Communicator)
    jssdk.performance = performance;
    return jssdk;
});
