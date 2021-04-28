
/**
 * @file store/pdp/scripts/entitlementarea.js
 */
(function(){
    'use strict';

     var CONTEXT_NAMESPACE = 'origin-store-pdp-hero';

    function OriginStorePdpEntitlementareaCtrl($scope, UtilFactory, moment) {

        function init() {
            $scope.entString = {
                startNewTrial: UtilFactory.getLocalizedStr($scope.startNewTrial, CONTEXT_NAMESPACE, 'start-new-trial'),
                viewInLibrary: UtilFactory.getLocalizedStr($scope.viewInLibrary, CONTEXT_NAMESPACE, 'view-in-library'),
                getTrial: UtilFactory.getLocalizedStr($scope.getTrial, CONTEXT_NAMESPACE, 'get-trial'),
                getBeta: UtilFactory.getLocalizedStr($scope.getBeta, CONTEXT_NAMESPACE, 'get-beta'),
                tryDemo: UtilFactory.getLocalizedStr($scope.tryDemo, CONTEXT_NAMESPACE, 'try-demo')
            };
        }

        function hideEntitlementArea() {
            return !$scope.ocdDataReady || (($scope.user.isSubscriber && ($scope.model.subscriptionAvailable || $scope.model.isUpgradeable)) ||
                    $scope.model.oth ||
                    $scope.model.isOwned ||
                    $scope.ownsLesserEdition() ||
                    $scope.ownsGreaterEdition());
        }

        function isProductReleased(){
            var releaseDate = moment($scope.model.releaseDate);
            return releaseDate.isBefore(moment());
        }

        $scope.$watch('editions', function(){
            $scope.hideEntitlementOptions = hideEntitlementArea();
        });

        $scope.$watchOnce('model', function(){
            $scope.isProductReleased = isProductReleased();
        }, false, function(model){
            return model.releaseDate;
        });

        $scope.isInactiveTrial = function(trial) {
            return !trial.isOwned;
        };

        $scope.inactiveTrialMessage = function(trialModel) {
            return UtilFactory.getLocalizedStr($scope.startNewTrial, CONTEXT_NAMESPACE, 'start-new-trial', {'%duration%': trialModel.trialLaunchDuration});
        };

        $scope.isActiveTrial = function(trial) {
            return trial.isOwned && trial.trialTimeRemaining && trial.trialTimeRemaining.hasTimeLeft;
        };

        $scope.activeTrialMessage = function(trialModel) {
            var gameName = trialModel.displayName,
            hours = trialModel.trialTimeRemaining ? Math.ceil((trialModel.trialTimeRemaining.leftTrialSec/60)/60) : 0;

            return UtilFactory.getLocalizedStr($scope.activeTrial, CONTEXT_NAMESPACE, 'active-trial', {'%game%': gameName, '%hours%': hours});
        };

        $scope.isExpiredTrial = function(trial) {
            return trial.isOwned && trial.trialTimeRemaining && trial.trialTimeRemaining.hasTimeLeft === false;
        };

        $scope.expiredTrialMessage = function(trialModel) {
            var gameName = trialModel.displayName;

            return UtilFactory.getLocalizedStr($scope.expiredTrial, CONTEXT_NAMESPACE, 'expired-trial', {'%game%': gameName});
        };

        $scope.isUnknownTrial = function(trial) {
            return trial.isOwned &&  trial.trialTimeRemaining && (trial.trialTimeRemaining.success === false);
        };

        $scope.unknownTrialMessage = function(trialModel) {
            var gameName = trialModel.displayName;

            return UtilFactory.getLocalizedStr($scope.unknownTrialTime, CONTEXT_NAMESPACE, 'unknown-trial-time', {'%game%': gameName});
        };

        var stopWatchingOcdDataReady = $scope.$watch('ocdDataReady', function (isOcdDataReady) {
            if (isOcdDataReady) {
                stopWatchingOcdDataReady();
                init();
            }
        });
    }
    function originStorePdpEntitlementarea(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginStorePdpEntitlementareaCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/entitlementarea.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpEntitlementarea
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-entitlementarea />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpEntitlementareaCtrl', OriginStorePdpEntitlementareaCtrl)
        .directive('originStorePdpEntitlementarea', originStorePdpEntitlementarea);
}());