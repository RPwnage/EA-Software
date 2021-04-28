/**
 * @file scripts/ocdtest.js
 */
(function () {
    'use strict';

    function originStoreOcdtest(ComponentsConfigFactory, GamesDataFactory, ProductFactory, OcdPathFactory) {

        function originStoreOcdTestLink(scope) {

            scope.ocd = {
                path: '/games/dragonage/dragon-age-origins/standard-edition',
                offerId: 'OFB-EAST:51582'
            };

            scope.callOcd = function (ocdPath) {
                OcdPathFactory.get(ocdPath || scope.ocd.path).attachToScope(scope, function (offer) {
                    console.log('OcdPathHelper.getOffersFromPaths ' + scope.ocd.path, offer);
                    scope.data = offer;
                });
            };

            scope.getOffer = function() {
                ProductFactory.get(scope.ocd.offerId).attachToScope(scope, function (offer) {
                    console.log('ProductFactory.get ' + scope.ocd.offerId, offer);
                    scope.data = offer;
                });
            };

            scope.getOcdByOfferId = function() {
                GamesDataFactory.getOcdByOfferId(scope.ocd.offerId).then(function(ocd) {
                    console.log('GamesDataFactory.getOcdFromOffer ' + scope.ocd.offerId, ocd);
                    scope.data = ocd;
                    scope.$digest();
                });
            };

            scope.getOcdResponseByOffer = function() {
                GamesDataFactory.getOcdByOfferId(scope.ocd.offerId).then(function(ocd) {
                    if (ocd.gamehub.keys.sling) {
                        scope.callOcd(ocd.gamehub.keys.sling);
                    } else {
                        scope.data = 'didn\'t find anything sir!!';
                        scope.$digest();
                    }
                });
            };

            scope.getOcdPathByOffer = function() {
                GamesDataFactory.getOcdByOfferId(scope.ocd.offerId).then(function(ocd) {
                    if (ocd.gamehub.keys.sling) {
                        scope.data = ocd.gamehub.keys.sling;
                    } else {
                        scope.data = 'didn\'t find anything sir!!';
                    }
                    scope.$digest();
                });
            };

        }

        return {
            restrict: 'E',
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/ocdtest.html'),
            link: originStoreOcdTestLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreOcdtest
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     *
     */
    angular.module('origin-components')
        .directive('originStoreOcdtest', originStoreOcdtest);
}());