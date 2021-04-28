/**
 * @file game/violator/releaseson.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-releaseson';

    function OriginGameViolatorReleasesonCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorReleaseson(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorReleasesonCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/releaseson.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorReleaseson
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * Releases on violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-releaseson></origin-game-violator-releaseson>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorReleasesonCtrl', OriginGameViolatorReleasesonCtrl)
        .directive('originGameViolatorReleaseson', originGameViolatorReleaseson);
}());