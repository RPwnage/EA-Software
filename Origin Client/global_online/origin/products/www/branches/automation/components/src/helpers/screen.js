/**
 * @file factories/common/screen.js
 */
(function() {
    'use strict';

    var SCROLLING_DISABLED_CLASS = 'origin-scrolling-disabled';

    function ScreenFactory(BREAKPOINTS) {
        var bodyElement = angular.element('body'),
            scrollPosition = 0;

        function hasMaxWidth(maxWidth) {
            return (window.matchMedia('(max-width: ' + maxWidth + 'px)').matches);
        }

        function hasMinWidth(minWidth) {
            return (window.matchMedia('(min-width: ' + minWidth + 'px)').matches);
        }

        function isPdpLarge() {
            return !hasMaxWidth(BREAKPOINTS.STORE.large);
        }

        function isXSmall() {
            return hasMaxWidth(BREAKPOINTS.GLOBAL.xsmall);
        }

        function isSmall() {
            return hasMaxWidth(BREAKPOINTS.GLOBAL.small) || (window.innerWidth === BREAKPOINTS.GLOBAL.small);
        }

        function isLarge(){
            return hasMinWidth(BREAKPOINTS.GLOBAL.large) || (window.innerWidth >= BREAKPOINTS.GLOBAL.large);
        }

        /**
         * Return true if element is on screen
         * @param {Element} elm jQlite element
         * @return {boolean}
         */
        function isOnScreen(elm) {
            // fix for elements that do not exist in the DOM
            if(angular.element(elm).length === 0) {
                return false;
            }

            var win = angular.element(window),
                elem = angular.element(elm),
                vtop = win.scrollTop(),
                vleft = win.scrollLeft(),
                vright = vleft + win.width(),
                vbottom = vtop + win.height(),
                bleft = elem.offset().left,
                btop = elem.offset().top,
                bright = bleft + elem.outerWidth(),
                bbottom = btop + elem.outerHeight();

            return (!(vright < bleft || vleft > bright || vbottom < btop || vtop > bbottom));
        }

        function isAboveScreen(elm) {
            var vtop = angular.element(window).scrollTop();
            var btop = angular.element(elm).offset().top;
            var bbottom = btop + angular.element(elm).outerHeight();
            return (bbottom < vtop );
        }

        function isOnOrAboveScreen(elm) {
            return isOnScreen(elm) || isAboveScreen(elm);
        }

        /**
         * scroll down to navigation point
         * @param target target element
         * @param offset offset from element position
         * @param animationSpeed animation speed
         */
        function scrollTo(target, offset, animationSpeed) {
            // if requested target is a valid element...
            var reqTarget = angular.element('#' + target);

            if (reqTarget.length && angular.isDefined(reqTarget.offset)) {
                // get position of requested element
                var elemPosition = reqTarget.offset().top;

                // determine final position to scroll to
                var scrollPosition = elemPosition - (offset || 0);

                // scroll to module position
                angular.element('html, body').animate({scrollTop: scrollPosition}, (animationSpeed || 'fast'));
            }
        }

        function disableBodyScrolling() {
            // disable scrolling by adding a class to the body and caching the scroll position
            scrollPosition = window.pageYOffset;
            bodyElement.css('top', '-' + scrollPosition + 'px').addClass(SCROLLING_DISABLED_CLASS);
        }

        function enableBodyScrolling() {
            // enable scrolling by removing disabled class from the body and reseting the scroll position
            bodyElement.css('top', '').removeClass(SCROLLING_DISABLED_CLASS);
            window.scrollTo(0, scrollPosition);
        }

        /**
         * sets up a listener that is triggered when we switch to/from mobile size
         * @param {function} listener the function that is triggered when we cross the mobile break point
         * @return {function} the remove function to call to remove the listener
         */
        function addSmallMediaQueryListener(listener) {
            var mediaQuery = window.matchMedia('(max-width: ' + BREAKPOINTS.GLOBAL.small + 'px)');
            mediaQuery.addListener(listener);
            return function() {
                mediaQuery.removeListener(listener);
            };
        }

        return {
            isXSmall: isXSmall,
            isSmall: isSmall,
            isLarge: isLarge,
            isOnScreen: isOnScreen,
            isAboveScreen: isAboveScreen,
            isOnOrAboveScreen: isOnOrAboveScreen,
            scrollTo: scrollTo,
            isPdpLarge: isPdpLarge,
            disableBodyScrolling: disableBodyScrolling,
            enableBodyScrolling: enableBodyScrolling,
            addSmallMediaQueryListener: addSmallMediaQueryListener
        };
    }

    /**
     *
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     * @description Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     */
    function ScreenFactorySingleton(BREAKPOINTS, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ScreenFactory', ScreenFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ScreenFactory
     * @description factory that provides utilities to check screen size width
     *
     * Object manipulation functions
     */
    angular.module('origin-components')
        .factory('ScreenFactory', ScreenFactorySingleton);
}());
