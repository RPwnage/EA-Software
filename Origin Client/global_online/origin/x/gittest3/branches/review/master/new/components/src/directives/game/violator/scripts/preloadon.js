/**
 * @file game/violator/preloadon.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-preloadon';

    function OriginGameViolatorPreloadonCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorPreloadon(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorPreloadonCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/preloadon.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorPreloadon
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * Preload on violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-preloadon></origin-game-violator-preloadon>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorPreloadonCtrl', OriginGameViolatorPreloadonCtrl)
        .directive('originGameViolatorPreloadon', originGameViolatorPreloadon);
}());