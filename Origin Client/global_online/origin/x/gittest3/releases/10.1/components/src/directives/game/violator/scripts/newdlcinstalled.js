/**
 * @file game/violator/newdlcinstalled.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-newdlcinstalled';

    function OriginGameViolatorNewdlcinstalledCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorNewdlcinstalled(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorNewdlcinstalledCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/newdlcinstalled.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorNewdlcinstalled
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * New DLC Installed violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-newdlcinstalled></origin-game-violator-newdlcinstalled>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorNewdlcinstalledCtrl', OriginGameViolatorNewdlcinstalledCtrl)
        .directive('originGameViolatorNewdlcinstalled', originGameViolatorNewdlcinstalled);
}());