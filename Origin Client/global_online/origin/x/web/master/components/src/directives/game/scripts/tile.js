/**
 * @file game/library/tile.js
 */

(function() {

    'use strict';

    function OriginGameTileCtrl($scope, $rootScope, UtilFactory, GamesDataFactory, GamesContextFactory, ComponentsLogFactory) {

        var game = GamesDataFactory.getClientGame($scope.offerId);
        var CONTEXT_NAMESPACE = 'origin-game-tile';

        /* Get Translated Strings */
        $scope.strings = {
            detailsButtonLabel: UtilFactory.getLocalizedStr($scope.detailsctaStr, CONTEXT_NAMESPACE, 'detailscta')
        };

        /**
         * Determine if the game is currently in download queue
         * @return {Boolean}
         * @method determineIfDownloading
         */
        function isInOperationQueue() {
            if (game && game.progressInfo) {
                return (game.progressInfo.active || game.completedIndex > -1);
            }
            return false;
        }

        // disable when you are not on the same platform
        // or if your subscription has expired
        $scope.isDisabled = (Math.random() > 0.5);
        $scope.isQueued = isInOperationQueue();

        /**
         * extracts a single offer out from the catalog response
         * @param  {object} response the catalog response
         */
        function extractSingleResponse(response) {
            return response[$scope.offerId];
        }
        /**
         * Stores the product information to the scope after
         * doing some transforms to the clientActions
         * @return {void}
         * @method storeProductInfo
         */
        function storeProductInfo(data) {
            $scope.packArt = data.i18n.packArtLarge;
            $scope.displayName = data.i18n.displayName;
            $scope.$digest();
        }

        /**
         * If the game is downloading then make sure we show the game info
         * @return {void}
         * @method updateDownloadState
         */
        function updateDownloadState() {
            $scope.isQueued = isInOperationQueue();
            $scope.$digest();
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
        
        /**
         * Goes to respectful OGD page. Removes item from complete list.
         * @return {object}
         */
        $scope.onTileClick = function () {
            if(typeof(game) !== 'undefined' && game.completedIndex > -1) {
                Origin.client.contentOperationQueue.remove($scope.offerId);
            }
            GamesDataFactory.showGameDetailsPageForOffer($scope.offerId);
        };

        // when the element is destroyed make sure that we cleanup
        $scope.$on('$destroy', function() {
            GamesDataFactory.events.off('games:update:' + $scope.offerId, updateDownloadState);
            GamesDataFactory.events.off('games:downloadqueueupdate:' + $scope.offerId, updateDownloadState);
            GamesDataFactory.events.off('games:catalogUpdated:' + $scope.offerId, storeProductInfo);            
        });

        GamesDataFactory.getCatalogInfo([$scope.offerId])
            .then(extractSingleResponse)
            .then(storeProductInfo)
            .catch(function(error) {
                ComponentsLogFactory.error('[origin-game-tile GamesDataFactory.getCatalogInfo] error', error.stack);
            });

        GamesDataFactory.events.on('games:update:' + $scope.offerId, updateDownloadState);
        GamesDataFactory.events.on('games:downloadqueueupdate:' + $scope.offerId, updateDownloadState);
        GamesDataFactory.events.on('games:catalogUpdated:' + $scope.offerId, storeProductInfo);
    }

    function originGameTile(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                title: '@title',
                hoursPlayed: '@hoursplayed',
                showDetailsButton: '@showdetailsbutton',
                detailsctaStr: '@detailsctastr'
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
     * @param {string} offerid - the offerid of the game you want to interact with
     * @param {string} title -  the title of the game you want to interact with
     * @param {number} hoursPlayed - hours of game play
     * @param {string} showDetailsButton - boolean, should we show the details cta?
     * @param {LocalizedString} detailsctaStr - details cta button label
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