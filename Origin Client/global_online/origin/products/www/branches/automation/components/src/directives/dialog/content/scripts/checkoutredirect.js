/**
 * @file dialog/content/checkoutredirect.js
 */
(function() {
    'use strict';

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var CONTEXT_NAMESPACE = 'origin-dialog-content-checkoutredirect';


    function originDialogContentCheckoutredirectCtrl($scope, UtilFactory, DialogFactory, CheckoutFactory, ComponentsConfigHelper) {
        function acknowledgeWarning(popupWindow) {
            return function () {
                DialogFactory.close(CONTEXT_NAMESPACE);
                if (popupWindow) {
                    CheckoutFactory.paymentProviderRedirectPrepare();
                    CheckoutFactory.paymentProviderRedirectReady($scope.type, $scope.redirectUri);
                }
            };
        }

        $scope.strings = {
            titleStr: UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'title-str'),
            description: UtilFactory.getLocalizedStr($scope.description, CONTEXT_NAMESPACE, 'description'),
            dismiss: UtilFactory.getLocalizedStr($scope.dismissText, CONTEXT_NAMESPACE, 'dismiss-text'),
            continue: UtilFactory.getLocalizedStr($scope.continueText, CONTEXT_NAMESPACE, 'continue-text')
        };

        $scope.btnClose = $scope.continueToPaymentProvider === BooleanEnumeration.true ? $scope.strings.continue : $scope.strings.dismiss;
        $scope.acknowledgeWarning = acknowledgeWarning($scope.continueToPaymentProvider);
        $scope.isOigContext = angular.isDefined(ComponentsConfigHelper.getOIGContextConfig());
    }

    function originDialogContentCheckoutredirect(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                description: '@',
                dismissText: '@',
                continueText: '@',
                continueToPaymentProvider: '@',
                type: '@',
                redirectUri: '@'
            },
            controller: originDialogContentCheckoutredirectCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/checkoutredirect.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentCheckoutredirect
     * @restrict E
     * @element ANY
     * @scope
     *
     * @param {LocalizedString} title-str The title text of the dialog that tells the user to complete thier purchase in the new tab
     * @param {LocalizedString} description The body text of the dialog that tells the user to complete thier purchase in the new tab
     * @param {LocalizedString} dismiss-text The button text to dismiss the dialog
     * @param {LocalizedString} continue-text The button text to continue to the third party payment provider ex. "Continue"
     * @param {BooleanEnumeration} continue-to-payment-provider * (Not to be merchandised) Whether this dialog should open up the payment provider in a new tab or just close itself upon button click.
     * @param {string} type * (Not to be merchandised) The checkout type that this flow will redirect to if 'continueToPaymentProvider' is true
     * @param {string} redirect-uri * (Not to be merchandised) The uri of the payment provider that will be opened if 'continueToPaymentProvider' is true
     *
     * @description Messaging to let user know they should continue adding their payment provider in the newly opened tab
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-checkout-confirmation>
     *         </origin-dialog-content-checkout-confirmation>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originDialogContentCheckoutredirect', originDialogContentCheckoutredirect);

}());