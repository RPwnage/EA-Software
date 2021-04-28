/**
 * @file home/recommended/ito.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-recommended-action-ito';

    function OriginHomeRecommendedActionItoCtrl($scope, GamesDataFactory, UtilFactory, ComponentsLogFactory) {
        // get the client data
        var game = GamesDataFactory.getClientGame($scope.offerId);
        $scope.isDownloading = (typeof(game) !== 'undefined') ? (game.downloading || game.installing) : false;
        $scope.offerBased = true;

        /**
         * Updates game data in scope based on the game info API response
         * @param {Object} response response object
         * @return {void}
         */
        function handleCatalogInfoResponse(response) {
            var game = {
                '%game%': response[$scope.offerId].displayName
            };

            $scope.title = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'titlestr', game);
            $scope.description = UtilFactory.getLocalizedStr($scope.descriptionStr, CONTEXT_NAMESPACE, 'descriptionstr', game) + ' ';
            $scope.image = $scope.image || response[$scope.offerId].packArt;
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


        GamesDataFactory.events.on('games:update:' + $scope.offerId, updateDownloadState);
        $scope.$on('$destroy', onDestroy);

        GamesDataFactory.getCatalogInfo([$scope.offerId])
            .then(handleCatalogInfoResponse)
            .catch(function(error) {
                ComponentsLogFactory.error('[origin-home-recommended-action-ito] GamesDataFactory.getCatalogInfo', error.stack);
            });

    }

    function originHomeRecommendedActionIto(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@',
                titleStr: '@title',
                descriptionStr: '@description',
                offerId: '@offerid'
            },
            controller: 'OriginHomeRecommendedActionItoCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionIto
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl|OCD} image the link to the image for the tile, if not passed in it will use a default image
     * @param {LocalizedString|OCD} gamename the name of the game, if not passed it will use the name from catalog
     * @param {LocalizedString|OCD} descriptioninstalling the description shown when ITO is installing
     * @param {LocalizedString|OCD} descriptionreadytoplay the description shown when ITO is ready to play
     * @param {string} offerid the offerid of the game
     * @description
     *
     * ito recommended next action
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-ito image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile-bf4-long.png" offerid="OFB-EAST:109549060"></origin-home-recommended-action-ito>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionItoCtrl', OriginHomeRecommendedActionItoCtrl)
        .directive('originHomeRecommendedActionIto', originHomeRecommendedActionIto);
}());