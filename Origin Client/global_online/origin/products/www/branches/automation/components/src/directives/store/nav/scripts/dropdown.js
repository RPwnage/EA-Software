/**
 * @file store/scripts/nav/dropdown.js
 */
(function(){
    'use strict';

    function originStoreNavDropdown(NavigationFactory) {

        function originStoreNavDropdownLink(scope, elem) {
            // Add class origin-storenavdropdown-item to all transclude <li>s
            elem.find('.otkmenu-dropdown:first > li').addClass("origin-storenav-dropdown-item");
            scope.openLink = function(href, $event){
                $event.preventDefault();
                NavigationFactory.openLink(href);
            };
            scope.absoluteUrl = NavigationFactory.getAbsoluteUrl(scope.href);
        }

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                titleStr: "@",
                href: "@"
            },
            template:   '<li class="origin-storenav-dropdown">'+
                            '<a class="otktitle-5 otkdropdown-trigger otkdropdown-trigger-caret" ng-href="{{::absoluteUrl}}" ng-click="openLink(href, $event)" ng-bind="titleStr"></a>'+
                            '<div class="otkdropdown otkdropdown-wrap">'+
                                '<ul class="otkmenu otkmenu-dropdown otkmenu-dropdown-filter" role="menu" ng-transclude></ul>'+
                            '</div>'+
                        '</li>',
            link: originStoreNavDropdownLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreNavDropdown
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str Title of the dropdown parent item.
     * @param {string} href Link for the dropdown parent item.
     * @description Store nav dropdown
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-nav-dropdown title-str="Store Home" href="store"><origin-store-nav-dropdown>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreNavDropdown', originStoreNavDropdown);
}());
