(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-join-game';

    function JoinGameFactory(DialogFactory, GamesDataFactory, AppCommFactory, UtilFactory, AuthFactory) {

        function onClientOnlineStateChanged(onlineObj) {
            if (onlineObj && !onlineObj.isOnline) {
                DialogFactory.close('social-join-game-not-owned');
                DialogFactory.close('social-join-game-not-installed');
            }
        }

        //listen for connection change (for embedded)
        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);

        return {

            joinFriendsGame: function (nucleusId, productId, multiplayerId, gameTitle) {
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
                                title: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'unabletojoinstr'),
                                description: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'notownedstr').replace('%1', gameTitle),
                                acceptText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'viewinstorestr'),
                                acceptEnabled: true,
                                rejectText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'closestr'),
                                defaultBtn: 'yes'
                            });
                            AppCommFactory.events.on('dialog:hide', function (result) {
                                if (result.accepted) {
                                    GamesDataFactory.purchaseGame(productId);
                                }
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
                                title: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'unabletojoinstr'),
                                description: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'notinstalledstr').replace('%1', gameTitle),
                                acceptText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'downloadstr'),
                                acceptEnabled: true,
                                rejectText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'closestr'),
                                defaultBtn: 'yes'
                            });
                            AppCommFactory.events.on('dialog:hide', function (result) {
                                if (result.accepted) {
                                    Origin.client.games.startDownload(productId);
                                }
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
    function JoinGameFactorySingleton(DialogFactory, GamesDataFactory, AppCommFactory, UtilFactory, AuthFactory, SingletonRegistryFactory) {
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
