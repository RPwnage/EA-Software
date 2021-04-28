/**
 * @file home/recommended/lastgameplayed.js
 */
(function() {
    'use strict';

    function OriginHomeRecommendedActionLastgameplayedCtrl($scope, GamesDataFactory, LocFactory, ComponentsLogFactory) {
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
                var gameName = response[$scope.offerId].displayName;
                var title = $scope.titleStr || gameName;
                $scope.title = LocFactory.substitute(title, {
                    '%game%': gameName
                });

                $scope.image = $scope.image || response[$scope.offerId].packArt;

            })
            .catch(function(error) {
                ComponentsLogFactory.error('[origin-home-recommended-action-lastgameplayed] GamesDataFactory.getCatalogInfo', error.stack);
            });
    }

    function originHomeRecommendedActionLastgameplayed(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@',
                titleStr: '@title',
                offerId: '@offerid'
            },
            controller: 'OriginHomeRecommendedActionLastgameplayedCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionLastgameplayed
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl|OCD} image the link to the image for the tile, if not passed in it will use a default image
     * @param {LocalizedString|OCD} gamename the name of the game, if not passed it will use the name from catalog
     * @param {string} offerid the offerid of the game
     * @description
     *
     * last game played recommended next action
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-lastgameplayed image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_masseffect3_long.png" offerid="OFB-EAST:57198"></origin-home-recommended-action-lastgameplayed>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionLastgameplayedCtrl', OriginHomeRecommendedActionLastgameplayedCtrl)
        .directive('originHomeRecommendedActionLastgameplayed', originHomeRecommendedActionLastgameplayed);
}());