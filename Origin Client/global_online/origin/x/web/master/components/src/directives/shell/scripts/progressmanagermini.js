/**
 * @file shell/scripts/progressmanagermini.js
 */
(function() {
    'use strict';
    
    var CONTEXT_NAMESPACE = 'download-preview';

    function progressManagerMiniCtrl($scope, UtilFactory, GamesDataFactory, ComponentsLogFactory) {
        var MAX_PERCENT = 100,
            FLAG_DEFAULT_OFFSET = 15,
            MIN_FLAG_LEFT = 3,
            MAX_FLAG_LEFT = 72,
            REMAINING_STR = UtilFactory.getLocalizedStr(REMAINING_STR, CONTEXT_NAMESPACE, 'remainingStr'),
            CALCULATING_STR = UtilFactory.getLocalizedStr(CALCULATING_STR, CONTEXT_NAMESPACE, 'calculatingStr');
            
        var headOfferId = '';
        
        $scope.playableStr = UtilFactory.getLocalizedStr($scope.playableStr, CONTEXT_NAMESPACE, 'playableStr');
        $scope.downloadManagerStr = UtilFactory.getLocalizedStr($scope.downloadManagerStr, CONTEXT_NAMESPACE, 'downloadManagerStr');
        $scope.flagHalfFlagPercent = 0;
        $scope.title = '';
        $scope.percent = 0;
        $scope.progressState = '';
        $scope.phaseDisplay = '';
        $scope.numNotification = 0;
        $scope.packArt = null;
        $scope.flagPosition = 0;
        $scope.arrowPosition = 0;
        $scope.shouldLightFlag = false;

        /**
         * Stores the information needed to reflect the download status
         * @return {void}
         * @method updateDownloadInfo
         */
        function storeDownloadInfo() {
            var game = GamesDataFactory.getClientGame(headOfferId),
                progressInfo;

            if (game && game.progressInfo && game.progressInfo.active) {
                progressInfo = game.progressInfo;
                $scope.progressState = UtilFactory.getDownloadState(progressInfo);
                if(progressInfo.phase === "RUNNING") {
                    if(progressInfo.secondsRemaining > 0) {
                        $scope.phaseDisplay = REMAINING_STR.replace('%1', progressInfo.timeLeft);
                    } else {
                        $scope.phaseDisplay = CALCULATING_STR;
                    }
                } else {
                    $scope.phaseDisplay = progressInfo.phaseDisplay;
                }
                $scope.percent = Math.floor(progressInfo.progress * MAX_PERCENT);
                $scope.shouldLightFlag = progressInfo.shouldLightFlag;
                $scope.arrowPosition = progressInfo.playableAt * 100;
                $scope.flagPosition = ($scope.arrowPosition - FLAG_DEFAULT_OFFSET);
                if($scope.flagPosition < MIN_FLAG_LEFT) {
                    $scope.flagPosition = MIN_FLAG_LEFT;
                } else if($scope.flagPosition > MAX_FLAG_LEFT) {
                    $scope.flagPosition = MAX_FLAG_LEFT;
                }
            } else {
                resetProperties();
            }
            if(!$scope.$$phase) {
                $scope.$digest();
            }
        }
        
        
        /**
         * Stores the product information to the scope after
         * doing some transforms to the clientActions
         * @return {void}
         * @method storeProductInfo
         */
        function storeProductInfo(data) {
            // Todo, need the keyart
            //$scope.packArt = data.i18n.packArtLarge;
            $scope.title = data.i18n.displayName;
            $scope.$digest();
        }
        
        
        /**
         * Store the game information
         * @return {void}
         * @method handleGetMyGame
         */
        function handleGetMyGame(response) {
            storeProductInfo(response[headOfferId]);
        }


        /**
         * Changes the entitlement we are pointing to. Disconnects prior entitlement
         * @return {void}
         * @method updateHead
         */
        function updateHead(newHeadOfferId) {
            if (Origin.client.isEmbeddedBrowser()) {
                // If the head is not valid: this might be the inital call, or the head might be empty.
                // Let's double check with the C++.
                if(!newHeadOfferId) {
                    newHeadOfferId = Origin.client.contentOperationQueue.headOfferId();
                }
                if(newHeadOfferId === headOfferId) {
                    return;
                }
                resetProperties();
                if(newHeadOfferId) {
                    headOfferId = newHeadOfferId;
                    GamesDataFactory.events.on('games:progressupdate:' + headOfferId, storeDownloadInfo);
                    GamesDataFactory.events.on('games:update:' + headOfferId, storeDownloadInfo);
                    GamesDataFactory.getCatalogInfo([headOfferId])
                    .then(handleGetMyGame)
                    .catch(function(error) {
                        ComponentsLogFactory.error('[download-preview GamesDataFactory.getCatalogInfo] error', error.stack);
                    });
                }
                storeDownloadInfo();
            }
        }


        function updateNotificationCount() {
            $scope.numNotification = Origin.client.contentOperationQueue.entitlementsCompletedOfferIdList().length;
            if(!headOfferId) {
                resetProperties();
            }
            $scope.$digest();
        }
        

        /**
         * disconnect signals and reset properties
         * @return {void}
         * @method resetProperties
         */
        function resetProperties() {
            GamesDataFactory.events.off('games:progressupdate:' + headOfferId, storeDownloadInfo);
            GamesDataFactory.events.off('games:update:' + headOfferId, storeDownloadInfo);
            headOfferId = '';
            $scope.title = '';
            $scope.percent = 0;
            $scope.progressState = '';
            $scope.phaseDisplay = '';
            $scope.packArt = null;
            $scope.flagPosition = 0;
            $scope.arrowPosition = 0;
            $scope.shouldLightFlag = false;
        }
        
        /**
         * disonnect all signals and reset properties
         * @return {void}
         * @method onDestroy
         */
        function onDestroy() {
            Origin.events.off(Origin.events.CLIENT_OPERATIONQUEUE_HEADCHANGED, updateHead);
            Origin.events.off(Origin.events.CLIENT_OPERATIONQUEUE_ADDEDTOCOMPLETE, updateNotificationCount);
            Origin.events.off(Origin.events.CLIENT_OPERATIONQUEUE_REMOVED, updateNotificationCount);
            Origin.events.off(Origin.events.CLIENT_OPERATIONQUEUE_COMPLETELISTCLEARED, updateNotificationCount);
            resetProperties();
        }
        
        updateHead();
        Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_HEADCHANGED, updateHead);
        Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_ADDEDTOCOMPLETE, updateNotificationCount);
        Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_REMOVED, updateNotificationCount);
        Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_COMPLETELISTCLEARED, updateNotificationCount);
        $scope.$on('$destroy', onDestroy);
    }
    
    function progressManagerMini(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            controller: 'progressManagerMiniCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/progressmanagermini.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:progressManagerMini
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * download preview menu item
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-shell-menu-item>
     *             <progress-manager-mini></progress-manager-mini>
     *         </origin-shell-menu-item>
     *     </file>
     * </example>
     *     
     */
    angular.module('origin-components')
        .controller('progressManagerMiniCtrl', progressManagerMiniCtrl)
        .directive('progressManagerMini', progressManagerMini);
}());