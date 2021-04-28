/**
 * @file dialog/content/checkoutconfirmation.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-checkoutconfirmation';

    function originDialogContentCheckoutconfirmationCtrl($scope, $attrs, DialogFactory, ObjectHelperFactory, PdpUrlFactory, ProductFactory, UtilFactory) {
        $scope.strings = {
            title: UtilFactory.getLocalizedStr($attrs.titleStr, CONTEXT_NAMESPACE, 'title')
        };
    }

    function originDialogContentCheckoutconfirmation(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offers: '='
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
     * @param {array} offers Array of offerIds used to populate list of checkout items
     * @param {LocalizedString|OCD} titleStr The "You're Awesome!" title text
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