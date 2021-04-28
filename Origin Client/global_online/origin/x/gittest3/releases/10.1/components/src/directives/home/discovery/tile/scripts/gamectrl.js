/**
 * @file home/discovery/tile/gamectrl.js
 */
(function() {
    'use strict';

    function OriginHomeDiscoveryTileGameCtrl($scope, $sce, GamesDataFactory, DiscoveryStoryFactory, LocFactory, ComponentsLogFactory) {

        var game; // client data

        /**
         * Get the information needed for the game
         * @return {void}
         * @method getGameInfo
         */
        function getGameInfo() {
            // we will never have game info.
            GamesDataFactory.getCatalogInfo([$scope.offerId])
                .then(storeProductInfo)
                .catch(function(error) {
                    //if we have a passed in image from CQ we can use that even if retrieving catalog fails
                    if ($scope.image) {
                        $scope.art = $scope.image;
                    }
                    $scope.$digest();
                    ComponentsLogFactory.error('[OriginHomeDiscoveryTileGameCtrl GamesDataFactory.getCatalogInfo] error', error.stack);
                });

            // get the client data
            game = GamesDataFactory.getClientGame($scope.offerId);
            $scope.isDownloading = (typeof(game) !== 'undefined') ? (game.downloading || game.installing) : false;

        }

        /**
         * Store the product info
         * @param {Object} data - the data to store
         * @return {void}
         */
        function storeProductInfo(data) {
            $scope.name = data[$scope.offerId].displayName;
            $scope.description = $scope.description || '';
            $scope.description = $sce.trustAsHtml(LocFactory.substitute($scope.description, {
                    '%game%': $scope.name
                })

            );
            $scope.art = $scope.image || data[$scope.offerId].packArt;
            $scope.$digest();
        }

        /**
         * If the game is downloading then make sure we show the game info
         * @return {void}
         * @method updateDownloadState
         */
        function updateDownloadState() {
            if (typeof(game) !== 'undefined') {
                $scope.isDownloading = (game.downloading || game.installing) ? true : false;
                $scope.$digest();
            }
        }

        /**
        * clean up
        * @return {void}
        * @method onDestroy
        */
        function onDestroy() {
            GamesDataFactory.events.off('games:update:' + $scope.offerId, updateDownloadState);
        }

        /**
         * Initialization
         * @return {void}
         */
        this.init = function() {
            if ($scope.offerId) {
                getGameInfo();
            }
            GamesDataFactory.events.on('games:update:' + $scope.offerId, updateDownloadState);
            $scope.$on('$destroy', onDestroy);
        };

    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileGameController
     * @restrict E
     * @element ANY
     * @scope
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileGameCtrl', OriginHomeDiscoveryTileGameCtrl);

}());