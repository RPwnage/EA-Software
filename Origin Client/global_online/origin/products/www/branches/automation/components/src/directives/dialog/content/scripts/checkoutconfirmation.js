/**
 * @file dialog/content/checkoutconfirmation.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-checkoutconfirmation';

    function originDialogContentCheckoutconfirmationCtrl($scope, UtilFactory, GamesCatalogFactory, GameRefiner) {
        $scope.isVault = false;
        $scope.strings = {
            title: UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'title-str')
        };
        // send the telemetry based on the offer ids
        if (_.isArray($scope.offers)) {
            var oth = false;

            // figure out if the offerIds are 'On the house' or 'Vault' game
            _.forEach($scope.offers, function(offerId) {
                var catalogInfo = GamesCatalogFactory.getExistingCatalogInfo(offerId);
                if (GameRefiner.isOnTheHouse(catalogInfo)) {
                    oth = true;
                }
            });
            var offers = $scope.offers.join(',');
            if (oth) {
                Origin.telemetry.sendMarketingEvent('Checkout', 'OTH', offers);
            } else {
                $scope.isVault = true;
                Origin.telemetry.sendMarketingEvent('Checkout', 'Vault', offers);
            }
        }
    }

    function originDialogContentCheckoutconfirmation(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offers: '=',
                modalId:'@id',
                titleStr: '@',
                isVault: '@'
            },
            controller: 'originDialogContentCheckoutconfirmationCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/checkoutconfirmation.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentCheckoutconfirmation
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString|OCD} title-str The title text of the dialog that confirms checkout upon direct entitlement ex. "You're Awesome!"
     * @description
     *
     * "Get it Now" direct entitlement dialog (for OTH titles)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-checkout-confirmation offers="["Origin.OFR.50.0000500"]">
     *         </origin-dialog-content-checkout-confirmation>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('originDialogContentCheckoutconfirmationCtrl', originDialogContentCheckoutconfirmationCtrl)
        .directive('originDialogContentCheckoutconfirmation', originDialogContentCheckoutconfirmation);

}());