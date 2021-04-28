
/**
 * @file store/pdp/access/scripts/nonvaultupsell.js
 */
 /* global moment */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-hero';

    function OriginStorePdpAccessNonvaultupsellCtrl($scope, UtilFactory, AppCommFactory, PriceInsertionFactory, NavigationFactory, DateHelperFactory) {
        function isReleased(releaseDate) {
            return DateHelperFactory.isInThePast(releaseDate);
        }

        function init() {
            var basegameDate = moment($scope.model.releaseDate).format('LL') || null;

            $scope.isTrialStarted = function() {
                return $scope.earlyAccessTrialModel && 
                        $scope.earlyAccessTrialModel.isOwned && 
                        !$scope.isTrialExpired() ? true : false;
            };

            $scope.isTrialExpired = function() {
                return $scope.earlyAccessTrialModel &&
                $scope.earlyAccessTrialModel.trialTimeRemaining &&
                $scope.earlyAccessTrialModel.trialTimeRemaining.hasTimeLeft === false ? true : false;
            };

            $scope.notExpiredEarlyAccessTrialMessage = function() {
                var trialHoursRemaining = Math.ceil(($scope.earlyAccessTrialModel.trialTimeRemaining.leftTrialSec/60)/60),
                    gameName = _.get($scope, ['earlyAccessTrialModel', 'displayName']);

                return UtilFactory.getLocalizedStr($scope.notExpiredEarlyAccessTrial, CONTEXT_NAMESPACE, 'not-expired-early-access-trial', {'%hours%': trialHoursRemaining, '%game%': gameName});
            };

            $scope.nonVaultUpsellStrings = {};

            PriceInsertionFactory
                .insertPriceIntoLocalizedStr($scope, $scope.nonVaultUpsellStrings, $scope.nonvaultUpsellHeader, CONTEXT_NAMESPACE, 'nonvault-upsell-header');
            PriceInsertionFactory
                .insertPriceIntoLocalizedStr($scope, $scope.nonVaultUpsellStrings, $scope.nonsubscriberAlternateEditionInVaultText, CONTEXT_NAMESPACE, 'nonsubscriber-alternate-edition-in-vault-text');
            PriceInsertionFactory
                .insertPriceIntoLocalizedStr($scope, $scope.nonVaultUpsellStrings, $scope.vaultGameOthText, CONTEXT_NAMESPACE, 'vault-game-oth-text');

            $scope.nonVaultUpsellStrings.nonvaultTrialUpsellMiddle = UtilFactory.getLocalizedStr($scope.nonvaultTrialUpsellMiddle, CONTEXT_NAMESPACE, 'nonvault-trial-upsell-middle', {'%date%': basegameDate});
            $scope.nonVaultUpsellStrings.nonvaultTrialUpsellAfter = UtilFactory.getLocalizedStr($scope.nonvaultTrialUpsellAfter, CONTEXT_NAMESPACE, 'nonvault-trial-upsell-after');
            $scope.nonVaultUpsellStrings.nonvaultDiscountUpsell = UtilFactory.getLocalizedStr($scope.nonvaultDiscountUpsell, CONTEXT_NAMESPACE, 'nonvault-discount-upsell');
            $scope.nonVaultUpsellStrings.nonvaultupsellLearnMore = UtilFactory.getLocalizedStr($scope.nonvaultupsellLearnMore, CONTEXT_NAMESPACE, 'nonvault-upsell-learn-more');
            $scope.nonVaultUpsellStrings.vaultGameOthHeader = UtilFactory.getLocalizedStr($scope.vaultGameOthHeader, CONTEXT_NAMESPACE, 'vault-game-oth-header');
            $scope.nonVaultUpsellStrings.nonVaultGameEarlyTrialAndGameReleased = UtilFactory.getLocalizedStr($scope.nonVaultGameEarlyTrialAndGameReleased, CONTEXT_NAMESPACE, 'nonvault-game-early-trial-and-game-released');
            $scope.nonVaultUpsellStrings.expiredEarlyAccessTrialGamePreOrdered = UtilFactory.getLocalizedStr($scope.expiredEarlyAccessTrialGamePreOrdered, CONTEXT_NAMESPACE, 'expired-early-access-trial-game-pre-ordered');
            $scope.nonVaultUpsellStrings.expiredEarlyAccessTrialGameNotPreOrdered = UtilFactory.getLocalizedStr($scope.expiredEarlyAccessTrialGameNotPreOrdered, CONTEXT_NAMESPACE, 'expired-early-access-trial-game-not-pre-ordered');
            $scope.nonVaultUpsellStrings.seeDetails = UtilFactory.getLocalizedStr($scope.seeDetails, CONTEXT_NAMESPACE, 'see-details');
            $scope.nonVaultUpsellStrings.download = UtilFactory.getLocalizedStr($scope.download, CONTEXT_NAMESPACE, 'download');
            $scope.nonVaultUpsellStrings.playNow = UtilFactory.getLocalizedStr($scope.playNow, CONTEXT_NAMESPACE, 'play-now');
            $scope.nonVaultUpsellStrings.getItNow = UtilFactory.getLocalizedStr($scope.getItNow, CONTEXT_NAMESPACE, 'get-it-now');
            $scope.nonVaultUpsellStrings.subscriberVaultUpgradeGameHeader = UtilFactory.getLocalizedStr($scope.subscriberVaultUpgradeGameHeader, CONTEXT_NAMESPACE, 'subscriber-vault-upgrade-game-header');
            $scope.nonVaultUpsellStrings.upgradeNow = UtilFactory.getLocalizedStr($scope.upgradeNow, CONTEXT_NAMESPACE, 'upgrade-now');

            function getTrialReleaseDate() {
                var trialDate = $scope.earlyAccessTrialModel ? moment($scope.earlyAccessTrialModel.releaseDate).format('LL') : '';

                $scope.nonVaultUpsellStrings.nonvaultTrialUpsellBefore = UtilFactory.getLocalizedStr($scope.nonvaultTrialUpsellBefore, CONTEXT_NAMESPACE, 'nonvault-trial-upsell-before', {'%date%': trialDate});
                $scope.nonVaultUpsellStrings.nonVaultGameEarlyTrialNotReleased = UtilFactory.getLocalizedStr($scope.nonVaultGameEarlyTrialNotReleased, CONTEXT_NAMESPACE, 'nonvault-game-early-trial-not-released', {'%date%': trialDate});
                $scope.nonVaultUpsellStrings.nonVaultGameEarlyTrialReleased = UtilFactory.getLocalizedStr($scope.nonVaultGameEarlyTrialReleased, CONTEXT_NAMESPACE, 'nonvault-game-early-trial-released', {'%date%': trialDate});
            }

            $scope.$watch('earlyAccessTrialModel.releaseDate', function(newValue, oldValue){
                if(newValue !== oldValue) {
                    getTrialReleaseDate();
                }
            });

            if($scope.earlyAccessTrialModel && $scope.earlyAccessTrialModel.releaseDate) {
                getTrialReleaseDate();
            }

            $scope.$watch('vaultEdition.displayName', function(newValue){
                if(typeof newValue !== 'undefined') {
                    $scope.nonVaultUpsellStrings.nonsubscriberAlternateEditionInVaultHeader = UtilFactory.getLocalizedStr($scope.nonsubscriberAlternateEditionInVaultHeader, CONTEXT_NAMESPACE, 'nonsubscriber-alternate-edition-in-vault-header', {'%game%': $scope.vaultEdition.displayName});
                }
            });

            $scope.goToOriginAccessTrialsPage = function ($event) {
                $event.preventDefault();
                AppCommFactory.events.fire('uiRouter:go', 'app.store.wrapper.origin-access-trials');
            };
            $scope.accessTrialUrl = NavigationFactory.getAbsoluteUrl('app.store.wrapper.origin-access-trials');

            $scope.showNonSubscriberUpsell = function() {
                return !$scope.user.isSubscriber &&
                    !$scope.lesserEditionInVault() &&
                    !$scope.greaterEditionInVault() &&
                    !$scope.model.oth &&
                    ($scope.earlyAccessTrialModel || $scope.model.hasSubsDiscount || $scope.hasManualOverrides());
            };

            $scope.showAlternateVaultEditionUpsell = function() {
                return !$scope.user.isSubscriber &&
                    !$scope.model.oth &&
                    !$scope.model.subscriptionAvailable &&
                    ($scope.lesserEditionInVault() || $scope.greaterEditionInVault());
            };

            $scope.showOthSubscriptionsUpsell = function() {
                return $scope.model.oth &&
                ($scope.model.subscriptionAvailable || $scope.model.isUpgradeable) &&
                !$scope.model.vaultEntitlement &&
                !$scope.user.isSubscriber ? true : false;
            };

            $scope.showSubscriberUpsell = function() {
                return $scope.user.isSubscriber &&
                (!$scope.model.isOwned || !isReleased(_.get($scope, ['model', 'releaseDate']))) &&
                ($scope.earlyAccessTrialModel || $scope.manualSubscriberTrialOverride) &&
                (!$scope.ownsLesserEdition() && !$scope.ownsGreaterEdition() || !isReleased(_.get($scope, ['model', 'releaseDate']))) ? true : false;
            };

            $scope.showSubscriberUpgradeArea = function() {
                return $scope.user.isSubscriber &&
                $scope.vaultEdition &&
                !$scope.model.oth &&
                ($scope.ownsLesserEdition() || $scope.model.isOwned) &&
                !$scope.vaultEdition.isOwned &&
                $scope.model.isUpgradeable ? true : false;
            };
        }

        var stopWatchingOcdDataReady = $scope.$watch('ocdDataReady', function (isOcdDataReady) {
            if (isOcdDataReady) {
                stopWatchingOcdDataReady();
                init();
            }
        });


    }

    function originStorePdpAccessNonvaultupsell(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginStorePdpAccessNonvaultupsellCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/access/views/nonvaultupsell.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpAccessNonvaultupsell
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedTemplateString} nonvault-upsell-header * merchandized in defaults
     * @param {LocalizedString} nonvault-trial-upsell-before * merchandized in defaults
     * @param {LocalizedString} nonvault-trial-upsell-middle * merchandized in defaults
     * @param {LocalizedString} nonvault-trial-upsell-after * merchandized in defaults
     * @param {LocalizedString} nonvault-discount-upsell * merchandized in defaults
     * @param {LocalizedString} nonvault-upsell-learn-more * merchandized in defaults
     * @param {LocalizedString} nonvault-game-early-trial-not-released * merchandized in defaults
     * @param {LocalizedString} nonvault-game-early-trial-released * merchandized in defaults
     * @param {LocalizedString} nonsubscriber-alternate-edition-in-vault-header * merchandized in defaults
     * @param {LocalizedTemplateString} nonsubscriber-alternate-edition-in-vault-text * merchandized in defaults
     * @param {LocalizedString} vault-game-oth-header * merchandized in defaults
     * @param {LocalizedTemplateString} vault-game-oth-text * merchandized in defaults
     * @param {LocalizedString} nonvault-game-early-trial-and-game-released * merchandized in defaults
     * @param {LocalizedString} not-expired-early-access-trial * merchandized in defaults
     * @param {LocalizedString} expired-early-access-trial-game-pre-ordered * merchandized in defaults
     * @param {LocalizedString} expired-early-access-trial-game-not-pre-ordered * merchandized in defaults
     * @param {LocalizedString} see-details * merchandized in defaults
     * @param {LocalizedString} download * merchandized in defaults
     * @param {LocalizedString} play-now * merchandized in defaults
     * @param {LocalizedString} get-it-now * merchandized in defaults
     * @param {LocalizedString} subscriber-vault-upgrade-game-header * merchandized in defaults
     * @param {LocalizedString} upgrade-now * merchandized in defaults
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-access-nonvaultupsell />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpAccessNonvaultupsellCtrl', OriginStorePdpAccessNonvaultupsellCtrl)
        .directive('originStorePdpAccessNonvaultupsell', originStorePdpAccessNonvaultupsell);
}());