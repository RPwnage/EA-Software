/**
 * @file gamelibrary/navigationitem.js
 */
(function() {
    'use strict';

    function originGamelibraryNavigationitem(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                title: '@',
                iconClass: '@',
                active: '&'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/navigationitem.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryNavigationitem
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} title menu item label
     * @param {string} iconClass CSS class for item's icon
     * @param {boolean} active flag indicating whether the item is in active state or not
     * @description
     *
     * GameNavigation bar item
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-navigationitem title="{{addGameStr}}" icon-class="otkicon-menu" active="isFilterPanelOpen"></origin-gamelibrary-navigationitem>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originGamelibraryNavigationitem', originGamelibraryNavigationitem);
}());