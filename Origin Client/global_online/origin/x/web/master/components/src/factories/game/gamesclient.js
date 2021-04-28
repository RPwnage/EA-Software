/**
 * @file common/gamesclient.js
 */
(function() {
    'use strict';

    function GamesClientFactory($timeout, ComponentsLogFactory, AuthFactory) {

        var myEvents = new Origin.utils.Communicator(),
            games = {}, //holds status like playable, installed, etc.
            initialStateReceivedFlag = false,
            initialStateReceivedTimeout = 6000;


        function markInitialStateAsReceived() {
            if (!initialStateReceivedFlag) {
                initialStateReceivedFlag = true;
                myEvents.fire('GamesClientFactory:initialClientStateReceived', {
                    signal: 'initialClientStateReceived',
                    eventObj: null
                });
            }
        }

        /**
         * returns true if we are performing a download-like operation
         * @param  {object}  clientInfo info from the C++ client
         * @return {Boolean}            true if we are downloading/updating/installing/repairing
         */
        function isOperationActive(offerId) {
            var active = false,
                clientInfo = games[offerId];

            if (clientInfo) {
                active = (clientInfo.downloading || clientInfo.updating || clientInfo.installing || clientInfo.repairing);
            }

            return active;
        }

        //we fire a message here as received for a standalone browser since we cannot
        //differentiate whether the client is not online or just haven't gone online yet.
        //use timeout only for external browser; for embedded wait for the actual signal from the client
        if (!Origin.client.isEmbeddedBrowser()) {
            $timeout(markInitialStateAsReceived, initialStateReceivedTimeout, false);
        }

        /**
         * @param <{Object}[]> a list of game status objects
         */
        function updateGameStatus(updatedGame) {
            var prodId = updatedGame.productId;
            if (games[prodId]) {
                Origin.utils.mix(games[prodId], updatedGame);
            } else {
                games[prodId] = updatedGame;
            }
        }

        function handleUpdateGamesStatus(updatedGames) {
            //empty response means that gamestatus is not ready, so hook up connection to listen for base games to finish load
            if (Object.keys(updatedGames).length === 0) {
                Origin.events.on(Origin.events.CLIENT_GAMES_BASEGAMESUPDATED, requestGamesStatus);
            } else {

                Origin.events.off(Origin.events.CLIENT_GAMES_BASEGAMESUPDATED, requestGamesStatus);
                for (var id in updatedGames) {
                    var prodId = updatedGames[id].productId;

                    updateGameStatus(updatedGames[id]);
                    myEvents.fire('GamesClientFactory:update', {
                        signal: 'update:' + prodId,
                        eventObj: null
                    });
                }
                markInitialStateAsReceived();
            }
        }

        function handleUpdateProgressStatus(updatedGames) {
            for (var id in updatedGames) {
                var prodId = updatedGames[id].productId;
                updateGameStatus(updatedGames[id]);
                myEvents.fire('GamesClientFactory:progressupdate', {
                    signal: 'progressupdate:' + prodId,
                    eventObj: null
                });
            }
        }
        
        function handleUpdateOperationFailedStatus(updatedGames) {
            for (var id in updatedGames) {
                var prodId = updatedGames[id].productId;
                updateGameStatus(updatedGames[id]);
                myEvents.fire('GamesClientFactory:operationfailedupdate', {
                    signal: 'operationfailedupdate:' + prodId,
                    eventObj: null
                });
            }
        }

        function handleUpdateDownloadQueueStatus(updatedGames) {
            for (var id in updatedGames) {
                var prodId = updatedGames[id].productId;
                updateGameStatus(updatedGames[id]);
                myEvents.fire('GamesClientFactory:downloadqueueupdate', {
                    signal: 'downloadqueueupdate:' + prodId,
                    eventObj: null
                });
            }
        }

        function handleRequestGamesStatusError() {
            handleUpdateGamesStatus([]);
            ComponentsLogFactory.log('Origin.client.games.requestGamesStatus:not connected to client');
        }

        function requestGamesStatus() {
            //request initial status
            Origin.client.games.requestGamesStatus()
                .then(handleUpdateGamesStatus)
                .catch(handleRequestGamesStatusError);
            ComponentsLogFactory.log('GamesClient: requested status');
        }


        //STUBBED FUNCTIONS
        function repairNeeded(offerId) {
            //reference offerId so we don't get jshint error
            return offerId ? false : false;
        }

        function downloadOverride(offerId) {
            //reference offerId so we don't get jshint error
            return offerId ? false: false;
        }

        function newDLCreadyForInstall(offerId) {
            //reference offerId so we don't get jshint error
            return offerId ? false: false;
        }

        function newDLCinstalled(offerId) {
            //reference offerId so we don't get jshint error
            return offerId ? false: false;
        }

        function patched(offerId) {
            //reference offerId so we don't get jshint error
            return offerId ? false: false;
        }
        //END STUBBED FUNCTIONS

        function onApplogout() {
            games = {}; //clear existing game state
        }

        function checkXMPPconnection() {
            //now that we're logged in, initiate xmpp startup
            //but check to see if we're already connected to xmpp
            if (Origin.xmpp.isConnected()) {
                requestGamesStatus();
            } else {
                //setup listening fo the connection first
                Origin.events.on(Origin.events.XMPP_CONNECTED, requestGamesStatus);

                //since we're not going to be able to retrieve client game data, just mark it as having been received (same as if request timed out)
                Origin.events.on(Origin.events.XMPP_USERCONFLICT, markInitialStateAsReceived);
            }
        }

        function onAuthChanged(loginObj) {
            if (loginObj) {
                if (loginObj.isLoggedIn) {
                    checkXMPPconnection();
                } else {
                    onApplogout();
                }
            }
        }

        function init(loginObj) {
            //listen for status changes
            Origin.events.on(Origin.events.CLIENT_GAMES_CHANGED, handleUpdateGamesStatus);
            Origin.events.on(Origin.events.CLIENT_GAMES_PROGRESSCHANGED, handleUpdateProgressStatus);
            Origin.events.on(Origin.events.CLIENT_GAMES_OPERATIONFAILED, handleUpdateOperationFailedStatus);
            Origin.events.on(Origin.events.CLIENT_GAMES_DOWNLOADQUEUECHANGED, handleUpdateDownloadQueueStatus);


            AuthFactory.events.on ('myauth:change', onAuthChanged);

            //if not yet logged in then onJSSDKlogin will get called
            //otherwise check to see if xmpp is connected, and if that's also done, then check for roster loading
            if (loginObj && loginObj.isLoggedIn) {
                if (!Origin.xmpp.isConnected()) {
                    checkXMPPconnection();
                } else {
                    requestGamesStatus();
                }
            }
        }

        function handleAuthReadyError(error) {
            ComponentsLogFactory.error('GAMESCLIENTFACTORY - auth ready error:', error.message);
        }

        AuthFactory.waitForAuthReady()
            .then(init)
            .catch(handleAuthReadyError);

        return {

            events: myEvents,
            initialStateReceived: function() {
                return initialStateReceivedFlag;
            },
            /**
             * Get the client data for the game
             * @return {Object}
             * @method getGame
             */
            getGame: function(id) {
                if (games[id]) {
                    return games[id];
                } else {
                    games[id] = {};
                    games[id].downloading = false;
                    return games[id];
                }
            },

            getITOGame: function() {
                for (var id in games) {
                    if (games.hasOwnProperty(id)) {
                        if (games[id].ito) {
                            return id;
                        }
                    }
                }
                return null;
            },
            /**
             * is there a download like operation happening
             * @param {string} offerid the offer
             * @return {Boolean} true if a download update or repair is happening, false other wise
             */
            isOperationActive: isOperationActive,

            //***********************************************************
            //STUBBED FUNCTIONS
            //***********************************************************

            /**
            * returns true if the entitlement associated with the offer is in need of repair
            * @param {string} offerId
            * @return {boolean}
            * @method repairNeeded
            */
            repairNeeded: repairNeeded,

            /**
            * returns true if there's a download override
            * @param {string} offerId
            * @return {boolean}
            * @method downloadOverride
            */
            downloadOverride: downloadOverride,

            /**
            * returns true if the the entitlement is 'new', owned, and downloadable
             * @param {string} offerId
            * @param {int} days
            * @return {boolean}
            * @method newDLCreadyForInstall
            */
            newDLCreadyForInstall: newDLCreadyForInstall,

            /**
            * returns true if the entitlement is 'new', owned, and installed within days
            * @param {string} offerId
            * @param {int} days
            * @return {boolean}
            * @method newDLCinstalled
            */
            newDLCinstalled: newDLCinstalled,

            /**
            * returns true if the game was patched
            * @param {string} offerId
            * @param {int} days
            * @return {boolean}
            * @method patched
            */
            patched: patched

            //***********************************************************
            //END STUBBED FUNCTIONS
            //***********************************************************

        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesClientFactorySingleton($timeout, ComponentsLogFactory, AuthFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GamesClientFactory', GamesClientFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesClientFactory
     * @requires $timeout

     * @description
     *
     * GamesClientFactory
     */
    angular.module('origin-components')
        .factory('GamesClientFactory', GamesClientFactorySingleton);
}());
