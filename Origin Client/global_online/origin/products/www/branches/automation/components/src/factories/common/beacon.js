/**
 * @file beacon.js
 */
(function() {
    'use strict';

    function BeaconFactory(CacheFactory, ObjectHelperFactory) {
        var BEACON_CACHE_TIME_SECONDS = 60,
            hasTruth = ObjectHelperFactory.hasTruth,
            toBoolean = ObjectHelperFactory.toBoolean,
            states = {
                RUNNING: 'running',
                INSTALLED: 'installed',
                INSTALLABLE: 'installable',
                INCOMPATIBLE: 'incompatible'
            },
            stateOrder = [states.RUNNING, states.INSTALLED, states.INSTALLABLE, states.INCOMPATIBLE];

        /**
         * Request installed and version number from the jssdk
         * @return {Promise.<String, Error>} [description]
         */
        var installed = function() {
            return Origin.beacon.installed();
        };

        /**
         * Request running state from the JSSDK
         * @return {Promise.<Boolean, Error>}
         */
        var running = function() {
            return Origin.beacon.running();
        };

        var cachedInstalled = CacheFactory.decorate(installed, BEACON_CACHE_TIME_SECONDS);

        var cachedRunning = CacheFactory.decorate(running, BEACON_CACHE_TIME_SECONDS);

        /**
         * Determine if the Origin Client is installed on this machine by querying the
         * Origin web helper
         * @param {Boolean} refresh get a fresh result from the cache
         * @return {Promise.<Boolean, Error>}
         */
        function isInstalled(refresh) {
            if(Origin.client.isEmbeddedBrowser()) {
                return Promise.resolve(true);
            }

            return cachedInstalled(CacheFactory.refresh(refresh))
                .then(toBoolean);
        }

        /**
         * Determine if the Origin Client is actively running on this machine by querying the
         * Origin web helper
         * @param {Boolean} refresh get a fresh result from the cache
         * @return {Promise.<Boolean, Error>}
         */
        function isRunning(refresh) {
            if(Origin.client.isEmbeddedBrowser()) {
                return Promise.resolve(true);
            }

            return cachedRunning(CacheFactory.refresh(refresh));
        }

        /**
         * Check the installable call on the jssdk is true otherwise try pinging the service
         * @param {Boolean} installable the installable response
         * @param {Boolean} refresh get a fresh result from the cache
         * @return {Promise.<Boolean, Error>}
         */
        function handleInstallable(installable, refresh) {
            if(installable) {
                return true;
            } else {
                return Promise.all([
                    isInstalled(refresh),
                    isRunning(refresh),
                ]).then(hasTruth);
            }
        }

        /**
         * Determine if this is a compatible platform for the Origin client.
         * -immediate return if the user is already in an embeddedclient context
         * -if the the user agent matches the minimum platfrom reqs
         * -queries beacon for running/installed in case the UA is malformed or out of date.
         * @param {Boolean} refresh force a refresh of the client state
         * @return {Promise.<Boolean, Error>}
         */
        function isInstallable(refresh) {
            if(Origin.client.isEmbeddedBrowser()) {
                return Promise.resolve(true);
            }

            return Origin.beacon.installable()
                .then(_.partial(handleInstallable, _, refresh));
        }

        /**
         * Pass through to is installable on catalog platform
         * @param  {string} platform the catalog platform enum eg. MAC/PCWIN
         * @return {Promise.<Boolean, Error>}
         */
        function isInstallableOnPlatform(platform) {
            return Origin.beacon.installableOnPlatform(platform);
        }

        /**
         * With the information retrieved from Beacon, return the first match
         * in stateOrder as a string
         * @param  {Object} stateList The list of states discovered
         * @return {String} the matching state name
         */
        function chooseState(stateList) {
            if(stateList.indexOf(true) > -1) {
                return stateOrder[stateList.indexOf(true)];
            }

            return states.INCOMPATIBLE;
        }

        /**
         * Get the current Beacon state as a string literal
         * @param {Boolean} refresh force a refresh of the client state
         * @return {Promise.<Object, Error>}
         */
        function getState(refresh) {
            if(Origin.client.isEmbeddedBrowser()) {
                return Promise.resolve(states.RUNNING);
            }

            return Promise.all([
                isRunning(refresh),
                isInstalled(refresh),
                isInstallable(refresh),
            ]).then(chooseState);
        }

        /**
         * Get the client version from the beacon service
         * @param {Boolean} refresh force a refresh of the client state
         * @return {Promise.<string, error>} the version number
         */
        function getClientVersion(refresh) {
            if(Origin.client.isEmbeddedBrowser() && Origin.client.info && _.isFunction(Origin.client.info.version)) {
                return Promise.resolve(Origin.client.info.version());
            }

            return cachedInstalled(CacheFactory.refresh(refresh));
        }

        return {
            isInstalled: isInstalled,
            isRunning: isRunning,
            isInstallable: isInstallable,
            isInstallableOnPlatform: isInstallableOnPlatform,
            getClientVersion: getClientVersion,
            getState: getState,
            states: states
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function BeaconFactorySingleton(CacheFactory, ObjectHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('BeaconFactory', BeaconFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.BeaconFactory
     * @description
     *
     * Beacon functions
     */
    angular.module('origin-components')
        .factory('BeaconFactory', BeaconFactorySingleton);
}());