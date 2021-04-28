/**
 * @file game/library/progress.js
 */
(function() {
    'use strict';

    function OriginGameProgressCtasCtrl($scope, GamesDataFactory) {

            $scope.downloadState = '';

            /**
             * Update the progress bar to complete or active
             * @return {void}
             * @method updateProgressBar
             */
            function getDownloadState(progressState) {
                var states = {
                    'State-Active': 'active',
                    'State-Paused': 'paused'
                };
                return states[progressState] || 'active';
            }

            /**
             * Stores the information needed to reflect the download status
             * @return {void}
             * @method storeProductInfo
             */
            function storeDownloadInfo() {
                var game = GamesDataFactory.getClientGame($scope.offerId);
                if (game && game.progressInfo && game.progressInfo.active) {
                    $scope.downloadState = getDownloadState(game.progressInfo.progressState);
                }
            }

            /**
             * Store the updated download information
             * @return {void}
             * @method updateDownloadInfo
             */
            function updateDownloadInfo() {
                storeDownloadInfo();
                $scope.$digest();
            }

            /**
            * Pause the download
            * @param {Event} $event
            * @return {Boolean}
            * @method pause
            */
            $scope.pause = function($event) {
                Origin.client.games.pauseDownload($scope.offerId);
                $event.preventDefault();
                $event.stopPropagation();
                return false;
            };

            /**
            * Resume the download
            * @param {Event} $event
            * @return {Boolean}
            * @method resume
            */
            $scope.resume = function($event) {
                Origin.client.games.resumeDownload($scope.offerId);
                $event.preventDefault();
                $event.stopPropagation();
                return false;
            };

            /**
            * Cancel the download
            * @param {Event} $event
            * @return {Boolean}
            * @method cancel
            */
            $scope.cancel = function($event) {
                Origin.client.games.cancelDownload($scope.offerId);
                $event.preventDefault();
                $event.stopPropagation();
                return false;
            };

            /**
            *
            * @return {void}
            * @method onDestroy
            */
            function onDestroy() {
                GamesDataFactory.events.off('games:update:' + $scope.offerId, updateDownloadInfo);
                GamesDataFactory.events.off('games:progressupdate:' + $scope.offerId, updateDownloadInfo);
            }

            storeDownloadInfo();

            // listen to any updates to the data
            GamesDataFactory.events.on('games:update:' + $scope.offerId, updateDownloadInfo);
            GamesDataFactory.events.on('games:progressupdate:' + $scope.offerId, updateDownloadInfo);
            $scope.$on('$destroy', onDestroy);


        }

        function originGameProgressCtas(ComponentsConfigFactory) {
            return {
                restrict: 'E',
                scope: {
                    offerId: '@offerid'
                },
                controller: 'OriginGameProgressCtasCtrl',
                templateUrl: ComponentsConfigFactory.getTemplatePath('game/views/progress-ctas.html')
            };
        }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameProgressCtas
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offerid of the game you want to interact with
     *
     * @description
     *
     * The call to action buttons that can optionally appear on a game progress
     *
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-progress-ctas offerid="OFB-EAST:109549060"></origin-game-progress-ctas>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameProgressCtasCtrl', OriginGameProgressCtasCtrl)
        .directive('originGameProgressCtas', originGameProgressCtas);
}());
