/**
 * @file directives/pagetransition.js
 */
(function () {
    'use strict';

    var TIMEOUT_MS = 60, // match CSS transition duration
        TRANSITION_LOADING_CLASS = 'origin-page-transition-loading',
        TRANSITION_HIDDEN_CLASS = 'origin-page-transition-hidden',
        TRANSITION_ZOOM_CLASS = 'origin-page-transition-zoom',
        TRANSITION_SHRINK_CLASS = 'origin-page-transition-shrink',
        TRANSITION_NONE_CLASS = 'origin-page-transition-none';

    function originPageTransition(AppCommFactory, EventsHelperFactory, $timeout, $anchorScroll) {

        function originPageTransitionLink(scope, element) {
            var initialLoad = true;

            /**
             * After transitioning away from a page, zoom the view in anticipation of fading into the next page
             */
            function startTransitionIn() {
                element.removeClass(TRANSITION_SHRINK_CLASS).addClass(TRANSITION_ZOOM_CLASS).removeClass(TRANSITION_LOADING_CLASS);
                setTimeout(finishTransitionIn, TIMEOUT_MS);
            }

            /**
             * After the next page is loaded and the prior animation steps have completed, show the page content and finalize all transition effects
             */
            function finishTransitionIn() {
                element.removeClass(TRANSITION_NONE_CLASS).removeClass(TRANSITION_HIDDEN_CLASS  + ' ' + TRANSITION_ZOOM_CLASS);
                AppCommFactory.events.fire('uiRouter:stateChangeAnimationComplete');
            }

            /**
             * Start page-transition animation sequence, if enabled for the given route
             */
            function onStateChangeStart(event, toState, toParams, fromState) {
                if (!initialLoad && toState.data.pageTransitionEnabled && (!_.get(fromState, 'data') || fromState.data.pageTransitionEnabled)) {
                    element.addClass(TRANSITION_SHRINK_CLASS + ' ' + TRANSITION_LOADING_CLASS + ' ' + TRANSITION_HIDDEN_CLASS);

                    // wait until initial page transition out is complete, then scroll to top
                    if (toState.data.scrollToTopOnLoad) {
                        $timeout($anchorScroll, TIMEOUT_MS);
                    }

                } else if (toState.data.scrollToTopOnLoad) {
                    // no page transition, scroll to top
                    $anchorScroll();
                }
            }

            /**
             * Begin transition-in sequence into a page after navigation has occurred
             */
            function onStateChangeEnd(event, toState, toParams, fromState) {
                if (initialLoad) {
                    initialLoad = false;
                } else if (toState.data.pageTransitionEnabled && (!_.get(fromState, 'data') || fromState.data.pageTransitionEnabled)) {
                    element.addClass(TRANSITION_NONE_CLASS); // so we can instantly switch to "zoomed" size afterward
                    setTimeout(startTransitionIn, TIMEOUT_MS);
                }
            }

            var handles = [
                AppCommFactory.events.on('uiRouter:stateChangeStart', onStateChangeStart),
                AppCommFactory.events.on('uiRouter:stateChangeSuccess', onStateChangeEnd),
                AppCommFactory.events.on('uiRouter:stateChangeError', onStateChangeEnd)
            ];

            function onDestroy() {
                EventsHelperFactory.detachAll(handles);
            }

            function init() {
                EventsHelperFactory.attachAll(handles);
                scope.$on('$destroy', onDestroy);
            }

            init();
        }

        return {
            restrict: 'A',
            link: originPageTransitionLink
        };
    }

    /**
     * @ngdoc directive
     * @name originApp.directives:originPageTransition
     * @description
     *
     * Shows loader gif on page transition
     *
     */
    angular.module('origin-components')
        .directive('originPageTransition', originPageTransition);

}());
