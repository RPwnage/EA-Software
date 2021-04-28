
/**
 * @file dialog/content/scripts/mediacarousel.js
 */
(function(){
    'use strict';

    function originDialogContentMediaCarousel(ComponentsConfigFactory, ObjectHelperFactory) {


        function originDialogContentMediaCarouselLink(scope) {
            var length = ObjectHelperFactory.length;

            scope.previousMediaItem = function() {
                var activeSlide = parseInt(scope.active, 10);

                if(activeSlide === 0) {
                    scope.active = length(scope.items) - 1;
                } else {
                    scope.active = activeSlide - 1;
                }

                scope.$broadcast('dialogContent:mediaCarousel:mediaChanged');

            };

            scope.nextMediaItem = function() {
                var activeSlide = parseInt(scope.active, 10);

                if(activeSlide === length(scope.items) - 1) {
                    scope.active = 0;
                } else {
                    scope.active = activeSlide + 1;
                }

                scope.$broadcast('dialogContent:mediaCarousel:mediaChanged');
            };
        }

        return {
            restrict: 'E',
            scope: {
                items: '=',
                active: '='
            },
            link: originDialogContentMediaCarouselLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/mediacarousel.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentMediaCarousel
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {string} images An array of MediaCarousel
     * @param {string} active The index of the active screenshot
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-dialog-content-media-carousel />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originDialogContentMediaCarousel', originDialogContentMediaCarousel);
}());
