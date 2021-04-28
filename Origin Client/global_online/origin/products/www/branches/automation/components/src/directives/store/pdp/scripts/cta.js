/**
 * @file store/pdp/cta.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-hero';

    function OriginStorePdpCtaCtrl($scope, UtilFactory, GameRefiner, StoreActionFlowFactory) {

        $scope.installable = false;
        $scope.isBrowserGame = false;
        $scope.isContextMenuVisible = false;
        $scope.isGiftable = false;

        $scope.isPurchaseAction = function() {
            return StoreActionFlowFactory.isPurchaseAction($scope.model);
        };

        $scope.isEntitleAction = function() {
            return StoreActionFlowFactory.isEntitleAction($scope.model);
        };

        $scope.isPlayAction = function() {
            return StoreActionFlowFactory.isPlayAction($scope.model);
        };

        function setButtonType() {
            // defined in vaultupsell.js
            return $scope.showPlayAction() ||
                    $scope.showAddToLibraryCta() ||
                    $scope.showSubscriptionUpsellArea() ||
                    $scope.showUpgradeToVaultEdition() ? 'transparent' : 'primary';
        }

        $scope.$watchOnce('ocdDataReady', function () {
            $scope.isBrowserGame = GameRefiner.isBrowserGame($scope.model);
            $scope.buttonType = setButtonType();
            $scope.pdpCtaStrings = {
                addToMyGames: UtilFactory.getLocalizedStr($scope.addToMyGames, CONTEXT_NAMESPACE, 'add-to-my-games'),
                playNow: UtilFactory.getLocalizedStr($scope.playNow, CONTEXT_NAMESPACE, 'play-now')
            };
        });

    }

    function originStorePdpCta(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/cta.html'),
            controller: 'OriginStorePdpCtaCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpCta
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * PDP CTA area with primary and optional secondary action buttons
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-cta></origin-store-pdp-cta>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpCtaCtrl', OriginStorePdpCtaCtrl)
        .directive('originStorePdpCta', originStorePdpCta);
}());
