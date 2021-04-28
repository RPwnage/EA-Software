/* UpdateAppFactory
 *
 * manages checking for a new version of the SPA
 * @file factories/updateapp.js
 */
(function() {
    'use strict';


    function UpdateAppFactory($timeout, APP_VERSION, AuthFactory, GamesDataFactory, ComponentsLogFactory) {
        var APP_UPDATE_CHECK_TIMEOUT = 60*60*1000, //interval in ms to check for revision
            HTTP_REQUEST_TIMEOUT = 30000,

            myEvents = new Origin.utils.Communicator(),
            appDirty = false,
            playingGame = false,
            timeoutHandle = null;


        function canCheckAppVersion() {
            //don'e need to check if it's already dirty
            if (appDirty) {
                return false;
            }

            //don't need to check if offline
            if (!AuthFactory.isClientOnline()) {
                return false;
            }

            //check and see if we're playing a game
            if (playingGame) {
                return false;
            }

            return true;
        }

        function handleAppVersionResponse(response) {
            var version;
            if (response) {
                version = response.trim();  //remove LF that exists in the response
                return (version !== APP_VERSION);
            }
            return false;
        }

        //couldn't retrieve appVersion for some reason, so just return false
        function handleAppVersionError(error) {
            ComponentsLogFactory.error('handleAppVersionError', error);
            return false;
        }

        function newAppVersion() {
            //need to get location and version.txt and compare against APP_VERSION
            var spaHost = 'https://'+window.location.host,
                spaVersionUrl = spaHost + '/version.txt',
                config = {
                    atype: 'GET',
                    headers: [],
                    parameters: [],
                    appendparams: [],
                    reqauth: false,
                    requser: false
                };

                config.timeout = HTTP_REQUEST_TIMEOUT;
                config.noconvert = true;

            return Origin.dataManager.enQueue(spaVersionUrl, config)
                .then(handleAppVersionResponse)
                .catch(handleAppVersionError);
        }

        function checkForNewAppVersion() {
            if (canCheckAppVersion()) {
                newAppVersion()
                    .then(function(newExists) {
                        appDirty = newExists;
                        if (appDirty) {
                            //trigger event to show globalstripe
                            myEvents.fire('showNewSPAAvailable');
                        }
                    });
            }
        }

        function onFinishedPlaying() {
            playingGame = false;
            checkForNewAppVersion();
        }

        function setPlayingGame() {
            playingGame = true;
        }

        GamesDataFactory.events.on('games:finishedPlaying', onFinishedPlaying);
        GamesDataFactory.events.on('games:startedPlaying', setPlayingGame);
        timeoutHandle = setInterval(checkForNewAppVersion, APP_UPDATE_CHECK_TIMEOUT);

        return {
            events: myEvents
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function UpdateAppFactorySingleton($timeout, APP_VERSION, AuthFactory, GamesDataFactory, ComponentsLogFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('UpdateAppFactory', UpdateAppFactory, this, arguments);
    }
    /**
     * @ngdoc service
     * @name originApp.factories.UpdateAppFactory
     * @requires $http
     * @requires origin-components.factories.AppCommFactory
     * @description
     *
     * UpdateApp factory
     */
    angular.module('origin-components')
        .factory('UpdateAppFactory', UpdateAppFactorySingleton);
}());