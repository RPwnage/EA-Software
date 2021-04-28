/**
 * Responsible for handling all things dropdown related
 * @file dropdown-item.js
 */

(function() {
    'use strict';

    function originDropdownitem(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                clickAction: '&'
            },
            transclude: true,
            replace: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/dropdownitem.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDropdownitem
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} click-action the action to run on click
     * @description
     *
     * An item for a dropdown
     *
     * @example
     *
     *   <origin-dropdown dropdown-id="miniprofile-dropdown-wrap" labelled-by="miniprofile-dropdown">
     *       <origin-dropdownitem click-action="logout()">{{myProfileStr}}</origin-dropdownitem>
     *   </origin-dropdown>
     *
     */
    angular.module('origin-components')
        .directive('originDropdownitem', originDropdownitem);

}());