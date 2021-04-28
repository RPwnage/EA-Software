/**
 * @file store/pdp-url.js
 */
(function() {
    'use strict';

    function PdpUrlFactory($state, $stateParams, AppCommFactory, GamesCatalogFactory, NavigationFactory) {

        function getCurrentEditionSelector() {
            if ($stateParams.offerid) {
                return {
                    offerId: $stateParams.offerid
                };
            } else {

                var result =  {
                    franchiseKey: $stateParams.franchise || null,
                    gameKey: $stateParams.game || null,
                    editionKey: $stateParams.edition || null,
                    demo: false,
                    trial: false,
                    beta: false,
                    earlyAccess: false,
                    isPurchasable: true
                };

                result.ocdPath = buildPathFromUrl();

                return result;
            }
        }

        function buildPathFromUrl() {
            var path = null;

            if ($stateParams.franchise && $stateParams.game && $stateParams.edition) {
                path = '/' + $stateParams.franchise;
                path += '/' + $stateParams.game;
                if($stateParams.type) {
                    path += '/' + $stateParams.type;
                }
                path += '/' + $stateParams.edition;
            } else if($stateParams.offerid) {
                var data = GamesCatalogFactory
                            .getExistingCatalogInfo($stateParams.offerid);

                path = '/' + data.franchiseFacetKey;
                path += '/' + data.gameNameFacetKey;
                if(data.gameTypeFacetKey !== 'basegame') {
                    path += '/' + data.gameTypeFacetKey;
                }
                path += '/' + data.gameEditionType;
            }

            return path;
        }

        function getPdpRouteConfig(offerData) {
            var routeName, parameters;
            var ocdPath = offerData.freeBaseGame || offerData.ocdPath || offerData.offerPath;
            if (ocdPath) {
                var params = _.compact(ocdPath.split('/'));

                if (params.length === 4) { //dlc expansion etc
                    routeName = 'app.store.wrapper.addon';
                    parameters = {
                        franchise: params[0],
                        game: params[1],
                        type: params[2],
                        edition: params[3]
                    };

                } else if (params.length === 3) {
                    routeName = 'app.store.wrapper.pdp';
                    parameters = {
                        franchise: params[0],
                        game: params[1],
                        edition: params[2]
                    };
                }
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

        /**
         * Gets the PDP page URL given a offer model
         *
         * @param {object} offerData
         * @param {boolean} absoluteUrl will return absolute URL if this flag is set to true
         * @returns {*}
         */
        function getPdpUrl(offerData, absoluteUrl) {
            var routeAndParams = getPdpRouteConfig(offerData);
            var pdpUrl;

            if (routeAndParams.routeName) {
                pdpUrl = $state.href(routeAndParams.routeName, routeAndParams.parameters);
                if (absoluteUrl){
                    pdpUrl = NavigationFactory.getAbsoluteUrl(pdpUrl);
                }
                return pdpUrl;
            }

            return pdpUrl;
        }

        function goToPdp(offerData) {
            var routeAndParams = getPdpRouteConfig(offerData);

            if (routeAndParams.routeName) {
               AppCommFactory.events.fire('uiRouter:go', routeAndParams.routeName, routeAndParams.parameters);
            }
        }

        function getBrowsePageUrl(facetQuery) {
            return $state.href('app.store.wrapper.browse', {fq: facetQuery});
        }

        function isPdpRoute() {
            return $state.is('app.store.wrapper.pdp') || $state.is('app.store.wrapper.addon') || $state.is('app.store.wrapper.sku') ? true : false;
        }

        return {
            getCurrentEditionSelector: getCurrentEditionSelector,
            goToPdp: goToPdp,
            getPdpUrl: getPdpUrl,
            getBrowsePageUrl: getBrowsePageUrl,
            buildPathFromUrl: buildPathFromUrl,
            isPdpRoute : isPdpRoute,
            getPdpRouteConfig: getPdpRouteConfig
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.PdpUrlFactory

     * @description
     *
     * Store product factory
     */
    angular.module('origin-components')
        .factory('PdpUrlFactory', PdpUrlFactory);
}());
