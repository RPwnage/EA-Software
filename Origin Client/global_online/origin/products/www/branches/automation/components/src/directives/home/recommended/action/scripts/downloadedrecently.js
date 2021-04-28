/**
 * @file home/recommended/Downloadedrecently.js
 */
(function() {
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-home-recommended-action-downloadedrecently';

    function OriginHomeRecommendedActionDownloadedrecentlyCtrl($scope, $controller) {
        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and subtitle/description function (if needed)
        $controller('OriginHomeRecommendedActionCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customSubtitleAndDescriptionFunction: null
        });
    }

    function originHomeRecommendedActionDownloadedrecently(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                titleStr: '@title',
                offerId: '@offerid',
                ocdPath: '@',
                discoverTileImage: '@',
                discoverTileColor: '@',
                sectionTitle: '@',
                sectionSubtitle: '@',
                descriptionRaw: '@description'
            },
            controller: 'OriginHomeRecommendedActionDownloadedrecentlyCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionDownloadedrecently
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} gamename the name of the game, if not passed it will use the name from catalog
     * @param {string} offerid the offerid of the game you want to interact with
     * @param {OcdPath} ocd-path the ocd path of the game you want to interact with
     * @param {ImageUrl} discover-tile-image tile image 1000x250
     * @param {string} discover-tile-color the background color
     * @param {LocalizedString} section-title the text to show in the area title
     * @param {LocalizedString} section-subtitle the text to show in the area subtitle
     * @param {LocalizedString} description the description string
     *
     * @description
     *
     * last game played recommended next action
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-downloadedrecently image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_masseffect3_long.png" offerid="OFB-EAST:57198"></origin-home-recommended-action-downloadedrecently>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionDownloadedrecentlyCtrl', OriginHomeRecommendedActionDownloadedrecentlyCtrl)
        .directive('originHomeRecommendedActionDownloadedrecently', originHomeRecommendedActionDownloadedrecently);
}());
