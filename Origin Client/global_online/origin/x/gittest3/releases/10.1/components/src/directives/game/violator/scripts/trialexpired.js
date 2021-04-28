/**
 * @file game/violator/trialexpired.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-trialexpired';

    function OriginGameViolatorTrialexpiredCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorTrialexpired(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorTrialexpiredCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/trialexpired.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorTrialexpired
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * Trial Expired violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-trialexpired></origin-game-violator-trialexpired>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorTrialexpiredCtrl', OriginGameViolatorTrialexpiredCtrl)
        .directive('originGameViolatorTrialexpired', originGameViolatorTrialexpired);
}());