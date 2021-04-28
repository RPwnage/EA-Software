/**
 * @file home/recommended/Justacquired.js
 */
(function() {
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-home-recommended-action-submostrecentnotinstalled';

    function OriginHomeRecommendedActionSubmostrecentnotinstalled($scope, $controller) {
        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and subtitle/description function (if needed)
        $controller('OriginHomeRecommendedActionCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customSubtitleAndDescriptionFunction: null
        });
    }

    function originHomeRecommendedActionSubmostrecentnotinstalled(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                titleStr: '@title',
                offerId: '@offerid',
                ocdPath: '@',
                discoverTileImage: '@',
                discoverTileColor: '@',
                sectionTitle: '@',
                subtitleRaw: '@subtitle',
                descriptionRaw: '@description',
                sectionSubtitle: '@'
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
     * @param {LocalizedString} gamename the name of the game, if not passed it will use the name from catalog
     * @param {string} offerid the offerid of the game you want to interact with
     * @param {OcdPath} ocd-path the path of the game you want to show pricing for
     * @param {ImageUrl} discover-tile-image tile image 1000x250
     * @param {string} discover-tile-color the background color
     * @param {LocalizedString} section-title the text to show in the area title
     * @param {LocalizedString} subtitle the subtitle for the tile
     * @param {LocalizedString} description the description string
     * @param {LocalizedString} section-subtitle the text to show in the area subtitle
     *
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
