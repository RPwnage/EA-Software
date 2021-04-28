/**
 * @file parallax-container.js
 */
(function(){
    'use strict';

    function originParallaxContainer(ComponentsConfigFactory, $window, CSSUtilFactory) {

        function originParallaxContainerLink(scope, element) {
            var parallaxContainer = element.find('.origin-parallax-container'),
                currentScrollY = 0,
                animating = false;

            // Alias for the browser-specific version of requestAnimationFrame
            var requestAnimationFrame = $window.requestAnimationFrame ||
               $window.mozRequestAnimationFrame ||
               $window.msRequestAnimationFrame ||
               $window.webkitRequestAnimationFrame;

            /**
             * Executes the callback when the next animation frame is available
             * @return {void}
             */
            function requestAnimation(callback) {
                if (animating) {
                    return;
                }

                animating = true;

                requestAnimationFrame(function () {
                    callback();
                    animating = false;
                });
            }

            /**
             * Renders parallax effect
             * @return {void}
             */
            function updatePosition() {
                var top = element.offset().top - scope.offset,
                    parallaxPosition = (currentScrollY - top) / scope.depth,
                    transition = 'translate(0,' + parallaxPosition + 'px)';
                parallaxContainer.css(CSSUtilFactory.addVendorPrefixes('transform', transition));

            }

            /**
             * Scroll event handler
             * @return {void}
             */
            function onScroll() {
                currentScrollY = window.pageYOffset;
                requestAnimation(updatePosition);
            }

            window.addEventListener('scroll', onScroll, false);
        }

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                image: '@',
                depth: '@',
                offset: '@'

            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/parallax-container.html'),
            link: originParallaxContainerLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originParallaxContainer
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {float} depth parallax depth (ratio)
     * @param {integer} offset difference between real and visible top side of the image
     *
     * Background image with the parallax effect on scrolling
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-parallax-container
     *             image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/dai_intro_bg.png"
     *         </origin-parallax-container>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originParallaxContainer', originParallaxContainer);
}());
