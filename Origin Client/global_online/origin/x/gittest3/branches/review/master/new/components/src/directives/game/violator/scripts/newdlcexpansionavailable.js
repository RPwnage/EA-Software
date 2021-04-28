/**
 * @file game/violator/newdlcexpansionavailable.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-newdlcexpansionavailable';

    function OriginGameViolatorNewdlcexpansionavailableCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorNewdlcexpansionavailable(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorNewdlcexpansionavailableCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/newdlcexpansionavailable.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorNewdlcexpansionavailable
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * New DLC Expansion Available violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-newdlcexpansionavailable></origin-game-violator-newdlcexpansionavailable>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorNewdlcexpansionavailableCtrl', OriginGameViolatorNewdlcexpansionavailableCtrl)
        .directive('originGameViolatorNewdlcexpansionavailable', originGameViolatorNewdlcexpansionavailable);
}());