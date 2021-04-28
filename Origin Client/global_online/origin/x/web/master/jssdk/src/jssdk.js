/*jshint unused: false */
/*jshint strict: false */

define([
    'promise',
    'core/utils',
    'core/logger',
    'core/user',
    'core/config',
    'core/auth',
    'core/events',
    'core/defines',
    'core/urls',
    'core/windows',
    'core/datetime',
    'core/telemetry',
    'modules/achievements/achievement',
    'modules/feeds/feeds',
    'modules/client/client',
    'modules/games/games',
    'modules/games/lmd',
    'modules/games/subscription',
    'modules/settings/settings',
    'modules/social/atom',
    'modules/social/avatar',
    'modules/social/friends',
    'modules/xmpp/xmpp',
    'modules/search/search',
    'modules/voice/voice',
    'social/groups',
    'patches/strophe-patch.js',
    'generated/jssdkconfig.js',
    'modules/client/clientobjectregistry'
], function(Promise, utils, logger, user, configService, auth, events, defines, urls, windows, datetime, telemetry, achievements, feeds, client, games, lmd, subscription, settings, atom, avatar, friends, xmpp, search, voice, groups, strophePatch, jssdkconfig, clientObjectRegistry) {

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

                jssdk.auth = auth;
                jssdk.user = user.publicObjs;
                jssdk.events = events;
                jssdk.defines = defines;
                jssdk.locale = configService;
                jssdk.windows = windows;
                jssdk.datetime = datetime;

                urls.init();
            }

            function initModules() {
                jssdk.achievements = achievements;
                jssdk.feeds = feeds;
                jssdk.client = client;
                jssdk.games = games;
                jssdk.subscription = subscription;
                jssdk.settings = settings;
                jssdk.atom = atom;
                jssdk.avatar = avatar;
                jssdk.xmpp = xmpp;
                jssdk.search = search;
                jssdk.voice = voice;
                jssdk.groups = groups;
                jssdk.friends = friends;
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
    return jssdk;
});