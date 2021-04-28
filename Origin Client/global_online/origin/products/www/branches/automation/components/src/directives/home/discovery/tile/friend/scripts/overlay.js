
/**
 * @file home/discovery/tile/friend/scripts/overlay.js
 */
(function(){
    'use strict';
    function originHomeDiscoveryTileFriendOverlay(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/friend/views/overlay.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileFriendOverlay
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-home-discovery-tile-friend-overlay />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originHomeDiscoveryTileFriendOverlay', originHomeDiscoveryTileFriendOverlay);
}());
