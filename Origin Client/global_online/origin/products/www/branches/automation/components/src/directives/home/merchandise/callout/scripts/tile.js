/**
 * @file home/merchandise/callout/tile.js
 */
(function() {
    'use strict';

    /**
     * Home Merchandise Tile Callout directive
     * @directive originHomeMerchandiseCalloutTile
     */
    var TextColorEnumeration = {
        "light": "light",
        "dark": "dark"
    };
    function originHomeMerchandiseCalloutTile(ComponentsConfigFactory) {
        function originHomeMerchandiseCalloutTileLink(scope){
            scope.textColor = TextColorEnumeration[scope.textColor] || TextColorEnumeration.dark;
            scope.textColorClass = 'origin-home-merchandise-text-color-' + scope.textColor;
        }
        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                ctid: '@',
                image: '@imagepath',
                headline: '@',
                text: '@',
                textColor: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/merchandise/callout/views/tile.html'),
            controller: 'OriginHomeMerchandiseTileCtrl',
            link: originHomeMerchandiseCalloutTileLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeMerchandiseCalloutTile
     * @restrict E
     * @element ANY
     * @scope
     * @param {String} ctid Campaign Targeting ID
     * @param {ImageUrl} imagepath image in background
     * @param {LocalizedString} headline title text
     * @param {LocalizedString} text description text
     * @param {TextColorEnumeration} text-color dark or light to match background image. Defaults to dark
     *
     * @description
     *
     * merchandise callout tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-merchandise-callout-tile headline="Go All In. Save 30% On Premium." text="Upgrade to all maps, modes, vehicles and more for one unbeatable price." imagepath="/content/dam/originx/web/app/games/batman-arkham-city/181544/tiles/tile_batman_arkham_long.jpg" alignment="right" theme="dark" class="ng-isolate-scope" ctid="campaing-id-1"></origin-home-merchandise-callout-tile>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeMerchandiseCalloutTile', originHomeMerchandiseCalloutTile);

}());