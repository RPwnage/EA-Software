/**
 * @file store/pdp-url.js
 */
(function() {
    'use strict';

    function PdpUrlFactory($state, $stateParams, StoreMockDataFactory, AppCommFactory) {

        function getCurrentEditionSelector() {
            if ($stateParams.offerid) {
                return {
                    offerId: $stateParams.offerid
                };
            } else {
                return {
                    franchiseKey: $stateParams.franchise || null,
                    gameKey: $stateParams.game || null,
                    editionKey: $stateParams.edition || null
                };
            }
        }

        function getCurrentGameSelector() {
            var currentOffer;

            if ($stateParams.offerid) {
                currentOffer = StoreMockDataFactory.getMock($stateParams.offerid);

                return {
                    franchiseKey: currentOffer.franchiseKey || null,
                    gameKey: currentOffer.gameKey || null
                };
            } else {
                return {
                    franchiseKey: $stateParams.franchise || null,
                    gameKey: $stateParams.game || null
                };
            }
        }

        function getPdpRouteConfig(offerData) {
            var routeName, parameters;

            if (offerData.franchiseKey) {
                routeName = 'app.store.wrapper.pdp';
                parameters = {
                    franchise: offerData.franchiseKey,
                    game: offerData.gameKey,
                    edition: offerData.editionKey
                };
            } else if (offerData.offerId) {
                routeName = 'app.store.wrapper.skupdp';
                parameters = {
                    offerid: offerData.offerId
                };
            }

            return {
                routeName: routeName,
                parameters: parameters
            };
        }

        function getPdpUrl(offerData) {

            var routeAndParams = getPdpRouteConfig(offerData);

            if (routeAndParams.routeName) {
                return $state.href(routeAndParams.routeName, routeAndParams.parameters);
            }

            return '';
        }

        function goToPdp(offerData) {
            var routeAndParams = getPdpRouteConfig(offerData);

            if (routeAndParams.routeName) {
                AppCommFactory.events.fire('uiRouter:go', routeAndParams.routeName, routeAndParams.parameters);
            }
        }

        return {
            getCurrentEditionSelector: getCurrentEditionSelector,
            getCurrentGameSelector: getCurrentGameSelector,
            goToPdp: goToPdp,
            getPdpUrl: getPdpUrl
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function PdpUrlFactorySingleton($state, $stateParams, StoreMockDataFactory, AppCommFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('PdpUrlFactory', PdpUrlFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.PdpUrlFactory

     * @description
     *
     * Store product factory
     */
    angular.module('origin-components')
        .factory('PdpUrlFactory', PdpUrlFactorySingleton);
}());
