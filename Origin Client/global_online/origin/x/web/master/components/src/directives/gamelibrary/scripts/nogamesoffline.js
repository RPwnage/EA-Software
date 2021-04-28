/**
 * @file gamelibrary/nogamesoffline.js
 */

(function() {
    'use strict';

    function originGamelibraryNoGamesOffline(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/nogamesoffline.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryNoGamesOffline
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * A container for alternate content in the case the user has no games in their library (offline)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-no-games>
     *             <marketing-directives here></marketing-directives-here>
     *         </origin-gamelibrary-no-games>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originGamelibraryNoGamesOffline', originGamelibraryNoGamesOffline);
}());