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
                gamename: '@',
                subtitleRaw: '@subtitle',
                descriptionRaw: '@description',
                offerId: '@offerid',
                ocdPath: '@',
                discoverTileImage: '@',
                discoverTileColor: '@',
                sectionTitle: '@',
                sectionSubtitle: '@'
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
     * @param {LocalizedString} gamename the name of the game, if not passed it will use the name from catalog
     * @param {LocalizedString} subtitle the subtitle for the tile
     * @param {LocalizedString} description the description for the tile
     * @param {string} offerid the offerid of the game you want to interact with
     * @param {OcdPath} ocd-path the ocd path of the game you want to interact with
     * @param {ImageUrl} discover-tile-image tile image 1000x250
     * @param {string} discover-tile-color the background color
     * @param {LocalizedString} section-title the text to show in the area title
     * @param {LocalizedString} section-subtitle the text to show in the area subtitle
     *
     * @description
     *
     * origin-home-recommended-action-preload recommended next action
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionPreloadCtrl', OriginHomeRecommendedActionPreloadCtrl)
        .directive('originHomeRecommendedActionPreload', originHomeRecommendedActionPreload);
}());
