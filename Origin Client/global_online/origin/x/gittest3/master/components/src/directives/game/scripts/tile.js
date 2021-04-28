/**
 * @file game/library/tile.js
 */

(function() {

    'use strict';

    function OriginGameTileCtrl($scope, $rootScope, UtilFactory, GamesDataFactory, GamesContextFactory, ComponentsLogFactory) {

        var game = GamesDataFactory.getClientGame($scope.offerId);
        var CONTEXT_NAMESPACE = 'origin-game-tile';

        /**
         * Determine if the game is currently downloading
         * @return {Boolean}
         * @method determineIfDownloading
         */
        function downloadingOrQueued() {
            if (typeof(game) !== 'undefined') {
                return (game.downloading || game.updating || game.installing || game.repairing);
            }
            return false;
        }

        // disable when you are not on the same platform
        // or if your subscription has expired
        $scope.isDisabled = (Math.random() > 0.5);
        $scope.isDownloadingOrQueued = downloadingOrQueued();

        /**
         * Stores the product information to the scope after
         * doing some transforms to the clientActions
         * @return {void}
         * @method storeProductInfo
         */
        function storeProductInfo(data) {
            $scope.packArt = data.packArt;
            $scope.displayName = data.displayName;
            $scope.$digest();
        }

        /**
         * If the game is downloading then make sure we show the game info
         * @return {void}
         * @method updateDownloadState
         */
        function updateDownloadState() {
            $scope.isDownloadingOrQueued = downloadingOrQueued();
            $scope.$digest();
        }

        /**
         * Store the game information
         * @return {void}
         * @method handleGetMyGame
         */
        function handleGetMyGame(response) {
            storeProductInfo(response[$scope.offerId]);
        }

        /**
         * Translates labels in the given list of menu item objects
         * @return {object}
         */
        function translateMenuStrings(menuItems) {
            angular.forEach(menuItems, function(item) {
                item.label = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, item.label);
            });

            return menuItems;
        }

        /**
         * Generates list of context menu items available for this game
         * @return {object}
         */
        $scope.getAvailableActions = function() {
            return GamesContextFactory
                .contextMenu($scope.offerId)
                .then(translateMenuStrings);
        };


        // when the element is destroyed make sure that we cleanup
        $scope.$on('$destroy', function() {
            GamesDataFactory.events.off('games:update:' + $scope.offerId, updateDownloadState);
        });

        /**
         * Get the information needed for the game
         * @return {void}
         * @method getGameInfo
         */
        GamesDataFactory.getCatalogInfo([$scope.offerId])
            .then(handleGetMyGame)
            .catch(function(error) {
                ComponentsLogFactory.error('[origin-game-tile GamesDataFactory.getCatalogInfo] error', error.stack);
            });

        GamesDataFactory.events.on('games:update:' + $scope.offerId, updateDownloadState);
    }

    function originGameTile(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                title: '@title',
                hoursPlayed: '@hoursplayed',
            },
            controller: 'OriginGameTileCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameTile
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offerid of the game you want to interact with
     * @description
     *
     * game tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-tile offerid="OFB-EAST:109549060"></origin-game-tile>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameTileCtrl', OriginGameTileCtrl)
        .directive('originGameTile', originGameTile);
}());