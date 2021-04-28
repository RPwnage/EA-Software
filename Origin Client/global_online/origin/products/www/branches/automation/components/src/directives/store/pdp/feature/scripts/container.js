
/**
 * @file store/pdp/feature/scripts/container.js
 */
(function(){
    'use strict';

    /**
      * A list of text colors
      * @readonly
      * @enum {string}
      */
    var TextColorsEnumeration = {
        "dark": "dark",
        "light": "light"
    };

    var PARENT_CONTEXT_NAMESPACE = 'origin-store-pdp-feature-container-wrapper',
        CONTEXT_NAMESPACE = 'origin-store-pdp-feature-container',

        BUBBLE_ELEM_SELECTOR = '.origin-pdp-feature-bubble',

        BUBBLE_SLIDEDOWN_CLASS = 'origin-pdp-feature-slideDown',
        BUBBLE_SLIDEUP_CLASS = 'origin-pdp-feature-slideUp',
        BUBBLE_NOANIMATE_CLASS = 'origin-pdp-feature-noanimation',

        BUBBLE_ANIMATE_DELAY_MS = 100,
        RESIZE_THROTTLE_MS = 200;

    function originStorePdpFeatureContainerCtrl($scope) {

        $scope.linkFunctionReady = false;
        var stopWatchingLinkFunctionReady;


        function onReady(newValue) {
            if (newValue) {
                if (stopWatchingLinkFunctionReady) {
                    stopWatchingLinkFunctionReady();
                }
                $scope.setWrapperVisibility();
            }
        }

        this.registerItem = function(){
            if (!$scope.linkFunctionReady) {
                stopWatchingLinkFunctionReady = $scope.$watch('linkFunctionReady', onReady);
            } else {
                $scope.setWrapperVisibility();
            }
        };
    }

    function originStorePdpFeatureContainer(ComponentsConfigFactory, DirectiveScope, $timeout, CSSUtilFactory, ScreenFactory, AnimateFactory) {

        function originStorePdpFeatureContainerLink(scope, element, attrs, originStorePdpSectionWrapperCtrl) {

            var featureElems,
                isAnimationEnabled,
                isScrollEventBound = false,
                isSmallScreen,
                timers = [];
            scope.anyOcdDataFound = false;
            scope.linkFunctionReady = true;

            function setBubbleVisible(elem) {
                return function() {
                    elem.addClass(BUBBLE_SLIDEUP_CLASS);
                };
            }

            /**
             * Check position of feature-bubble relative to scroll position and show/hide the bubble as needed
             */
            function checkBubbles() {
                if (!isSmallScreen) {
                    _.forEach(featureElems, function (featureElem, i) {
                        var currElem = angular.element(featureElem);
                        if (ScreenFactory.isOnOrAboveScreen(currElem)) {
                            trackTimer($timeout(setBubbleVisible(currElem), i * BUBBLE_ANIMATE_DELAY_MS, false));
                        } else {
                            currElem.removeClass(BUBBLE_SLIDEUP_CLASS);
                        }
                    });
                }
            }

            /**
             * Disables the animation and displays the bubble right away.
             */
            function disableBubbleAnimation() {
                featureElems
                    .removeClass(BUBBLE_SLIDEDOWN_CLASS)
                    .addClass(BUBBLE_NOANIMATE_CLASS);

                isAnimationEnabled = false;
            }

            /**
             * Update classes to enable scroll-position-based showing of bubble, and bind scroll event if needed
             */
            function enableBubbleAnimation() {
                if (!isAnimationEnabled) {
                    featureElems
                        .removeClass(BUBBLE_NOANIMATE_CLASS)
                        .addClass(BUBBLE_SLIDEDOWN_CLASS);

                    // Re-check bubbles in case user was already scrolled to the feature bubbles.
                    // Wait short delay for CSS transition anims to finish as they affect layout on this component
                    trackTimer($timeout(checkBubbles, BUBBLE_ANIMATE_DELAY_MS, false));

                    // Only bind event if it wasn't already bound and we actually have some elements to animate
                    if (!isScrollEventBound && scope.anyOcdDataFound) {
                        bindScrollEventHandler();
                    }
                }

                isAnimationEnabled = true;
            }

            function bindScrollEventHandler() {
                AnimateFactory.addScrollEventHandler(scope, checkBubbles);
                isScrollEventBound = true;
            }

            function checkScreenSizeAndUpdate() {
                isSmallScreen = ScreenFactory.isXSmall();

                if (!isSmallScreen) {
                    enableBubbleAnimation();
                } else {
                    disableBubbleAnimation();
                }
            }

            /**
             * Add timer Promise to internal array so it can be canceled if/when scope is destroyed
             * @param timerPromise Promise returned from $timeout service
             */
            function trackTimer(timerPromise){
                timers.push(timerPromise);
                timerPromise.then(forgetTimer(timerPromise));
            }

            /**
             * Remove $timeout reference from timers
             * @param timerPromise Promise returned from $timeout service
             */
            function forgetTimer(timerPromise) {
                _.remove(timers, timerPromise);
            }

            /**
             * Iterate over timers array and clear them all
             */
            function cancelTimers() {
                if (timers.length) {
                    _.forEach(timers, function(timerPromise) {
                        $timeout.cancel(timerPromise);
                    });
                }
            }

            /**
             * Init feature container and enable animation scroll event if screen is larger than mobile-size
             */
            function init(anyDataFound) {
                featureElems = element.find(BUBBLE_ELEM_SELECTOR);
                scope.anyOcdDataFound = anyDataFound;
                scope.moduleBackgroundColor = CSSUtilFactory.createBackgroundColor(scope.backgroundColor);
                scope.moduleAccentColor = CSSUtilFactory.createBackgroundColor(scope.accentColor);
                scope.isDarkColor = (scope.textColor === TextColorsEnumeration.dark);

                if (anyDataFound) {
                    checkScreenSizeAndUpdate();

                    // Set a resize handler to enable/disable the animation as needed if the user resizes their screen
                    AnimateFactory.addResizeEventHandler(scope, checkScreenSizeAndUpdate, RESIZE_THROTTLE_MS);
                }
            }

            scope.setWrapperVisibility = function(){
                if (originStorePdpSectionWrapperCtrl && _.isFunction(originStorePdpSectionWrapperCtrl.setVisibility)){
                    originStorePdpSectionWrapperCtrl.setVisibility(true);
                }
            };

            DirectiveScope.populateScope(scope, [PARENT_CONTEXT_NAMESPACE, CONTEXT_NAMESPACE]).then(init);

            scope.$on('$destroy', cancelTimers);
        }

        return {
            require: '^?originStorePdpSectionWrapper',
            restrict: 'E',
            replace: true,
            scope: {
                titleStr: '@',
                text: '@',
                textColor: '@',
                backgroundColor: '@',
                accentColor: '@'
            },
            transclude: true,
            link: originStorePdpFeatureContainerLink,
            controller: originStorePdpFeatureContainerCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/feature/views/container.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureContainer
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str (optional) key feature container title copy
     * @param {LocalizedString} text (optional) key feature container text copy
     * @param {TextColorsEnumeration} text-color desc 
     * @param {string} background-color module background color
     * @param {string} accent-color module accent color (used for divider line)
     * @description
     *
     * Container to hold the key feature items (bubbles)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-feature-container title-str="Five Reasons to Play" text="Explore a vast range of experiences that allow you to play your way." />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureContainer', originStorePdpFeatureContainer);
}());
