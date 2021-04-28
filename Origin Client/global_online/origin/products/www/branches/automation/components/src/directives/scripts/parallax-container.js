/**
 * @file parallax-container.js
 */
(function () {
    'use strict';


    function originParallaxContainer(ComponentsConfigFactory, CSSUtilFactory, ScreenFactory, AnimateFactory) {

        function originParallaxContainerLink(scope, element) {
            var parallaxContainer = element,
                currentScrollY = 0,
                isParallaxEnabled = true,
                referenceFrameDistance = 1;

            /**
             * Renders parallax effect
             * @return {void}
             */
            function updatePosition() {
                currentScrollY = window.pageYOffset - element.offset().top;
                isParallaxEnabled = !ScreenFactory.isXSmall();

                if (isParallaxEnabled && window.pageYOffset !== 0) {
                    //The relative ratio between foreground content and parallax image
                    //Describes the parallax image movement speed relative to current focal plane
                    var depthRatio = 1 - (referenceFrameDistance / scope.depth),
                        parallaxPosition = (depthRatio * currentScrollY).toFixed(2),
                        transition = 'translate3d(0,' + Math.max(parallaxPosition, 0) + 'px, 0)';

                    parallaxContainer.css(CSSUtilFactory.addVendorPrefixes('transform', transition));
                } else {
                    parallaxContainer.removeAttr('style');
                }
            }
            function init(){
                AnimateFactory.addScrollEventHandler(scope, updatePosition);
                setTimeout(function(){ //Making sure image is rendered
                    updatePosition();
                }, 0);
            }
            init();
        }

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                depth: '@',
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
     * @param {Number} depth distance of the image relative to foreground content
     * @param {Number} offset difference between real and visible top side of the image
     *
     * Background image with the parallax effect on scrolling
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-parallax-container offset="" depth="3">
     *
     *         </origin-parallax-container>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originParallaxContainer', originParallaxContainer);
}());
