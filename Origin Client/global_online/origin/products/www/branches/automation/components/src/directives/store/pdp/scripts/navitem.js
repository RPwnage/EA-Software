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
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */


    function originStorePdpNavitem(AppCommFactory, ComponentsConfigFactory, QueueFactory, ScreenFactory, $timeout, $location) {
        var stripeMargin = 0;

        function getNavHeight() {
            return angular.element('.store-pdp-nav').height() + stripeMargin;
        }

        function originStorePdpNavitemLink(scope) {
            scope.setStripeMargin = function(margin) {
                stripeMargin = margin;
            };

            function onHashChange() {
                var currentHash = $location.hash();

                if (currentHash === scope.anchorName) {
                    ScreenFactory.scrollTo(currentHash, getNavHeight());
                }
            }

            // let anyone listening know that a new navigation point has been added
            // if it is meant to be included in a nav-bar
            $timeout(function()  {
                QueueFactory.enqueue('navitem:added', {
                    anchorName: scope.anchorName,
                    anchorTitle: scope.titleStr,
                    showOnNav: scope.showOnNav
                });
            }, 0, false );


            // on manual URL hash-change, use custom scrolling
           var unbindHashChangeEventListener = AppCommFactory.events.on('navitem:hashChange', onHashChange);

            // attempt to scroll to this navitem when it is deep linked
           var  cancelTimeout = $timeout(onHashChange, 500);
            
            function onDestroy() {
                unbindHashChangeEventListener.detach();
                $timeout.cancel(cancelTimeout);
            }
            
            scope.$on('$destroy', onDestroy);
        }

        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                anchorName: '@',
                showOnNav: '@'
            },
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
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @description Creates a navigation point for deep-linking on a page, and emits its presence for inclusion in pdp nav.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-navitem title-str="Friends Who Play" anchor-name="friends" show-on-nav="true"></origin-store-pdp-navitem>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpNavitem', originStorePdpNavitem);
}());