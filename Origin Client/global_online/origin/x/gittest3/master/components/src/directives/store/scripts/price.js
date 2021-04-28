/**
 * @file /store/scripts/price.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-price';

    function originStorePriceCtrl($scope, UtilFactory, GamesDataFactory) {

        $scope.isSubscriber = false;

        /* Set loading used for directive class */
        $scope.isLoading = true;
        $scope.ownedString = UtilFactory.getLocalizedStr($scope.ownedStr, CONTEXT_NAMESPACE, 'ownedstr');
        $scope.orString = UtilFactory.getLocalizedStr($scope.orStr, CONTEXT_NAMESPACE, 'orstr');
        $scope.freeString = UtilFactory.getLocalizedStr($scope.freeStr, CONTEXT_NAMESPACE, 'freestr');
        $scope.withString = UtilFactory.getLocalizedStr($scope.withStr, CONTEXT_NAMESPACE, 'withstr');
        $scope.zeroPriceString = UtilFactory.getLocalizedStr($scope.zeroPriceStr, CONTEXT_NAMESPACE, 'zeropricestr');


        function onGameData(data) {
            $scope.isOth = data[$scope.offerId].oth;
            $scope.isOriginAccessOffer = (Math.random() < 0.5);
            $scope.isFree = $scope.offerId.search("52") !== -1;
            $scope.price = "$19.95";
            $scope.strike = "$99.00";
            $scope.saleString = "Save 50%";
            $scope.isFree = $scope.isOth ? true : $scope.isFree;

            if (Origin && Origin.auth && Origin.auth.isLoggedIn()) {
                $scope.isOwned = GamesDataFactory.getEntitlement($scope.offerId) !== undefined;
            } else {
                $scope.isSubscriber = false;
                $scope.isOwned = false;
            }
            $scope.isLoading = false;
            $scope.$digest();
        }

        /* Get catalog data by offerId */
        /* This is faking the price service */
        GamesDataFactory.getCatalogInfo([$scope.offerId])
            .then(onGameData)
            .catch(function(error) {
                Origin.log.exception(error, 'origin-store-price - getGameInfo');
            });

        /* Update owned based on users entitlements */
        function onEntitlementUpdate() {
            $scope.isOwned = GamesDataFactory.getEntitlement($scope.offerId) !== undefined;
            $scope.isLoading = false;
        }

        function onDestroy() {
            GamesDataFactory.events.off('games:baseGameEntitlementsUpdate', onEntitlementUpdate);
        }

        /* Bind to entitlement updates */
        GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', onEntitlementUpdate);
        $scope.$on('$destroy', onDestroy);
    }

    function originStorePrice(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: '@offerid',
                ownedStr: "@ownedstr",
                orStr: "@orstr",
                freeStr: "@freestr",
                withStr: "@withstr",
                zeroPriceStr: "@zeropricestr",
            },
            controller: 'originStorePriceCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/price.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePrice
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offer id of the price requested
     * @description
     *
     * Shows Price for an Offer
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-price offerid="OFR.123"></origin-store-price>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('originStorePriceCtrl', originStorePriceCtrl)
        .directive('originStorePrice', originStorePrice);
}());