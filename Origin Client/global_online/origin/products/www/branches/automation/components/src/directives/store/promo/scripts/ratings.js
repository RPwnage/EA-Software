
/**
 * @file store/promo/scripts/ratings.js
 */
(function(){
    'use strict';

    function OriginStorePromoRatingsCtrl($scope) {
        $scope.$watch('model', function(newValue, oldValue) {
            if(newValue !== oldValue) {
                var gameRatignIcon = Origin.utils.getProperty($scope.model, ['gameRatingSystemIcon']),
                    matureRatedGame = !!gameRatignIcon && gameRatignIcon.indexOf('TWAR_18') !== -1 ? true : false;

                $scope.showRatingsArea = Origin.locale.countryCode() === 'TW' && matureRatedGame ? true : false;
            }
        });
    }

    function originStorePromoRatings(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: OriginStorePromoRatingsCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/ratings.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoRatings
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-promo-ratings>
     *     </origin-store-promo-ratings>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePromoRatingsCtrl', OriginStorePromoRatingsCtrl)
        .directive('originStorePromoRatings', originStorePromoRatings);
}());
