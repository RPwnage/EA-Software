/**
 * @file game/violator/gameexpires.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-gameexpires';

    function OriginGameViolatorGameexpiresCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorGameexpires(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorGameexpiresCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/gameexpires.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorGameexpires
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * game expires violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-gameexpires></origin-game-violator-gameexpires>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorGameexpiresCtrl', OriginGameViolatorGameexpiresCtrl)
        .directive('originGameViolatorGameexpires', originGameViolatorGameexpires);
}());