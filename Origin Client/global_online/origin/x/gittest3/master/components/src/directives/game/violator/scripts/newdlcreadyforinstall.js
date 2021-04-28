/**
 * @file game/violator/newdlcreadyforinstall.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-newdlcreadyforinstall';

    function OriginGameViolatorNewdlcreadyforinstallCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorNewdlcreadyforinstall(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorNewdlcreadyforinstallCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/newdlcreadyforinstall.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorNewdlcreadyforinstall
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * New DLC Ready for Install violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-newdlcreadyforinstall></origin-game-violator-newdlcreadyforinstall>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorNewdlcreadyforinstallCtrl', OriginGameViolatorNewdlcreadyforinstallCtrl)
        .directive('originGameViolatorNewdlcreadyforinstall', originGameViolatorNewdlcreadyforinstall);
}());