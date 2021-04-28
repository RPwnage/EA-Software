/**
 * @file game/violator/gamesunsetted.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-gamesunsetted';

    function OriginGameViolatorGamesunsettedCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorGamesunsetted(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorGamesunsettedCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/gamesunsetted.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorGamesunsetted
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * Game sunsetted violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-gamesunsetted></origin-game-violator-gamesunsetted>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorGamesunsettedCtrl', OriginGameViolatorGamesunsettedCtrl)
        .directive('originGameViolatorGamesunsetted', originGameViolatorGamesunsetted);
}());