/**
 * @file home/recommended/preload.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-recommended-action-preload';

    function OriginHomeRecommendedActionPreloadCtrl($scope, GamesDataFactory, LocFactory, UtilFactory, ComponentsLogFactory) {
        // get the client data
        var game = GamesDataFactory.getClientGame($scope.offerId);
        $scope.isDownloading = (typeof(game) !== 'undefined') ? (game.downloading || game.installing) : false;
        $scope.offerBased = true;
        $scope.displayFriends = true;
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
            .then(function(response) {
                var gameName = response[$scope.offerId].displayName,
                    game = {
                        '%game%': gameName
                    },
                    title = $scope.titlestr || gameName;

                $scope.title = LocFactory.substitute(title, game);

                //tile specific string fallback
                $scope.subtitle = UtilFactory.getLocalizedStr($scope.subtitlestr, CONTEXT_NAMESPACE, 'subtitle', game);

                $scope.description = UtilFactory.getLocalizedStr($scope.descriptionstr, CONTEXT_NAMESPACE, 'description', game) + ' ';

                $scope.image = $scope.image || response[$scope.offerId].packArt;

            })
            .catch(function(error) {
                ComponentsLogFactory.error('[origin-home-recommended-action-preload] GamesDataFactory.getCatalogInfo', error.stack);
            });

    }

    function originHomeRecommendedActionPreload(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@image',
                title: '@title',
                subtitle: '@subtitle',
                description: '@description',
                offerId: '@offerid'
            },
            controller: 'OriginHomeRecommendedActionPreloadCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionPreload
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl|OCD} image the link to the image for the tile, if not passed in it will use a default image
     * @param {LocalizedString|OCD} gamename the name of the game, if not passed it will use the name from catalog
     * @param {LocalizedString|OCD} subtitle the subtitle for the tile
     * @param {LocalizedString|OCD} description the description for the tile
     * @param {string} offerid the offerid for the tile
     * @description
     *
     * origin-home-recommended-action-preload recommended next action
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionPreloadCtrl', OriginHomeRecommendedActionPreloadCtrl)
        .directive('originHomeRecommendedActionPreload', originHomeRecommendedActionPreload);
}());