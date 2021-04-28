
/**
 * @file home/discovery/tile/game/scripts/overlay.js
 */
(function(){

    'use strict';

    function OriginHomeDiscoveryTileGameOverlayCtrl($scope, $controller) {
        $controller('OriginHomeDiscoveryTileGameCtrl', {
            $scope: $scope,
            contextNamespace: null,
            customDescriptionFunction: null
        });
    }

    function originHomeDiscoveryTileGameOverlay(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/game/views/overlay.html'),
            controller: OriginHomeDiscoveryTileGameOverlayCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileGameOverlay
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-home-discovery-tile-game-overlay />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileGameOverlayCtrl', OriginHomeDiscoveryTileGameOverlayCtrl)
        .directive('originHomeDiscoveryTileGameOverlay', originHomeDiscoveryTileGameOverlay);
}());
