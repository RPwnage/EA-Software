/** 
 * @file store/scripts/nav/item.js
 */ 
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStoreNavItem() {

        return {
            restrict: 'E',
            replace: true,
            scope: {
            	title: "@",
            	href: "@"
            },
            template:   '<li class="origin-storenav-item">'+
                            '<a class="otktitle-5" ng-href="{{href}}">{{title}}</a>'+
                        '</li>'
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreNavItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} title Title of the item.
     * @param {string} href Link for the item.
     * @description Store nav item
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-nav-item title="Store Home" href="#/store"><origin-store-nav-item>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreNavItem', originStoreNavItem);
}()); 