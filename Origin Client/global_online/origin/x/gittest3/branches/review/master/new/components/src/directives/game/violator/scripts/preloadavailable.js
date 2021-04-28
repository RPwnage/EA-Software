/**
 * @file game/violator/preloadavailable.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-preloadavailable';

    function OriginGameViolatorPreloadavailableCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorPreloadavailable(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorPreloadavailableCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/preloadavailable.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorPreloadavailable
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * Preload available violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-preloadavailable></origin-game-violator-preloadavailable>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorPreloadavailableCtrl', OriginGameViolatorPreloadavailableCtrl)
        .directive('originGameViolatorPreloadavailable', originGameViolatorPreloadavailable);
}());