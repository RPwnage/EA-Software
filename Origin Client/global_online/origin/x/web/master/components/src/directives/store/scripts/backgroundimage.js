/**
 * @file store/scripts/backgroundimage.js
 */
(function(){
    'use strict';

    function originStoreBackgroundimage(ComponentsConfigFactory, CSSGradientFactory) {
        var gradient = CSSGradientFactory.createLinearGradient;

        function originStoreBackgroundimageLink(scope) {
            if (scope.startColor && scope.endColor) {
                scope.gradientRight = gradient(scope.startColor, 0, scope.endColor, 100);
                scope.gradientLeft = gradient(scope.endColor, 0, scope.startColor, 100);
            } else {
                scope.gradientRight = scope.gradientLeft = {};
            }
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                startColor: '@',
                endColor: '@',
                image: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/backgroundimage.html'),
            link: originStoreBackgroundimageLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreBackgroundimage
     * @restrict E
     * @element ANY
     *
     * @param {string} start-color The background color to fade from
     * @param {string} end-color The image color to fade too.
     * @param {ImageUrl} image The image to use as the background image
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-backgroundimage startColor='rgba(30, 38, 44, 0)' endColor='rgba(30, 38, 44, 1)' image=''> </origin-store-backgroundimage>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreBackgroundimage', originStoreBackgroundimage);
}());
