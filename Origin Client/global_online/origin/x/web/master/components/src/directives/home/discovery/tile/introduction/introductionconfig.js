/**
 * @file home/discovery/introduction/introductionconfig.js
 */
(function() {

    'use strict';

    function originHomeDiscoveryTileIntroductionConfig() {
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                title: '@',
                showfordays: '@',
            },
            template: '<div class="home-discovery-introduction-config-container" ng-transclude></div>'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileIntroductionConfig
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title The bucket name to give the collection of tiles
     * @param {Number} showfordays the number of days to show this tile after initial entitlement
     * @description
     *
     * This is an OCD configuration container for programmable tiles that introduces a newly entitled game
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-introduction-config title="Because you play Battlefield 4" showfordays="5">
     *            <origin-home-discovery-tile-config-programmable image="path/to/image.gif" description="BF4 Tips for beginners" placementtype="position" placementvalue="1">
     *                <origin-home-discovery-tile-config-cta actionid="url-external" href="https://www.example.com/bf4-tips.html" description="Read more" type="primary"></origin-home-discovery-tile-config-cta>
     *            </origin-home-discovery-tile-config-programmable>
     *         </origin-home-discovery-tile-introduction-config>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileIntroductionConfig', originHomeDiscoveryTileIntroductionConfig);
}());