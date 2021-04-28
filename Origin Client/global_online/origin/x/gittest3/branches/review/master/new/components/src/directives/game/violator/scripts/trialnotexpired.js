/**
 * @file game/violator/trialnotexpired.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-trialnotexpired';

    function OriginGameViolatorTrialnotexpiredCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorTrialnotexpired(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorTrialnotexpiredCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/trialnotexpired.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorTrialnotexpired
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * Trial Not Expired violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-trialnotexpired></origin-game-violator-trialnotexpired>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorTrialnotexpiredCtrl', OriginGameViolatorTrialnotexpiredCtrl)
        .directive('originGameViolatorTrialnotexpired', originGameViolatorTrialnotexpired);
}());