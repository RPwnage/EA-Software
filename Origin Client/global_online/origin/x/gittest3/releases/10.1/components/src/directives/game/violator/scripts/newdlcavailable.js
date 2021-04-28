/**
 * @file game/violator/newdlcavailable.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-newdlcavailable';

    function OriginGameViolatorNewdlcavailableCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorNewdlcavailable(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorNewdlcavailableCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/newdlcavailable.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorNewdlcavailable
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * New DLC Available violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-newdlcavailable></origin-game-violator-newdlcavailable>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorNewdlcavailableCtrl', OriginGameViolatorNewdlcavailableCtrl)
        .directive('originGameViolatorNewdlcavailable', originGameViolatorNewdlcavailable);
}());