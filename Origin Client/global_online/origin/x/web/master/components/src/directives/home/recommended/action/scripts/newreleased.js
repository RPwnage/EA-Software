/**
 * @file home/recommended/Newreleased.js
 */
(function() {
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-home-recommended-action-newreleased';

    function OriginHomeRecommendedActionNewreleasedCtrl($scope, $controller) {
        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and subtitle/description function (if needed)
        $controller('OriginHomeRecommendedActionCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customSubtitleAndDescriptionFunction: null
        });
    }

    function originHomeRecommendedActionNewreleased(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                imageRaw: '@image',
                offerId: '@offerid',
                gamename: '@',
                subtitleRaw: '@subtitle',
                priceOfferId: '@priceofferid'
            },
            controller: 'OriginHomeRecommendedActionNewreleasedCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionNewreleased
     * @restrict E
     * @element ANY
     * @param {ImageUrl|OCD} image the link to the image for the tile, if not passed in it will use a default image
     * @param {LocalizedString|OCD} gamename the name of the game, if not passed it will use the name from catalog
     * @param {LocalizedString} subtitle the subtitle for the tile
     * @param {OfferId} offerid the offerid of the game you want to interact with
     * @param {OfferId|OCD} priceofferid the offerid of the game you want to show pricing for

     * @scope
     *
     *
     * @description
     *
     * newly released recommended next action tile
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-newreleased image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_masseffect3_long.png" offerid="OFB-EAST:57198"></origin-home-recommended-action-newreleased>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionNewreleasedCtrl', OriginHomeRecommendedActionNewreleasedCtrl)
        .directive('originHomeRecommendedActionNewreleased', originHomeRecommendedActionNewreleased);
}());