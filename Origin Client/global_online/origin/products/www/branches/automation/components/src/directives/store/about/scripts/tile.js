/**
 * @file store/about/scripts/tile.js
 */
(function () {
    'use strict';

    function originStoreAboutTile(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            replace: true,
            scope: {
                titleStr: '@',
                tileContent: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/tile.html')
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutTile
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str Main heading for the tile
     * @param {LocalizedText} tile-content Content to go inside the tile (HTML)
     *
     * @description Simple tile for arbitrary content to go in any section in About
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-about-tile title-str="Lorem Ipsum" tile-content="Lorem ipsum dolor sit amet."></origin-store-about-tile>
     *     </file>
     * </example>
     */

    angular.module('origin-components')
        .directive('originStoreAboutTile', originStoreAboutTile);
}());
