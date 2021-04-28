/**
 * @file /store/game/scripts/rating.js
 */
(function() {
    'use strict';

    function OriginStoreGameRatingCtrl($scope, ProductFactory) {

        $scope.$watch('offerId', function (newValue) {
            if (newValue) {
                ProductFactory.get(newValue).attachToScope($scope, 'model');
            }
        });
    }

    function originStoreGameRating(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: '@offerid'
            },
            controller: 'OriginStoreGameRatingCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/game/views/rating.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreGameRating
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid The offer id for the ratings and legal text
     * @description
     *
     * Product details page call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-game-rating offerid="OFFER-123"></origin-store-game-rating>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreGameRatingCtrl', OriginStoreGameRatingCtrl)
        .directive('originStoreGameRating', originStoreGameRating);
}());
