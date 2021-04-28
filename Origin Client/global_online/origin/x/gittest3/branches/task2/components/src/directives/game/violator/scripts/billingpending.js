/**
 * @file game/violator/billingpending.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-billingpending';

    function OriginGameViolatorBillingpendingCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorBillingpending(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorBillingpendingCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/billingpending.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorBillingpending
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * Billing pending violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-billingpending></origin-game-violator-billingpending>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorBillingpendingCtrl', OriginGameViolatorBillingpendingCtrl)
        .directive('originGameViolatorBillingpending', originGameViolatorBillingpending);
}());