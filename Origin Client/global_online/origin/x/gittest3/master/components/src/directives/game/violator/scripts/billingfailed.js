/**
 * @file game/violator/billingfailed.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-billingfailed';

    function OriginGameViolatorBillingfailedCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorBillingfailed(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorBillingfailedCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/billingfailed.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorBillingfailed
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * Billing failed violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-billingfailed></origin-game-violator-billingfailed>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorBillingfailedCtrl', OriginGameViolatorBillingfailedCtrl)
        .directive('originGameViolatorBillingfailed', originGameViolatorBillingfailed);
}());