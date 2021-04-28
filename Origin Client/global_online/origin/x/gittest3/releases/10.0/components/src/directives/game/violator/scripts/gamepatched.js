/**
 * @file game/violator/gamepatched.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-gamepatched';

    function OriginGameViolatorGamepatchedCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorGamepatched(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorGamepatchedCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/gamepatched.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorGamepatched
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * game patched violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-gamepatched></origin-game-violator-gamepatched>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorGamepatchedCtrl', OriginGameViolatorGamepatchedCtrl)
        .directive('originGameViolatorGamepatched', originGameViolatorGamepatched);
}());