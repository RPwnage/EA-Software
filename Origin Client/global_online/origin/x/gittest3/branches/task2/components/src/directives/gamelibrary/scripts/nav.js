/**
 * @file gamelibrary/nav.js
 */
(function() {
    'use strict';

    function originGamelibraryNav(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/nav.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryNav
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * horizontal navigation menu
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-nav>
     *             <origin-gamelibrary-navigationitem title="{{addGameStr}}" icon-class="otkicon-add" active="false"></origin-gamelibrary-navigationitem>
     *         </origin-gamelibrary-nav>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originGamelibraryNav', originGamelibraryNav);
}());