/**
 * @file game/library/progress.js
 */
(function() {
    'use strict';
    
    var CONTEXT_NAMESPACE = 'origin-game-progress';

    function OriginGameProgressCtrl($scope, GamesDataFactory, UtilFactory) {
            //constants
            var MAX_PERCENT = 100,
                MAX_DEGREES = 360,
                COMPLETE_STR = UtilFactory.getLocalizedStr($scope.completestr, CONTEXT_NAMESPACE, 'completestr'),
                PLAYABLE_NOW_STR = UtilFactory.getLocalizedStr($scope.playablenowstr, CONTEXT_NAMESPACE, 'playablenowstr'),
                PLAYABLE_AT_STR = UtilFactory.getLocalizedStr($scope.playableatstr, CONTEXT_NAMESPACE, 'playableatstr'),
                ERROR_RETRY_STR = UtilFactory.getLocalizedStr($scope.errorretrystr, CONTEXT_NAMESPACE, 'errorretrystr');

            //do not display unless the user is downloading or isComplete. isComplete is there because there is a weird in between state.
            $scope.isDownloading = false;
            $scope.isComplete = false;
            $scope.downloadState = '';
            $scope.rotationDegreesCSS = '';
            $scope.playableAtStr = '';
            $scope.errorCountStr = '';
            $scope.shouldLightFlag = false;

            /**
            * Update the rotation value of the progress bar
            * @method updateRotation
            */
            function updateRotation(progress) {
                var prevDegreesCSS, curDegrees, curDegreesCSS;
                
                if($scope.downloadState === 'active' || $scope.downloadState === 'paused') {
                    prevDegreesCSS = $scope.rotationDegreesCSS;
                    curDegrees = (Math.round(progress * MAX_DEGREES)) + 'deg';
                    curDegreesCSS = {
                        'transform': 'rotate(' + curDegrees + ')',
                        '-webkit-transform': 'rotate(' + curDegrees + ')'
                    };
                    if(curDegreesCSS !== prevDegreesCSS) {
                        $scope.rotationDegreesCSS = curDegreesCSS;
                    }
                } else {
                    $scope.rotationDegreesCSS = '';
                }
            }

            /**
             * Stores the information needed to reflect the download status
             * @return {void}
             * @method storeProductInfo
             */
            function storeDownloadInfo() {
                var game = GamesDataFactory.getClientGame($scope.offerId),
                    progressInfo, playableAt;

                if (game && game.progressInfo && game.progressInfo.active) {
                    progressInfo = game.progressInfo;
                    playableAt = progressInfo.playableAt;
                    $scope.stateStr = progressInfo.phaseDisplay;
                    $scope.downloadState = UtilFactory.getDownloadState(progressInfo);
                    $scope.percent = Math.floor(progressInfo.progress * MAX_PERCENT);
                    $scope.shouldLightFlag = progressInfo.shouldLightFlag;
                    updateRotation(progressInfo.progress);
                    
                    if(playableAt) {
                        if($scope.shouldLightFlag) {
                            $scope.playableAtStr = PLAYABLE_NOW_STR;
                        } else {
                            $scope.playableAtStr = PLAYABLE_AT_STR.replace('%1', (playableAt * 100).toFixed(0) + '%');
                        } 
                    }
                    
                    switch(game.progressInfo.phase) {
                        case 'ENQUEUED': {
                            $scope.isDownloading = progressInfo.progress > 0;
                            $scope.isComplete = false;
                            break;
                        }
                        case 'READYTOSTART': {
                            $scope.isDownloading = false;
                            $scope.errorCountStr = '';
                            break;
                        }
                        default: {
                            $scope.isComplete = false;
                            $scope.isDownloading = true;
                            $scope.errorCountStr = '';
                            break;
                        }
                    }
                } else {
                    $scope.isDownloading = false;
                }
            }
            
            /**
             * Stores the information needed to reflect the complete status
             * @return {void}
             * @method storeCompleteInfo
             */
            function storeCompleteInfo() {
                var game = GamesDataFactory.getClientGame($scope.offerId);
                if(!game || !game.progressInfo) {
                    return;
                }

                // There is a bit of a timing issue here...
                if(game.completedIndex > -1 || game.progressInfo.phase === 'COMPLETED' || (game.queueIndex === 0 && game.progressInfo.phase === 'READYTOSTART')) {
                    $scope.downloadState = 'complete';
                    $scope.isComplete = true;
                    $scope.stateStr = COMPLETE_STR;
                }
            }

            /**
             * Store the updated download information
             * @return {void}
             * @method updateDownloadInfo
             */
            function updateDownloadInfo() {
                storeDownloadInfo();
                if(!$scope.$$phase) {
                    $scope.$digest();
                }
            }
            
            /**
             * Store the updated possible complete information
             * @return {void}
             * @method updateDownloadInfo
             */
            function updateCompleteInfo(offerId) {
                if(offerId === $scope.offerId) {
                    storeCompleteInfo();
                }
            }
            
            /**
             * Stores the operation error info
             * @return {void}
             * @method operationErrorInfo
             */
            function operationErrorInfo() {
                var game = GamesDataFactory.getClientGame($scope.offerId);
                var errorCount = (game && game.progressInfo) ? game.progressInfo.errorRetryCount : 0;
                if(errorCount) {
                    $scope.errorCountStr = ERROR_RETRY_STR;
                    if(!$scope.$$phase) {
                        $scope.$digest();
                    }
                } else {
                    $scope.errorCountStr = '';
                }
            }

            /**
             * Clears download info
             * @return {void}
             * @method resetDownloadInfo
             */
            function resetDownloadInfo() {
                $scope.isDownloading = false;
                $scope.isComplete = false;
                $scope.downloadState = '';
                $scope.rotationDegreesCSS = '';
                $scope.playableAtStr = '';
                $scope.shouldLightFlag = false;
                $scope.errorCountStr = '';
            }

            /**
             * Clears complete info
             * @return {void}
             * @method clearCompleteInfo
             */
            function clearCompleteInfo() {
                if($scope.downloadState === 'complete') {
                    resetDownloadInfo();
                }
            }

            /**
             * clean up
             * @return {void}
             * @method onDestroy
             */
            function onDestroy() {
                resetDownloadInfo();
                GamesDataFactory.events.off('games:update:' + $scope.offerId, updateDownloadInfo);
                GamesDataFactory.events.off('games:progressupdate:' + $scope.offerId, updateDownloadInfo);
                GamesDataFactory.events.off('games:operationfailedupdate:' + $scope.offerId, operationErrorInfo);
                Origin.events.off(Origin.events.CLIENT_OPERATIONQUEUE_ADDEDTOCOMPLETE, updateCompleteInfo);
                Origin.events.off(Origin.events.CLIENT_OPERATIONQUEUE_REMOVED, clearCompleteInfo);
                Origin.events.off(Origin.events.CLIENT_OPERATIONQUEUE_COMPLETELISTCLEARED, clearCompleteInfo);
            }

            // listen to any updates to the data
            GamesDataFactory.events.on('games:update:' + $scope.offerId, updateDownloadInfo);
            GamesDataFactory.events.on('games:progressupdate:' + $scope.offerId, updateDownloadInfo);
            GamesDataFactory.events.on('games:operationfailedupdate:' + $scope.offerId, operationErrorInfo);
            Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_ADDEDTOCOMPLETE, updateCompleteInfo);
            Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_REMOVED, clearCompleteInfo);
            Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_COMPLETELISTCLEARED, clearCompleteInfo);
            $scope.$on('$destroy', onDestroy);

            storeCompleteInfo();
            storeDownloadInfo();
            operationErrorInfo();
        }

        function originGameProgress(ComponentsConfigFactory) {
            return {
                restrict: 'E',
                scope: {
                    offerId: '@offerid',
                    percent: '@',
                    state: '@',
                    completestr: '@',
                    errorretrystr: '@',
                    playableatstr: '@',
                    playablenowstr: '@'
                },
                controller: 'OriginGameProgressCtrl',
                templateUrl: ComponentsConfigFactory.getTemplatePath('game/views/progress.html')
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
     * @param {LocalizedString} completestr Complete
     * @param {LocalizedString} errorretrystr Trying again
     * @param {LocalizedString} playableatstr PLAYABLE AT %1
     * @param {LocalizedString} playablenowstr NOW PLAYABLE
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
