/**
 * @file gamelibrary/nogamesonlineunderage.js
 */

(function() {
    'use strict';

    function originGamelibraryNoGamesOnlineUnderAge(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/nogamesonlineunderage.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryNoGamesOnlineUnderAge
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * A container for alternate content in the case the user has no games in their library (online) and is under age
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-no-games-underage>
     *             <marketing-directives here></marketing-directives-here>
     *         </origin-gamelibrary-no-games-underage>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originGamelibraryNoGamesOnlineUnderAge', originGamelibraryNoGamesOnlineUnderAge);
}());