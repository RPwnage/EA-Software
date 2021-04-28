/**
 * @file menu/dropdown-menu.js
 */
(function() {
    'use strict';

    function originDropdownMenu(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                items: '&',
                left: '=?',
                top: '=?',
                visible: '=?'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('menu/views/dropdown-menu.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDropdownMenu
     * @restrict E
     * @element ANY
     * @param {array} items array of menu item objects of following format
     *                  {
     *                      label: 'some text to show in the menu'
     *                      callback: function() {...item click handler...}
     *                      enabled: true|false
     *                  }
     * @param {string} left menu on-scrin position: x ccordinate
     * @param {string} top menu on-scrin position: y ccordinate
     * @param {boolean} visible flag indicating whether the menu should appear on the screen or not
     * @scope
     * @description
     *
     * general purpose drop-down menu
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dropdown-menu items="currentActions" visible="true"></origin-dropdown-menu>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originDropdownMenu', originDropdownMenu);

}());