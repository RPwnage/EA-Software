/**
 * @file menu/dropdown-menu.js
 */
(function() {
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function originDropdownMenu(ComponentsConfigFactory) {
        function originDropdownMenuLink(scope, elem) {
            scope.setMargin = function(margin) {
                elem.find('.otkcontextmenu-wrap').css('margin-top', (-1 * margin) + 'px');
            };
        }
        return {
            restrict: 'E',
            scope: {
                items: '&',
                left: '=?',
                top: '=?',
                bottom: '=?',
                isVisible: '=?'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('menu/views/dropdown-menu.html'),
            link: originDropdownMenuLink
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
     *                      tealiumType: Gift|Wishlist - Add| Wishlist - Remove|Buy Now
     *                  }
     * @param {string} left menu on-scrin position: x ccordinate
     * @param {string} top menu on-scrin position: y ccordinate
     * @param {BooleanEnumeration} visible flag indicating whether the menu should appear on the screen or not
     * @scope
     * @description
     *
     * general purpose drop-down menu
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dropdown-menu items="currentActions" is-visible="true"></origin-dropdown-menu>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originDropdownMenu', originDropdownMenu);

}());