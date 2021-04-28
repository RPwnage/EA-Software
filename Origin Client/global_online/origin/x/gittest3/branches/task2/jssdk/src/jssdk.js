/*jshint unused: false */
/*jshint strict: false */
/*jshint undef: false */

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
    'modules/feeds/feeds',
    'modules/client/client',
    'modules/games/games',
    'modules/settings/settings',
    'modules/social/atom',
    'modules/social/avatar',
    'modules/xmpp/xmpp',
    'modules/search/search',
    'modules/voice/voice',
	'social/groups',
    'patches/strophe-patch.js',
    'generated/jssdkconfig.js'
], function(Promise, utils, logger, user, configService, auth, events, defines, urls, feeds, client, games, settings, atom, avatar, xmpp, search, voice, groups, strophePatch, jssdkconfig) {
    /**
     * @namespace
     * @alias Origin
     */
    var jssdk = {
        /**
         * version number of sdk (X.X.X)
         * @method
         * @return {string}
         */
        version: function() {
            return '0.0.1';
        },

        /**
         * is a property of {@link configObject}
         * @typedef overrideUrlObject
         * @type {object}
         * @property {string=} userPID endpoint to retrieve the user pid
         * @property {string=} catalogInfo endpoint to retrieve catalog info
         * @property {string=} baseGameEntitlements endpoint to retrive base game entitlements
         * @property {string=} extraContentEntitlements endpoint to retrive extra content entitlements
         * @property {string=} catalogInfoLMD endpoint to get last modified data for a catalog offer
         */
        /**
         * passed as a param into {@link Origin.init}
         * @typedef configObject
         * @type {object}
         * @property {string} env The environment the Origin JSSDK should run in.
         * @property {overrideUrlObject=} assign urls to the specific properties will override them.
         */

        /**
         * initialization function for the Origin JSSDK
         * @method
         * @param {configObject} config Object containing configuration properties for the JSSDK
         * @return {promise} resolved indicates the initialization succeed, reject means the intialization failed
         */
        init: function(overrides) {

            /*jshint undef:false */
            var self = this;

            //init the core
            function initCore() {
                configService.setEnvironment(jssdkconfig.common.env); //save off the environment
                //need to call initEvents BEFORE initAuth since it hooks up to listen to events
                configService.setLocale(jssdkconfig.common.locale);

                jssdk.auth = auth;
                jssdk.user = user.publicObjs;
                jssdk.events = events;
                jssdk.defines = defines;
                jssdk.locale = configService;
            }

            function initModules() {
                jssdk.feeds = feeds;
                jssdk.client = client;
                jssdk.games = games;
                jssdk.settings = settings;
                jssdk.atom = atom;
                jssdk.avatar = avatar;
                jssdk.xmpp = xmpp;
                jssdk.search = search;
                jssdk.voice = voice;
				jssdk.groups = groups;
            }

            utils.mix(jssdkconfig, overrides);

            initCore();

            //waits for the promise to resolve then calls init modules before return a promise for the Origin.init call
            return urls.init().then(initModules);
        },

        /** Tells if the bridge is available or not
        @method
        @returns {boolean}
        */
        bridgeAvailable: utils.bridgeAvailable
    };

    //we want these available even before Origin.init happens
    jssdk.log = logger.publicObjs;
    jssdk.utils = utils;
    return jssdk;
});