/**
 * @file game/violator/gametobesunset.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-gametobesunset';

    function OriginGameViolatorGametobesunsetCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorGametobesunset(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorGametobesunsetCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/gametobesunset.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorGametobesunset
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * Game to be sunset violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-gametobesunset></origin-game-violator-gametobesunset>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorGametobesunsetCtrl', OriginGameViolatorGametobesunsetCtrl)
        .directive('originGameViolatorGametobesunset', originGameViolatorGametobesunset);
}());