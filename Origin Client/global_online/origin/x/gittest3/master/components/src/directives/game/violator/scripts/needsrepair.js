/**
 * @file game/violator/needsrepair.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-needsrepair';

    function OriginGameViolatorNeedsrepairCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorNeedsrepair(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorNeedsrepairCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/needsrepair.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorNeedsrepair
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * Needs repair violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-needsrepair></origin-game-violator-needsrepair>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorNeedsrepairCtrl', OriginGameViolatorNeedsrepairCtrl)
        .directive('originGameViolatorNeedsrepair', originGameViolatorNeedsrepair);
}());