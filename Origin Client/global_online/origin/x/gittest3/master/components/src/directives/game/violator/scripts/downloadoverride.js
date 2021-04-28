/**
 * @file game/violator/downloadoverride.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-violator-downloadoverride';

    function OriginGameViolatorDownloadoverrideCtrl($scope, UtilFactory) {
        $scope.violatorText = UtilFactory.getLocalizedStr($scope.violatorStr, CONTEXT_NAMESPACE, 'violatorstr');
    }

    function originGameViolatorDownloadoverride(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                violatorStr: '@violatorstr'
            },
            controller: 'OriginGameViolatorDownloadoverrideCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/downloadoverride.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameViolatorDownloadoverride
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} violatorstr string for the violator text
     * @description
     *
     * download override violator
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-violator-downloadoverride></origin-game-violator-downloadoverride>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGameViolatorDownloadoverrideCtrl', OriginGameViolatorDownloadoverrideCtrl)
        .directive('originGameViolatorDownloadoverride', originGameViolatorDownloadoverride);
}());