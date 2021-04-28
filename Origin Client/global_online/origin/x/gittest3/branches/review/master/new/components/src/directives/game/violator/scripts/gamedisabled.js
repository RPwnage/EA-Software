/**
 * @file game/violator/gamedisabled.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-gamedisabled';

    function OriginGameViolatorGamedisabledCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorGamedisabled(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorGamedisabledCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/gamedisabled.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorGamedisabled
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * Game disabled violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-gamedisabled></origin-game-violator-gamedisabled>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorGamedisabledCtrl', OriginGameViolatorGamedisabledCtrl)
        .directive('originGameViolatorGamedisabled', originGameViolatorGamedisabled);
}());