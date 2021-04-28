/**
 * @file payment.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings-payment';

    function OriginSettingsPaymentCtrl($scope, UtilFactory) {
        $scope.title = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'titlestr');
        $scope.description = UtilFactory.getLocalizedStr($scope.descriptionStr, CONTEXT_NAMESPACE, 'descriptionstr');
    }

    function originSettingsPayment(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                descriptionStr: '@descriptionstr',
                titleStr: '@titlestr'
            },
            controller: 'OriginSettingsPaymentCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/payment.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSettingsPayment
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} titlestr the title string
     * @param {string} descriptionstr the description str
     * @description
     *
     * Settings section 
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-settings-payment titlestr="Looking for more?" descriptionstr="The more you play games and the more you add friends, the better your recommendations will become."></origin-settings-payment>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsPaymentCtrl', OriginSettingsPaymentCtrl)
        .directive('originSettingsPayment', originSettingsPayment);
}());