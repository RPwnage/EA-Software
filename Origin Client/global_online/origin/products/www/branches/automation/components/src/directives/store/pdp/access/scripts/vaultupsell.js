
/**
 * @file store/pdp/access/scripts/vaultupsell.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-hero',
        ACCESS_LANDING_ROUTE = 'app.store.wrapper.origin-access';

    function OriginStorePdpAccessVaultupsellCtrl($scope, CurrencyHelper, UtilFactory, PdpUrlFactory, PriceInsertionFactory, NavigationFactory) {
        var logo = '<span class="origin-store-pdp-cta-icon"><i class="otkicon otkicon-access origin-tealium-otkicon-access"></i></span>';


        function init() {
            $scope.vaultupsellStrings = {
                vaultupsellPointOne: UtilFactory.getLocalizedStr($scope.vaultupsellPointOne, CONTEXT_NAMESPACE, 'vaultupsell-point-one'),
                vaultupsellPointTwo: UtilFactory.getLocalizedStr($scope.vaultupsellPointTwo, CONTEXT_NAMESPACE, 'vaultupsell-point-two'),
                vaultupsellLearnMore: UtilFactory.getLocalizedStr($scope.vaultupsellLearnMore, CONTEXT_NAMESPACE, 'vaultupsell-learn-more'),
                addToMyGames: UtilFactory.getLocalizedStr($scope.addToMyGames, CONTEXT_NAMESPACE, 'add-to-my-games'),
                orText: UtilFactory.getLocalizedStr($scope.orText, CONTEXT_NAMESPACE, 'or-text'),
                getItNow: UtilFactory.getLocalizedStr($scope.getItNow, CONTEXT_NAMESPACE, 'get-it-now')
            };

            PriceInsertionFactory
                .insertPriceIntoLocalizedStr($scope, $scope.vaultupsellStrings, $scope.vaultupsellHeader, CONTEXT_NAMESPACE, 'vaultupsell-header');

            function setAccessCtaText(result) {
                PriceInsertionFactory
                    .insertPriceIntoLocalizedStr($scope, $scope.vaultupsellStrings, $scope.playForZeroWith, CONTEXT_NAMESPACE, 'play-for-zero-with', {'%logo%': logo, '%zero%': result});
            }

            CurrencyHelper
                .formatCurrency(0)
                .then(setAccessCtaText);

            $scope.$watch('vaultEdition.displayName', function (newValue) {
                if (typeof newValue !== 'undefined') {
                    $scope.vaultupsellStrings.upgrade = UtilFactory.getLocalizedStr($scope.subscriberVaultGameUpgradeCta, CONTEXT_NAMESPACE, 'subscriber-vault-game-upgrade-cta', {'%edition%': $scope.vaultEdition.editionName});
                    $scope.vaultupsellStrings.subscriberVaultGameHeader = UtilFactory.getLocalizedStr($scope.subscriberVaultGameHeader, CONTEXT_NAMESPACE, 'subscriber-vault-game-header', {'%game%': $scope.vaultEdition.displayName});
                }
            });

            $scope.accessLandingUrl = NavigationFactory.getAbsoluteUrl(ACCESS_LANDING_ROUTE);
        }

        $scope.goToAccessLandingPage = function($event){
            $event.preventDefault();
            NavigationFactory.openLink(ACCESS_LANDING_ROUTE);
        };

        $scope.showPlayAction = function() {
            return $scope.model.isOwned &&
                     $scope.model.vaultEntitlement &&
                     $scope.user.isSubscriber && 
                     !$scope.model.oth &&
                     ($scope.model.subscriptionAvailable || $scope.model.isUpgradeable) ? true : false;
        };

        $scope.goToVaultPdp = function() {
            if($scope.vaultEdition) {
                return PdpUrlFactory.goToPdp($scope.vaultEdition);
            }
        };

        $scope.$watchOnce('vaultEdition', function(){
           $scope.vaultPdpUrl = PdpUrlFactory.getPdpUrl($scope.vaultEdition, true);
        });

        $scope.showSubscriptionUpsellArea = function() {
            return !$scope.user.isSubscriber &&
                    !$scope.model.oth &&
                    $scope.model.subscriptionAvailable &&
                    !$scope.model.isOwned ? true : false;
        };

        $scope.showSubscriberAquisitionArea = function() {
            return $scope.user.isSubscriber &&
                    !$scope.model.isOwned &&
                    !$scope.model.oth &&
                    !$scope.ownsLesserEdition() &&
                    $scope.vaultEdition && !$scope.vaultEdition.isOwned &&
                    ($scope.lesserEditionInVault() || $scope.greaterEditionInVault() || $scope.model.isUpgradeable);
        };

        $scope.showAddToLibraryCta = function() {
            return $scope.user.isSubscriber &&
                    !$scope.model.oth &&
                    !$scope.model.isOwned &&
                    !$scope.ownsLesserEdition() &&
                    $scope.model.subscriptionAvailable ? true : false;
        };

        $scope.showUpgradeToVaultEdition = function() {
            return $scope.user.isSubscriber &&
                    $scope.model.subscriptionAvailable &&
                    !$scope.model.isOwned &&
                    $scope.ownsLesserEdition() ? true : false;
        };


        var stopWatchingOcdDataReady = $scope.$watch('ocdDataReady', function (isOcdDataReady) {
            if (isOcdDataReady) {
                stopWatchingOcdDataReady();
                init();
            }
        });
    }
    function originStorePdpAccessVaultupsell(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginStorePdpAccessVaultupsellCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/access/views/vaultupsell.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpAccessVaultupsell
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedTemplateString} vaultupsellheader desc
     * @param {LocalizedString} vaultupsell-point-one desc
     * @param {LocalizedString} vaultupsell-point-two desc
     * @param {LocalizedString} vaultupsell-learn-more desc
     * @param {LocalizedString} add-to-my-games desc
     * @param {LocalizedTemplateString} play-for-zero-with desc
     * @param {LocalizedString} or-text desc
     * @param {LocalizedString} subscriber-vault-game-header desc
     * @param {LocalizedString} get-it-now desc
     * @param {LocalizedString} subscriber-vault-game-upgrade-cta desc
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-access-vaultupsell />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpAccessVaultupsellCtrl', OriginStorePdpAccessVaultupsellCtrl)
        .directive('originStorePdpAccessVaultupsell', originStorePdpAccessVaultupsell);
}());
