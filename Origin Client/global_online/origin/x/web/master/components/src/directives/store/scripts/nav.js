/**
 * @file store/scripts/nav.js
 */
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStoreNav() {

        function originStoreNavLink(scope, elem) {
            elem.find('.origin-storenav > .otknav-pills > li').addClass("otkpill origin-storenav-otkpill");
        }

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            template:   '<div class="l-origin-storenav"><nav role="navigation" class="origin-storenav">'+
                            '<ul class="otknav otknav-pills" ng-transclude></ul>'+
                        '</nav></div>',
            link: originStoreNavLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreNav
     * @restrict E
     * @element ANY
     * @scope
     * @description Store nav container
     *
     * Store navigation container.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-nav />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreNav', originStoreNav);
}());