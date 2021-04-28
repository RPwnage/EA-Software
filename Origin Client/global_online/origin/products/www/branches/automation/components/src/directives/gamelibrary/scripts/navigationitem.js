/**
 * @file gamelibrary/navigationitem.js
 */
(function() {
    'use strict';

    function originGamelibraryNavigationitem(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
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
     * @param {string} title-str menu item label passed from loggedin.js - this field is not inteded for merchandising
     * @param {string} icon-class CSS class for item's icon passed in from loggedin.js
     * @param {boolean} active flag indicating whether the item is in active state or not - this is a javascript reference only
     * @description
     *
     * GameNavigation bar item - this is a partial of loggedin.js and nav.js and not directly merchandised
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-navigationitem title-str="{{addGameStr}}" icon-class="otkicon-menu" active="isFilterPanelOpen"></origin-gamelibrary-navigationitem>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originGamelibraryNavigationitem', originGamelibraryNavigationitem);
}());
