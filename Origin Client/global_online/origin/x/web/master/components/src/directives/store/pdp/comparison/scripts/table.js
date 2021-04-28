/**
 * @file store/pdp/comparison/scripts/table.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-comparison-table';

    /**
    * The controller
    */
    function OriginStorePdpComparisonTableCtrl($scope, UtilFactory, ProductFactory, ObjectHelperFactory, AppCommFactory) {
        var map = ObjectHelperFactory.map,
            getProperty = ObjectHelperFactory.getProperty,
            offerIds = map(getProperty('offerid'), $scope.offers);

        $scope.models = {};
        $scope.offerLength = $scope.offers.length;
        $scope.buyNow = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'buyNow');
        $scope.youOwnThis = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'youOwnThis');
        $scope.ownedVersionWeight = 0;

        function applyModelData(data) {
            var i = 0,
                editions = {};
            $scope.ownedVersionWeight = 0;
            $scope.models = data;

            ObjectHelperFactory.forEach(function(offer){
                if(offer.isOwned && $scope.ownedVersionWeight < offer.weight) {
                    $scope.ownedVersionWeight = offer.weight;
                }

                editions[i] = offer.editionName;
                i++;
            }, data);

            AppCommFactory.events.fire('storeComparisonTable:updateModels', editions);
        }

        ProductFactory.get(offerIds).attachToScope($scope, applyModelData);

        // method for child directives to get offer count
        this.getOfferCount = function() {
            return $scope.offers.length;
        };

        $scope.isPurchasable = function (offerId) {
            if ($scope.models[offerId] === undefined || $scope.models[offerId].isOwned) {
                return false;
            } else {
                return $scope.models[offerId].weight > $scope.ownedVersionWeight;
            }
        };
    }

    function originStorePdpComparisonTable(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                title: '@',
                subtitle: '@',
                offers: '=',
                flag: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/comparison/views/table.html'),
            controller: OriginStorePdpComparisonTableCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpComparisonTable
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title The main title for the comparison table
     * @param {LocalizedString} subtitle The subtitle for the comparison table
     * @param {String} offers A list of offer ids.
     * @param {LocalizedString} flag An optional flag to show over the right most offer in the table
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-comparison-table
     *          title="Some title"
     *          subtitle="Some subtitle"
     *          offers="[{offerid: 'OFB-EAST:109546867'},{offerid: 'OFB-EAST:109549060'},{offerid: 'OFB-EAST:109552316'}]"
     *          flag="Most Popular">
     *     </origin-store-pdp-comparison-table>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpComparisonTableCtrl', OriginStorePdpComparisonTableCtrl)
        .directive('originStorePdpComparisonTable', originStorePdpComparisonTable);
}());