/**
 * @file common/gamesclient.js
 */
(function() {
    'use strict';

    function GamesClientFactory($timeout, ComponentsLogFactory) {

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

        //we fire a message here as received for a standalone browser since we cannot
        //differentiate whether the client is not online or just haven't gone online yet.
        //use timeout only for external browser; for embedded wait for the actual signal from the client
        if (!Origin.bridgeAvailable()) {
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

        function requestGamesStatus() {
            //request initial status
            Origin.client.games.requestStatus().then(handleUpdateGamesStatus);
            ComponentsLogFactory.log('GamesClient: requested status');
        }

        function onJSSDKlogout() {
            games = {}; //clear existing game state
        }

        function onJSSDKlogin() {
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

        function init() {
            //listen for status changes
            Origin.events.on(Origin.events.CLIENT_GAMES_CHANGED, handleUpdateGamesStatus);
            Origin.events.on(Origin.events.CLIENT_GAMES_PROGRESSCHANGED, handleUpdateProgressStatus);
            Origin.events.on(Origin.events.CLIENT_GAMES_DOWNLOADQUEUECHANGED, handleUpdateDownloadQueueStatus);


            //set up to listen for login/logout to clear the roster
            Origin.events.on(Origin.events.AUTH_LOGGEDOUT, onJSSDKlogout);
            Origin.events.on(Origin.events.AUTH_SUCCESSRETRY, onJSSDKlogin);

            //if not yet logged in then onJSSDKlogin will get called
            //otherwise check to see if xmpp is connected, and if that's also done, then check for roster loading
            if (Origin.auth.isLoggedIn()) {
                if (!Origin.xmpp.isConnected()) {
                    onJSSDKlogin();
                } else {
                    requestGamesStatus();
                }
            }
        }


        init();

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
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GamesClientFactorySingleton($timeout, ComponentsLogFactory, SingletonRegistryFactory) {
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
