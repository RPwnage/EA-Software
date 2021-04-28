(function() {
    'use strict';

    function JoinGameFactory(DialogFactory, GamesDataFactory, AuthFactory) {

        function onClientOnlineStateChanged(onlineObj) {
            if (onlineObj && !onlineObj.isOnline) {
                DialogFactory.close('social-join-game-not-owned');
                DialogFactory.close('social-join-game-not-installed');
            }
        }

        //listen for connection change (for embedded)
        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);

        function handleJoinGameNotOwned(productId) {
            return function(result) {
                if (result.accepted) {
                    GamesDataFactory.buyNow(productId);
                }
            };
        }

        function handleJoinGameNotInstalled(productId) {
            return function(result) {
                if (result.accepted) {
                    Origin.client.games.startDownload(productId);
                }
            };
        }

        return {

            joinFriendsGame: function (nucleusId, productId, multiplayerId, gameTitle, joinGameStrings) {
                var entitlement = null;

                if (!Origin.client.isEmbeddedBrowser()) {
                    // Nice try - maybe someday
                    return;
                }

                nucleusId = nucleusId;

                // First check to see if we own this game
                if (GamesDataFactory.ownsEntitlement(productId)) {
                    // owned
                    entitlement = GamesDataFactory.getEntitlement(productId);
                }
                else {
                    // See if we own a game with the same multiplayerId
                    var ent = GamesDataFactory.getBaseGameEntitlementByMultiplayerId(multiplayerId);
                    if (!!ent) {
                        // We own something with the same multiplayerId - use that
                        entitlement = ent;
                    }
                    else {
                        setTimeout(function(){
                            // not owned
                            DialogFactory.openPrompt({
                                id: 'social-join-game-not-owned',
                                title: joinGameStrings.unableToJoinText,
                                description: joinGameStrings.notOwnedText.replace('%game%', gameTitle),
                                acceptText: joinGameStrings.viewInStoreText,
                                acceptEnabled: true,
                                rejectText: joinGameStrings.closeText,
                                defaultBtn: 'yes',
                                callback: handleJoinGameNotOwned(productId)
                            });

                        }, 0);
                        return;
                    }
                }

                // See if it's installed
                if (!!entitlement) {

                    productId = entitlement.productId;

                    var installed = GamesDataFactory.isInstalled(productId);
                    if (installed) {
                        // *** Installed - JOIN! ***
                        Origin.xmpp.joinGame(nucleusId);
                    }
                    else {
                        setTimeout(function(){
                            // Not installed
                            DialogFactory.openPrompt({
                                id: 'social-join-game-not-installed',
                                title: joinGameStrings.unableToJoinText,
                                description: joinGameStrings.notInstalledText.replace('%1', gameTitle),
                                acceptText: joinGameStrings.downloadText,
                                acceptEnabled: true,
                                rejectText: joinGameStrings.closeText,
                                defaultBtn: 'yes',
                                callback: handleJoinGameNotInstalled(productId)
                            });
                        }, 0);
                        return;
                    }
                }


            }

        };
    }

     /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function JoinGameFactorySingleton(DialogFactory, GamesDataFactory, AuthFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('JoinGameFactory', JoinGameFactory, this, arguments);
    }
    /**
     * @ngdoc service
     * @name origin-components.factories.JoinGameFactory

     * @description
     *
     * JoinGameFactory
     */
    angular.module('origin-components')
        .factory('JoinGameFactory', JoinGameFactorySingleton);
}());
