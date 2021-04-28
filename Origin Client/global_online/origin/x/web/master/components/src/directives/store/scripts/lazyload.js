/**
 * @file /store/scripts/lazyload.js
 */
(function() {
    'use strict';

    var SCROLL_THROTTLE_MS = 350;

    function originLazyload($timeout, $window, UtilFactory) {

        function originLazyloadLink(scope, elem, attrs) {
                // Throttle polling to 500ms interval to reduce event spam and space out "loadMore" query rate to a reasonable minimum
            var scrollUpdateThrottleRate = attrs.originLazyloadThrottle || SCROLL_THROTTLE_MS,
                lazyLoadEnabled = false,
                // Allow for a pixel offset from the bottom in case the loadMoreFn shuld be triggered a bit higher up
                offsetFromBottom = attrs.originLazyloadOffset || 0,
                windowElement = angular.element($window),
                scrollEventBound = false;

            /**
             * Checks whether the user has scrolled past the bottom of the container.
             * "offsetFromBottom" allows to add the threshold to be higher up from the bottom
             * of the container so the loadMore function is called sooner
             * @return {boolean}
             */
            function scrolledPastThreshold() {
                var threshold = (elem.offset().top + elem.outerHeight()) - offsetFromBottom;
                var scrollPos = windowElement.scrollTop() + window.innerHeight;
                return scrollPos >= threshold;
            }

            /**
             * Handles scroll event and fires the "load more" function reference if scrolling past the threshold
             * @return {void}
             */
            function scrollHandler() {
                if (lazyLoadEnabled && scrolledPastThreshold()) {
                    scope.$eval(attrs.originLazyload);
                }
            }

            var throttledScrollHandler = UtilFactory.throttle(scrollHandler, scrollUpdateThrottleRate);

            /**
             * Bind scroll handler if it wasn't already
             * @return {void}
             */
            function bindScrollHandler() {
                if (!scrollEventBound) {
                    windowElement.on('scroll', throttledScrollHandler);
                    scrollEventBound = true;
                }
            }

            /**
             * Unbind scroll handler if it is currently bound
             * @return {void}
             */
            function unbindScrollHandler() {
                if (scrollEventBound) {
                    windowElement.off('scroll', throttledScrollHandler);
                    scrollEventBound = false;
                }
            }

            /**
             * Update enabled flag and bind scroll-handler if true, unbind if false
             * @return {void}
             */
            function updateFlagAndBindings(value) {
                lazyLoadEnabled = !value;
                if (lazyLoadEnabled) {
                    bindScrollHandler();
                } else {
                    unbindScrollHandler();
                }
            }
            scope.$watch(attrs.originLazyloadDisabled, updateFlagAndBindings);

            scope.$on('$destroy', unbindScrollHandler);
        }

        return {
            restrict: 'A',
            link: originLazyloadLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originLazyload
     * @restrict A
     * @element ANY
     * @scope
     * @description
     *
     * Lazy load directive that will call your "load more" function when user scrolls past the
     * bottom of the element upon which the directive is added
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-tile-list origin-lazyload="loadMore()" origin-lazyload-disabled="lazyLoadDisabled()" origin-lazyload-offset="200" origin-lazyload-throttle="500">
     *            <li class="origin-tilelist-item" ng-repeat="item in games" origin-postrepeat>
     *                 <origin-store-game-tile offer-id="{{item.offerId}}" type="responsive"></origin-store-game-tile>
     *            </li>
     *          </origin-tile-list>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originLazyload', originLazyload);
}());
