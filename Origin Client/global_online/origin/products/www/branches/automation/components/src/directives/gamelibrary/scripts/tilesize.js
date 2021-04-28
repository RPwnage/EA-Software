/**
 * @file gamelibrary/tilesize.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-gamelibrary-tilesize';

    function OriginGamelibraryTilesizeCtrl($scope, AppCommFactory, UtilFactory, GameLibraryTileSizeFactory) {

        var tileSizeObserver = GameLibraryTileSizeFactory.getTileSizeObserver();
        tileSizeObserver.getProperty('tileSize').attachToScope($scope, 'gridSize');

        function onDestroy() {
            tileSizeObserver.detach();
        }

        $scope.$on('$destroy', onDestroy);

        $scope.updateGridTileSize = function(size) {
            GameLibraryTileSizeFactory.setTileSize(size);
        };

        $scope.isEmbeddedBrowser = Origin.client.isEmbeddedBrowser();

        $scope.localizedStrings = {
            smallTiles: UtilFactory.getLocalizedStr($scope.smalltilesstr, CONTEXT_NAMESPACE, 'smalltilesstr'),
            mediumTiles: UtilFactory.getLocalizedStr($scope.mediumtilesstr, CONTEXT_NAMESPACE, 'mediumtilesstr'),
            largeTiles: UtilFactory.getLocalizedStr($scope.largetilesstr, CONTEXT_NAMESPACE, 'largetilesstr')
        };

    }

    function originGamelibraryTilesize(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            scope: {
                smalltilesstr: '@',
                mediumtilesstr: '@',
                largetilesstr: '@'
            },
            controller: 'OriginGamelibraryTilesizeCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/tilesize.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryTilesize
     * @restrict E
     * @element ANY
     * @description game library tile size selector
     *
     * @param {LocalizedString} smalltilesstr Small tile size tooltip
     * @param {LocalizedString} mediumtilesstr Medium tile size tooltip
     * @param {LocalizedString} largetilesstr Large tile size tooltip
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-tilesize></origin-gamelibrary-tilesize>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryTilesizeCtrl', OriginGamelibraryTilesizeCtrl)
        .directive('originGamelibraryTilesize', originGamelibraryTilesize);
}());
