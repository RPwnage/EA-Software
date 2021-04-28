/**
 * @file game/library/progress.js
 */
(function() {
    'use strict';

    function OriginGameProgressCtrl($scope, GamesDataFactory) {

            //constants
            var MAX_PERCENT = 100,
                MAX_DEGREES = 360;

            //do not display unless the user is downloading
            $scope.isNotDownloading = true;
            $scope.downloadState = '';
            $scope.rotationDegrees = '0deg';

            /**
             * Update the progress bar to complete or active
             * @return {void}
             * @method updateProgressBar
             */
            function getDownloadState(progressState) {
                var state = '';
                switch (progressState) {
                    case 'State-Active':
                        if (typeof($scope.percent) !== 'undefined') {
                            if ($scope.percent === MAX_PERCENT) {
                                state = 'complete';
                            } else {
                                state = 'active';
                            }
                        }
                        break;
                    case 'State-Paused':
                        state = 'paused';
                    break;
                }
                return state;
            }

            /**
            * I need another degree! Just kidding, convert the
            * percent to the degree value for rotation
            * @param {Number} percent
            * @return {String} degree string
            * @method getDegrees
            */
            function getDegrees(percent) {
                return (Math.round(percent * MAX_DEGREES)) + 'deg';
            }

            /**
             * Stores the information needed to reflect the download status
             * @return {void}
             * @method storeProductInfo
             */
            function storeDownloadInfo() {

                // this stuff needs to be refactored big time..
                var game = GamesDataFactory.getClientGame($scope.offerId),
                    progressInfo, progressVal, prevDegrees = '';

                if (game) {
                    if (game.progressInfo && game.progressInfo.active) {
                        progressInfo = game.progressInfo;
                        progressVal = Math.floor(progressInfo.progress * MAX_PERCENT);
                        $scope.stateStr = progressInfo.phaseDisplay;
                        $scope.downloadState = getDownloadState(progressInfo.progressState);
                        $scope.percent = progressVal;
                        prevDegrees = $scope.rotationDegrees;
                        $scope.rotationDegrees = getDegrees(progressInfo.progress);
                        $scope.isNotDownloading = false;
                        if($scope.updateRotation && (prevDegrees !== $scope.rotationDegrees)) {
                            $scope.updateRotation();
                        }
                    } else {
                        $scope.isNotDownloading = true;
                    }
                } else {
                    $scope.isNotDownloading = true;
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
             * clean up
             * @return {void}
             * @method onDestroy
             */
            function onDestroy() {
                GamesDataFactory.events.off('games:update:' + $scope.offerId, updateDownloadInfo);
                GamesDataFactory.events.off('games:progressupdate:' + $scope.offerId, updateDownloadInfo);
            }

            // listen to any updates to the data
            GamesDataFactory.events.on('games:update:' + $scope.offerId, updateDownloadInfo);
            GamesDataFactory.events.on('games:progressupdate:' + $scope.offerId, updateDownloadInfo);
            $scope.$on('$destroy', onDestroy);

            storeDownloadInfo();

        }

        function originGameProgress(ComponentsConfigFactory) {

            function originGameProgressLink(scope, element) {

                var progressFill = element.find('.otkprogress-radial-progressfill');

                /**
                * Update the rotation value of the progress bar
                * @method updateRotation
                */
                function updateRotation() {
                    progressFill.css({
                        'transform': 'rotate(' + scope.rotationDegrees + ')',
                        '-webkit-transform': 'rotate(' + scope.rotationDegrees + ')'
                    });
                }

                scope.updateRotation = updateRotation;
            }

            return {
                restrict: 'E',
                scope: {
                    offerId: '@offerid',
                    percent: '@',
                    state: '@'
                },
                controller: 'OriginGameProgressCtrl',
                templateUrl: ComponentsConfigFactory.getTemplatePath('game/views/progress.html'),
                link: originGameProgressLink
            };
        }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameProgress
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offerid of the game you want to interact with
     * @param {string} percent the percentage
     * @param {string} state the state
     *
     * @description
     *
     * game progress for  game tile
     *
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-progress offerid="OFB-EAST:109549060" percent=50 state="DOWNLOADING"></origin-game-progress>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameProgressCtrl', OriginGameProgressCtrl)
        .directive('originGameProgress', originGameProgress);
}());
