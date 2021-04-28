/**
 * @file home/recommended/Newreleased.js
 */
(function() {
    'use strict';

    function OriginHomeRecommendedActionNewreleasedCtrl($scope, GamesDataFactory) {
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

    }

    function originHomeRecommendedActionNewreleased(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@',
                title: '@title',
                subtitle: '@subtitle',
                description: '@description',
                offerId: '@offerid',
                friendsPlayingDescription: '@friendsplayingdescription'
            },
            controller: 'OriginHomeRecommendedActionNewreleasedCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionNewreleased
     * @restrict E
     * @element ANY
     * @param {ImageUrl|OCD} image the link to the image for the tile, if not passed in it will use a default image
     * @param {LocalizedString|OCD} gamename the name of the game, if not passed it will use the name from catalog
     * @param {LocalizedString|OCD} subtitle the subtitle for the tile
     * @param {LocalizedString|OCD} description the description for the tile
     * @param {string} offerid the offerid for the tile
     * @scope
     *
     *
     * @description
     *
     * newly released recommended next action tile
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-newreleased image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_masseffect3_long.png" offerid="OFB-EAST:57198"></origin-home-recommended-action-newreleased>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionNewreleasedCtrl', OriginHomeRecommendedActionNewreleasedCtrl)
        .directive('originHomeRecommendedActionNewreleased', originHomeRecommendedActionNewreleased);
}());