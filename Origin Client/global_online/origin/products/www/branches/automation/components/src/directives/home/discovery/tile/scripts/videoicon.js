/**
 * @file home/discovery/tile/videoicon.js
 */
(function() {
    'use strict';

    function originHomeDiscoveryTileVideoIcon(ComponentsConfigFactory, $timeout) {

        function originHomeDiscoverTileVideoIconLink(scope, elem) {

            // Check if the tile has video then make the CTA() always visisble. Timeout needed as the button renders in DOM
            // We have do this as we don't know beforehand if the tile has a video in it or not.
            $timeout(function() {
                scope.hasVideo = elem.parent().find('.origin-tile-overlay-content .otkicon-play').length;
            }, 0, false);
        }

        return {
            restrict: 'E',
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/videoicon.html'),
            link: originHomeDiscoverTileVideoIconLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileVideoIcon
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * programmable discovery tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-home-discovery-tile-video-icon></origin-home-discovery-tile-video-icon>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileVideoIcon', originHomeDiscoveryTileVideoIcon);
}());