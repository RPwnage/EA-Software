/**
 * @file home/recommended/preload.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-recommended-action-preload';

    function OriginHomeRecommendedActionPreloadCtrl($scope, $controller) {
        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and subtitle/description function (if needed)
        $controller('OriginHomeRecommendedActionCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customSubtitleAndDescriptionFunction: null
        });
    }

    function originHomeRecommendedActionPreload(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                imageRaw: '@image',
                gamename: '@',
                subtitleRaw: '@subtitle',
                descriptionRaw: '@description',
                offerId: '@offerid',
                priceOfferId: '@priceofferid'
            },
            controller: 'OriginHomeRecommendedActionPreloadCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionPreload
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl|OCD} image the link to the image for the tile, if not passed in it will use a default image
     * @param {LocalizedString|OCD} gamename the name of the game, if not passed it will use the name from catalog
     * @param {LocalizedString} subtitle the subtitle for the tile
     * @param {LocalizedString} description the description for the tile
     * @param {OfferId} offerid the offerid of the game you want to interact with
     * @param {OfferId|OCD} priceofferid the offerid of the game you want to show pricing for

     * @description
     *
     * origin-home-recommended-action-preload recommended next action
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionPreloadCtrl', OriginHomeRecommendedActionPreloadCtrl)
        .directive('originHomeRecommendedActionPreload', originHomeRecommendedActionPreload);
}());