/**
 * @file /store/scripts/lazyload.js
 */
(function() {
    'use strict';

    var SCROLL_THROTTLE_MS = 350;

    function originLazyload($parse, $window, UtilFactory, $compile) {

        function originLazyloadLink(scope, elem, attrs) {
        
            var loaderElement = $compile('<origin-loaderindicator></origin-loaderindicator>')(scope);
            //attrs.originLazyload function must be defined at controller level for this to work.
            var promise = scope.$eval(attrs.originLazyload);
            var lazyLoadDisabled = $parse(attrs.originLazyloadDisabled);
            // Throttle polling to 500ms interval to reduce event spam and space out "loadMore" query rate to a reasonable minimum
            var scrollUpdateThrottleRate = attrs.originLazyloadThrottle || SCROLL_THROTTLE_MS,
                lazyLoadEnabled = false,
                // Allow for a pixel offset from the bottom in case the loadMoreFn should be triggered a bit higher up
                offsetFromBottom = attrs.originLazyloadOffset || 0,
                containerElement = angular.element(attrs.originLazyloadContainer || $window),
                scrollEventBound = false;

            /**
             * Checks whether the user has scrolled past the bottom of the container.
             * "offsetFromBottom" allows to add the threshold to be higher up from the bottom
             * of the container so the loadMore function is called sooner
             * @return {boolean}
             */
            function scrolledPastThreshold() {
                var threshold = (elem.offset().top + elem.outerHeight()) - offsetFromBottom;
                var scrollPos = containerElement.scrollTop() + containerElement.innerHeight();
                return scrollPos >= threshold;
            }

            /**
             * Handles scroll event and fires the "load more" function reference if scrolling past the threshold
             * @return {void}
             */
            function scrollHandler() {
                if (!lazyLoadDisabled(scope) && scrolledPastThreshold()) {
                    addLoading();
                    promise().then(clearLoading).catch(clearLoading);
                }
            }
            function addLoading() {
                elem.append(loaderElement);
            }

            function clearLoading() {
                angular.element(elem.find('origin-loaderindicator')).remove();
            }

            var throttledScrollHandler = UtilFactory.throttle(scrollHandler, scrollUpdateThrottleRate);

            /**
             * Bind scroll handler if it wasn't already
             * @return {void}
             */
            function bindScrollHandler() {
                if (!scrollEventBound) {
                    containerElement.on('scroll', throttledScrollHandler);
                    scrollEventBound = true;
                }
            }

            /**
             * Unbind scroll handler if it is currently bound
             * @return {void}
             */
            function unbindScrollHandler() {
                if (scrollEventBound) {
                    containerElement.off('scroll', throttledScrollHandler);
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
            scope.$watch(lazyLoadDisabled(scope), updateFlagAndBindings);

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
     *
     * @param {Function} origin-lazyload function that returns a promise. Must be defined in controller
     * @param {Function} origin-lazyload-disabled whether lazyloading is disabled/enabled
     * @param {Function} origin-lazyload-offset offset from parent container
     * @param {string} origin-lazyload-container container that scrolls (Defaults to window). Should only be specified if lazy loading in overlay
     * @param {Number} origin-lazyload-throttle delay )in Milliseconds) before invoking origin-lazyload function
     *
     * @description
     *
     * Lazy load directive that will execute the promise attached to origin-lazyload when user scrolls past the
     * bottom of the element upon which the directive is added
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-tile-list origin-lazyload="loadMore" origin-lazyload-disabled="lazyLoadDisabled()" origin-lazyload-offset="200" origin-lazyload-throttle="500">
     *            <li class="origin-tilelist-item" ng-repeat="item in games" origin-postrepeat>
     *                 <origin-store-game-tile ocd-path="{{item.path}}" type="responsive"></origin-store-game-tile>
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
