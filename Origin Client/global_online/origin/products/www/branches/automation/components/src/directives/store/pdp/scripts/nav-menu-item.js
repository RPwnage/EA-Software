/**
 * @file store/pdp/scripts/nav-menu-item.js
 */
(function (){
    'use strict';
    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function originStorePdpNavMenuItem(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                titleStr: '@',
                onClick: '&',
                isActive: '&'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/nav-menu-item.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpNavMenuItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str the title of the menu item
     * @param {Function} on-click the function to call when the menu item is clicked
     * @param {BooleanEnumeration} is-active true if the menu item is currently active, false otherwise
     * @description
     *
     * PDP nagigation menu item
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-nav>
     *         <origin-store-pdp-nav-menu-item ng-repeat="item in items"
     *             title-str="{{ item.title }}"
     *             is-active="isItemActive(item)"
     *             on-click="navClickCallback()">
     *         </origin-store-pdp-nav-menu-item>
     *     </origin-store-pdp-nav>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpNavMenuItem', originStorePdpNavMenuItem);
}());
