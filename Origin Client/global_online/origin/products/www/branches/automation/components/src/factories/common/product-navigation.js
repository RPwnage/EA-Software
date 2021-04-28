/**
 *  * @file factories/common/client-nav-helper.js
 */
(function () {
    'use strict';

    var routeStateNameMap = {
        'route:home': 'app.home_home',
        'route:store': 'app.store.wrapper.home',
        'route:vault': 'app.store.wrapper.origin-access-vault',
        'route:settings': 'app.settings',
        'route:settings.page': 'app.settings.page',
        'route:mygames': 'app.game_gamelibrary',
        'route:profile': 'app.profile',
        'route:achievements': 'app.profile',
        'route:wishlist': 'app.profile.topic'
    };

    function ProductNavigationFactory(GamesDataFactory, NavigationFactory, PdpUrlFactory, ComponentsLogFactory) {


        function handleRouteChangeRequest(routeChangeInfo) {
            //omit drops the id from the routeChangeInfo object leaving us with just the params

            if(routeChangeInfo.id) {
                NavigationFactory.goToState(routeStateNameMap[routeChangeInfo.id], _.omit(routeChangeInfo, ['id']));
            } else {
                ComponentsLogFactory.error('ERROR:[NavigationFactory] route change from client must have an id property');
            }
        }

        function goToStoreByProductId(obj) {
            if (!!obj.productId) {
                GamesDataFactory.waitForGamesDataReady()
                    .then(GamesDataFactory.getCatalogInfo([obj.productId])
                        .then(function (data) {
                            PdpUrlFactory.goToPdp(data[obj.productId]);
                        }));
            }
        }

        /**
         * Go to the OGD page if the game is owned, otherwise the store PDP
         */
        function goProductDetails(productId) {
            GamesDataFactory.getCatalogInfo([productId])
                .then(function (data) {
                    var baseGame = GamesDataFactory.getBaseGameEntitlementByMasterTitleId(data[productId].masterTitleId);
                    if (!!baseGame) {
                        NavigationFactory.goToState('app.game_gamelibrary.ogd', {
                        'offerid': baseGame.offerId
                        });
                    } else {
                        PdpUrlFactory.goToPdp(data[productId]);
                    }
            }) .catch(function() { ComponentsLogFactory.error("Failed to get Catalog Info");} );
        }

        /**
         * Go to the OGD page if the game is owned or if we own an entitlement with the same multiplayerId, otherwise the store PDP
         */
        function goProductDetailsWithMultiplayerId(productId, multiplayerId) {
            var ent = GamesDataFactory.getBaseGameEntitlementByMultiplayerId(multiplayerId);
            if (!!ent){
                // owned
                NavigationFactory.goToState('app.game_gamelibrary.ogd', {
                    'offerid': ent.offerId
                });
            } else {
                // not owned
                GamesDataFactory.getCatalogInfo([productId])
                    .then(function (data) {
                        PdpUrlFactory.goToPdp(data[productId]);
                    }) .catch(function() { ComponentsLogFactory.error("Failed to get Catalog Info");} );
            }
        }

        function goToStoreByMasterTitleId(obj) {
            GamesDataFactory.waitForGamesDataReady()
                .then(GamesDataFactory.getPurchasablePathByMasterTitleId(obj.masterTitleId, true)
                    .then(function (data) {
                        if (!!data) {
                            var params = data.split('/'),
                                parameters = {
                                    franchise: params[1],
                                    game: params[2],
                                    edition: params[3]
                                };
                            NavigationFactory.goToState('app.store.wrapper.pdp', parameters);
                        }
                    }));
        }

        function handleSendProductNavigationInitalizedError(error) {
            ComponentsLogFactory.error('ERROR:[ProductNavigationFactory]', error.message);
        }

        function init() {
            if(Origin.client.isEmbeddedBrowser()){
                Origin.events.on(Origin.events.CLIENT_NAV_ROUTECHANGE, handleRouteChangeRequest);
                Origin.events.on(Origin.events.CLIENT_NAV_STOREBYPRODUCTID, goToStoreByProductId);
                Origin.events.on(Origin.events.CLIENT_NAV_STOREBYMASTERTITLEID, goToStoreByMasterTitleId);
                Origin.events.on(Origin.events.CLIENT_NAV_OPEN_GAME_DETAILS, goProductDetails);
                //let the client know when we have our listeners ready, so that it can send origin2:// and
                //local host information
                Origin.client.info.sendProductNavigationInitialized()
                    .catch(handleSendProductNavigationInitalizedError);
            }
        }


        return {
            goToStoreByProductId: goToStoreByProductId,
            goToStoreByMasterTitleId: goToStoreByMasterTitleId,
            goProductDetails: goProductDetails,
            goProductDetailsWithMultiplayerId: goProductDetailsWithMultiplayerId,
            init: init
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ProductNavigationFactory
     * @description
     *
     * ProductNavigationFactory
     */
    angular.module('origin-components')
        .factory('ProductNavigationFactory', ProductNavigationFactory);
}());
