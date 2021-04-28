/**
 * @file game/download/icon.js
 */
(function() {
    'use strict';

    /**
     * Download Game directive
     * @directive originGameDownloadIcon
     */
    function originGameDownloadIcon(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginCtaDownloadinstallplayCtrl',
            scope: {
                offerid: '@offerid'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/download/views/icon.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameDownloadIcon
     * @restrict E
     * @element ANY
     * @param {string} offerid the offerid of the game
     * @scope
     *
     * quick download icon
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-download-icon offerid="OFB-EAST:109552154"></origin-game-download-icon>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originGameDownloadIcon', originGameDownloadIcon);
}());