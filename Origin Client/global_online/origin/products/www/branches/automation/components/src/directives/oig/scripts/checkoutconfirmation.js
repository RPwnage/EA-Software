/**
 * @file oig/scripts/checkoutconfirmation.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-oig-checkoutconfirmation';

    function originOigCheckoutconfirmationCtrl($scope, $stateParams, UtilFactory) {
        $scope.offers = $stateParams.offerIds.split(',');
        $scope.strings = {
            title: UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'title')
        };
    }

    function originOigCheckoutconfirmation(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offers: '=',
                modalId:'@id',
                titleStr: '@'
            },
            controller: 'originOigCheckoutconfirmationCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('oig/views/checkoutconfirmation.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originOigCheckoutconfirmation
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offers The JSON list of offer IDs that have been purchased.
     * @param {string} id The ID of the modal to close when the user clicks "Download"
     * @param {LocalizedString|OCD} title-str The title text of the dialog that confirms checkout upon direct entitlement ex. "You're Awesome!"
     * @description
     *
     * "Get it Now" direct entitlement dialog (for OTH titles)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-oig-checkoutconfirmation
     *              offers="["Origin.OFR.50.0000100"]"
     *              id="checkout-confirmation"
     *              title-str="You're Awesome!">
     *         </origin-oig-checkoutconfirmation>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('originOigCheckoutconfirmationCtrl', originOigCheckoutconfirmationCtrl)
        .directive('originOigCheckoutconfirmation', originOigCheckoutconfirmation);

}());