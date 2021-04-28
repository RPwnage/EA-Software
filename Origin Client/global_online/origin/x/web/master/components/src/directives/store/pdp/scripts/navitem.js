/**
 * @file store/pdp/scripts/navitem.js
 */
(function(){
    'use strict';

    /**
     * BooleanTypeEnumeration list of allowed types
     * @enum {string}
     */
    /* jshint ignore:start */
    var BooleanTypeEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    /**
    * The controller
    */
    function OriginStorePdpNavitemCtrl($scope, AppCommFactory) {
        // let anyone listening know that a new navigation point has been added
        // if it is meant to be included in a nav-bar
        if ($scope.showOnNav === 'true') {
            AppCommFactory.events.fire('navitem:added', {
                anchorName: $scope.anchorName,
                title: $scope.title
            });
        }
    }

    function originStorePdpNavitem(AppCommFactory, ComponentsConfigFactory, $timeout, $location) {
        /**
         * scroll down to navigation point
         */
        function scrollTo(target) {
            // if requested target is a valid element...
            var reqTarget = angular.element('#' + target);

            if (angular.isDefined(reqTarget.offset())) {
                // get position of requested element
                var elemPosition = reqTarget.offset().top;

                    // determine "top" based on offset of gus and pdp nav
                var offset = angular.element('.l-origin-storenav').height() +
                    angular.element('.store-pdp-nav').height();

                    // determine final position to scroll to
                var scrollPosition = elemPosition - offset;

                // scroll to module position
                angular.element('html, body').animate({ scrollTop: scrollPosition }, 'fast');
            }
        }

        function originStorePdpNavitemLink(scope) {
            function onHashChange() {
                var currentHash = $location.hash();

                if (currentHash === scope.anchorName) {
                    scrollTo(currentHash);
                }
            }

            // on manual URL hash-change, use custom scrolling
            AppCommFactory.events.on('navitem:hashChange', onHashChange);

            // attempt to scroll to this navitem when it is deep linked
            $timeout(onHashChange, 500);
        }

        return {
            restrict: 'E',
            scope: {
                anchorName: '@',
                showOnNav: '@'
            },
            controller: 'OriginStorePdpNavitemCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/navitem.html'),
            link: originStorePdpNavitemLink,
            replace: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpNavitem
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {BooleanTypeEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @description Creates a navigation point for deep-linking on a page, and emits its presence for inclusion in pdp nav.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-navitem title="Friends Who Play" anchor-name="friends" show-on-nav="true"></origin-store-pdp-navitem>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpNavitemCtrl', OriginStorePdpNavitemCtrl)
        .directive('originStorePdpNavitem', originStorePdpNavitem);
}());
