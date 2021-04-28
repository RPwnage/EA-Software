/**
 * @file game/violator/trialnotactivated.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-trialnotactivated';

    function OriginGameViolatorTrialnotactivatedCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorTrialnotactivated(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorTrialnotactivatedCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/trialnotactivated.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorTrialnotactivated
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * Trial Not Activated violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-trialnotactivated></origin-game-violator-trialnotactivated>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorTrialnotactivatedCtrl', OriginGameViolatorTrialnotactivatedCtrl)
        .directive('originGameViolatorTrialnotactivated', originGameViolatorTrialnotactivated);
}());