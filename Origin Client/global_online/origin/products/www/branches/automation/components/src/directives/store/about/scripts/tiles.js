/**
 * @file store/about/scripts/tiles.js
 */
(function () {
    'use strict';

    /* Directive Declaration */
    function originStoreAboutTiles(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/tiles.html'),
            transclude: true
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutTiles
     * @restrict E
     * @element ANY
     * @scope
     *
     * @description Container for tiles in About page
     *
     * @example
     * <origin-store-about-tiles>
     *     <origin-store-about-tile></origin-store-about-tile>
     * </origin-store-about-tiles>
     */

    angular.module('origin-components')
        .directive('originStoreAboutTiles', originStoreAboutTiles);
}());
