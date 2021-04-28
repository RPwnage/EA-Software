/**
 * @file game/violator/updateavailable.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-updateavailable';

    function OriginGameViolatorUpdateavailableCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorUpdateavailable(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorUpdateavailableCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/updateavailable.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorUpdateavailable
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * Update Available violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-updateavailable></origin-game-violator-updateavailable>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorUpdateavailableCtrl', OriginGameViolatorUpdateavailableCtrl)
        .directive('originGameViolatorUpdateavailable', originGameViolatorUpdateavailable);
}());