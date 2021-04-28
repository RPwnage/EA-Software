/**
 * @file factories/common/animate.js
 */
(function () {
    'use strict';

    var FOCUSIN_EVENT = 'focusin';
    var FOCUSOUT_EVENT = 'focusout';

    function AnimateFactory($window, FeatureDetectionFactory, UtilFactory) {

        var requestAnimationFrame = $window.requestAnimationFrame ||
                $window.mozRequestAnimationFrame ||
                $window.msRequestAnimationFrame ||
                $window.webkitRequestAnimationFrame,
            cancelAnimationFrame = $window.cancelAnimationFrame ||
                $window.mozCancelAnimationFrame ||
                $window.msCancelAnimationFrame ||
                $window.webkitCancelAnimationFrame;


        /**
         * Internal Event handler
         * @param event
         * @param scope
         * @param animateCallback
         * @param throttleDelay default is 10 ms
         */
        function addEventHandler(event, scope, animateCallback, throttleDelay) {

            var isAnimating = false,
                requestAnimationFrameHandle;

            /**
             * Executes the callback when the next animation frame is available
             * @return {void}
             */
            function requestAnimation(callback) {
                if (isAnimating) {
                    return;
                }

                isAnimating = true;

                requestAnimationFrameHandle = requestAnimationFrame(function () {
                    callback();
                    isAnimating = false;
                });
            }

            /**
             * When animation request has been granted, call animationCallback
             */
            function callOnEvent() {
                UtilFactory.throttle(requestAnimation(animateCallback), (throttleDelay || 10));
            }

            /**
             *  remove scroll event on destroy
             */
            function removeEventHandlerOnDestroy() {
                if (scope) {
                    scope.$on('$destroy', function () {
                        cancelAnimationFrame(requestAnimationFrameHandle);
                        window.removeEventListener(event, callOnEvent, false);
                    });
                }
            }

            if (animateCallback) {
                window.addEventListener(event, callOnEvent, false);
                removeEventHandlerOnDestroy();
            }

        }

        function addResizeEventHandler(scope, animateCallback, throttleDelay) {
            addEventHandler('resize', scope, animateCallback, throttleDelay);
        }

        function addVisibilityChangeEventHandler(scope, animateCallback) {
            if (FeatureDetectionFactory.browserSupports('visibilitychange')) {
                var jQueryWindow = angular.element(window);
                jQueryWindow.on(FOCUSIN_EVENT, animateCallback)
                            .on(FOCUSOUT_EVENT, animateCallback);
                if (scope) {
                    scope.$on('$destroy', function () {
                        jQueryWindow.off(FOCUSIN_EVENT, animateCallback)
                                    .off(FOCUSOUT_EVENT, animateCallback);
                    });
                }
            }
        }

        function removeVisibilityChangeEventHandler(animateCallback) {
            angular.element(window).off(FOCUSIN_EVENT, animateCallback)
                .off(FOCUSOUT_EVENT, animateCallback);

        }

        /**
         *
         * @param scope to unregister scroll event listener on $destroy
         * @param animateCallback action to be performed when animation request has been granted
         * @param throttleDelay
         */
        function addScrollEventHandler(scope, animateCallback, throttleDelay) {
            addEventHandler('scroll', scope, animateCallback, throttleDelay);
        }

        return {
            addScrollEventHandler: addScrollEventHandler,
            addResizeEventHandler: addResizeEventHandler,
            addVisibilityChangeEventHandler: addVisibilityChangeEventHandler,
            removeVisibilityChangeEventHandler: removeVisibilityChangeEventHandler
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.AnimateFactory
     * @description factory that provides on scroll functionality and takes performance and device limitations into consideration
     *
     *
     * Object manipulation functions
     */
    angular.module('origin-components')
        .factory('AnimateFactory', AnimateFactory);
}());

