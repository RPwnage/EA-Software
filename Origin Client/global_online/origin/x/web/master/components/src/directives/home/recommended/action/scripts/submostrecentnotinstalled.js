/**
 * @file home/recommended/Justacquired.js
 */
(function() {
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-home-recommended-action-submostrecentnotinstalled';

    function OriginHomeRecommendedActionSubmostrecentnotinstalled($scope, $controller) {
        function customSubtitleDescriptionFunction() {
            //we don't set a subtitle and description for this
        }

        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and subtitle/description function (if needed)
        $controller('OriginHomeRecommendedActionCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customSubtitleAndDescriptionFunction: customSubtitleDescriptionFunction
        });
    }

    function originHomeRecommendedActionSubmostrecentnotinstalled(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@',
                titleStr: '@title',
                offerId: '@offerid',
                priceOfferId: '@priceofferid'
            },
            controller: 'OriginHomeRecommendedActionSubmostrecentnotinstalled',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionSubmostrecentnotinstalled
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl|OCD} image the link to the image for the tile, if not passed in it will use a default image
     * @param {LocalizedString|OCD} gamename the name of the game, if not passed it will use the name from catalog
     * @param {OfferId} offerid the offerid of the game you want to interact with
     * @param {OfferId|OCD} priceofferid the offerid of the game you want to show pricing for

     * @description
     *
     * recently entitled vault game that's not installed recommended next action
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-submostrecentnotinstalled image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_masseffect3_long.png" offerid="OFB-EAST:57198"></origin-home-recommended-action-submostrecentnotinstalled>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionSubmostrecentnotinstalled', OriginHomeRecommendedActionSubmostrecentnotinstalled)
        .directive('originHomeRecommendedActionSubmostrecentnotinstalled', originHomeRecommendedActionSubmostrecentnotinstalled);
}());