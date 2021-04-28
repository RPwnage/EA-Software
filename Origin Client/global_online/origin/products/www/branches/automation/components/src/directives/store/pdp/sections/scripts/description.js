/**
 * @file store/pdp/sections/description.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-hero';

    function originStorePdpSectionsDescription(ComponentsConfigFactory, UtilFactory) {

        function originStorePdpSectionsDescriptionLink(scope, elem) {
            var titleHeight,
                productTitle = elem.find('.origin-pdp-hero-title');


            function init() {
                scope.descriptionStrings = {
                    readMoreText: UtilFactory.getLocalizedStr(scope.readMoreText, CONTEXT_NAMESPACE, 'read-more-text')
                };
            }


            scope.goToInfo = function () {
                // if requested target is a valid element...
                var reqTarget = angular.element('#description');

                if (reqTarget.length && angular.isDefined(reqTarget.offset())) {
                    // get position of requested element
                    var elemPosition = reqTarget.offset().top;

                    // determine "top" based on offset of the pdp nav
                    var offset = angular.element('.store-pdp-nav').height() || 0;

                    // determine final position to scroll to
                    var scrollPosition = elemPosition - offset;

                    // scroll to module position
                    angular.element('html, body').animate({ scrollTop: scrollPosition }, 'fast');
                }
            };

            /**
             * Set scope variable based on how many lines the product title takes up
             * so it will stay at this height when other editions are hovered over
             * in the edition selector.
             */
            function setTitleLines() {
                // Each time we update the height, disable line-clamping if set
                // to ensure correct calculation
                if (scope.titleLines) {
                    clearTitleLines();
                }
                titleHeight = productTitle[0].offsetHeight;
                var lineHeight = parseInt(productTitle.css('line-height'), 10);
                scope.titleLines = Math.round(titleHeight / lineHeight);
                scope.$digest();
            }

            /**
             * Clear scope value to disable line-clamping
             */
            function clearTitleLines() {
                scope.titleLines = null;
                scope.$digest();
            }

            /**
             * Poll every 100ms for the title to be actually present on-page
             * so its height can be evaluated so as to set the line-clamp value.
             * Some browsers (*cough*Edge*cough*) take a very long time to
             * actually render the DOM elements
             */
            function pollForTitleHeight() {
                // only run this if the element exists
                if (productTitle) {
                    if (productTitle[0].offsetHeight > 0) {
                        setTitleLines();
                    } else {
                        setTimeout(pollForTitleHeight, 100);
                    }
                }
            }

            setTimeout(pollForTitleHeight, 100);

            /**
             * Resize event handling
             */
            var resizeThrottled = UtilFactory.throttle(setTitleLines, 200);
            angular.element(window).on('resize', resizeThrottled);

            scope.$on('$destroy', function() {
                angular.element(window).off('resize', resizeThrottled);
            });


            var stopWatchingOcdDataReady = scope.$watch('ocdDataReady', function (isOcdDataReady) {
                if (isOcdDataReady) {
                    stopWatchingOcdDataReady();
                    init();
                }
            });
        }

        return {
            restrict: 'E',
            link: originStorePdpSectionsDescriptionLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/sections/views/description.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSectionsDescription
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-sections-description>
     *         </origin-store-pdp-sections-description>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpSectionsDescription', originStorePdpSectionsDescription);
}());
