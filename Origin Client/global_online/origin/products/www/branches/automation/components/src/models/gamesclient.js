/**
 * @file common/gamesclient.js
 */
(function() {
    'use strict';

    function GamesClientFactory($timeout, ComponentsLogFactory, AuthFactory) {

        var myEvents = new Origin.utils.Communicator(),
            games = {}, //holds status like playable, installed, etc.
            initialStateReceivedFlag = false,
            initialStateReceivedTimeout = 0; //set this time out to zero right now so that we don't wait for client info in standalone browser since theres no communication for rev1

        /**
         * Get the client data for the game
         * @return {Object}
         * @method getGame
         */
        function getGame(id) {
            if (games[id]) {
                return games[id];
            } else {
                games[id] = {};
                games[id].downloading = false;
                return games[id];
            }
        }

        /**
         * Get the client data for the game
         * @return {Promise}
         * @method getGamePromise
         */
        function getGamePromise(id) {
            return new Promise(function(resolve) {
                function resolveGetGame() {
                    resolve(getGame(id));
                }
                if (initialStateReceivedFlag) {
                    resolveGetGame();
                } else {
                    myEvents.once('GamesClientFactory:initialClientStateReceived', resolveGetGame);
                }
            });
        }

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

        //check and see if we're getting an update for play finished, if so, emit a signal
        function checkPlayFinished(prodId, updatedGame) {
            if (games[prodId].playing && !_.isUndefined(updatedGame.playing) && (updatedGame.playing !== null) && !updatedGame.playing) {
                myEvents.fire('GamesClientFactory:finishedPlaying', {
                    signal: 'finishedPlaying',
                    eventObj: null
                });
            }
        }

        //check and see if we're getting an update for play finished, if so, emit a signal
        function checkPlayStarted(prodId, updatedGame) {
            if (!_.get(games, [prodId, 'playing']) && _.get(updatedGame, ['playing'])) {
                myEvents.fire('GamesClientFactory:startedPlaying', {
                    signal: 'startedPlaying',
                    eventObj: null
                });
            }
        }        

        /**
         * @param <{Object}[]> a list of game status objects
         */
        function updateGameStatus(updatedGame) {
            var prodId = updatedGame.productId;
            if (games[prodId]) {
                checkPlayStarted(prodId, updatedGame);
                checkPlayFinished(prodId, updatedGame);
                Origin.utils.mix(games[prodId], updatedGame);
            } else {
                games[prodId] = updatedGame;
            }
        }

        /**
         * Fire an updateCloudUsage event
         * @param  {object}  gamesObject info from the C++ client
         * @method handleUpdateGamesCloudUsageStatus
         */
        function handleUpdateGamesCloudUsageStatus(gamesObject) {
            Object.keys(gamesObject).forEach(function(game) {
                myEvents.fire('GamesClientFactory:updateCloudUsage:' + game.productId, game);
            });
        }

        function handleUpdateGamesStatus(updatedGamesObj) {
            //empty response means that gamestatus is not ready, so hook up connection to listen for base games to finish load
            var refreshComplete = updatedGamesObj.refreshComplete,
                updatedGames = updatedGamesObj.updatedGames;

            if (refreshComplete === false) {
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
            handleUpdateGamesStatus({ refreshComplete: false, updatedGames: [] });
            ComponentsLogFactory.log('Origin.client.games.requestGamesStatus:not connected to client');
        }

        function requestGamesStatus() {
            //request initial status
            Origin.client.games.requestGamesStatus()
                .then(handleUpdateGamesStatus)
                .catch(handleRequestGamesStatusError);
            ComponentsLogFactory.log('GamesClient: requested status');
        }

        function onApplogout() {
            games = {}; //clear existing game state
        }

        function onAuthChanged(loginObj) {
            if (loginObj) {
                if (loginObj.isLoggedIn) {
                    requestGamesStatus();
                } else {
                    onApplogout();
                }
            }
        }

        function init(loginObj) {
            //listen for status changes
            Origin.events.on(Origin.events.CLIENT_GAMES_CHANGED, handleUpdateGamesStatus);
            Origin.events.on(Origin.events.CLIENT_GAMES_CLOUD_USAGE_CHANGED, handleUpdateGamesCloudUsageStatus);

            Origin.events.on(Origin.events.CLIENT_GAMES_PROGRESSCHANGED, handleUpdateProgressStatus);
            Origin.events.on(Origin.events.CLIENT_GAMES_OPERATIONFAILED, handleUpdateOperationFailedStatus);
            Origin.events.on(Origin.events.CLIENT_GAMES_DOWNLOADQUEUECHANGED, handleUpdateDownloadQueueStatus);


            AuthFactory.events.on ('myauth:change', onAuthChanged);

            //if not yet logged in then onJSSDKlogin will get called
            //otherwise check to see if xmpp is connected, and if that's also done, then request games status
            if (loginObj && loginObj.isLoggedIn) {
                requestGamesStatus();
            }
        }

        function handleAuthReadyError(error) {
            ComponentsLogFactory.error('GAMESCLIENTFACTORY - auth ready error:', error);
        }

        AuthFactory.waitForAuthReady()
            .then(init)
            .catch(handleAuthReadyError);

        return {

            events: myEvents,
            initialStateReceived: function() {
                return initialStateReceivedFlag;
            },

            getGame: getGame,
            getGamePromise: getGamePromise,

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
            isOperationActive: isOperationActive
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
