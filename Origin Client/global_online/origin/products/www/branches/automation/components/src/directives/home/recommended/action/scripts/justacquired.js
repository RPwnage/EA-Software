/**
 * @file home/recommended/justacquired.js
 */
(function() {
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-home-recommended-action-justacquired';

    function OriginHomeRecommendedActionJustacquiredCtrl($scope, $controller) {
        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and subtitle/description function (if needed)
        $controller('OriginHomeRecommendedActionCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customSubtitleAndDescriptionFunction: null
        });
    }

    function originHomeRecommendedActionJustacquired(ComponentsConfigFactory) {
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
            controller: 'OriginHomeRecommendedActionJustacquiredCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionJustacquired
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} gamename override the name of the game from catalog if required
     * @param {LocalizedString} subtitle the subtitle for the tile - use %game% to place the game name in the string
     * @param {LocalizedString} description the description for the tile - use %game% to place the game name in the string
     * @param {string} offerid the offerid of the game you want to interact with
     * @param {OcdPath} ocd-path the ocd path of the game you want to interact with
     * @param {ImageUrl} discover-tile-image tile image 1000x250
     * @param {string} discover-tile-color the background color
     * @param {LocalizedString} section-title the text to show in the area title - use %game% to place the game name in the string
     * @param {LocalizedString} section-subtitle the text to show in the area subtitle
     * @description
     *
     * just acquired recommended next action
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-justacquired
     *             gamename="Battlefield Blitz"
     *             subtitle="A heading below the artwork"
     *             description="More details about the rna tile"
     *             offerid="OFB-EAST:abc123"
     *             ocd-path="path/to/ocd/content"
     *             discover-tile-image="http://www.example.com/foo.jpg"
     *             discover-tile-color="#FF00FF"
     *             section-title="You just aquired %game%"
     *             section-subtitle="<a href='game-library' class='otka'>Go to the library</a>"
     *         ></origin-home-recommended-action-justacquired>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionJustacquiredCtrl', OriginHomeRecommendedActionJustacquiredCtrl)
        .directive('originHomeRecommendedActionJustacquired', originHomeRecommendedActionJustacquired);
}());
