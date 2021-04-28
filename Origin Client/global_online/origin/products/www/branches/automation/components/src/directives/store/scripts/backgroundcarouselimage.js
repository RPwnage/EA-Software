/**
 * @file store/promo/scripts/backgroundcarouselimage.js
 */
(function(){
    'use strict';
    function originStoreBackgroundcarouselimage() {
        function originStoreBackgroundcarouselimageLink (scope, element, attributes, originStoreBackgroundimagecarouselCtrl) {
            if (originStoreBackgroundimagecarouselCtrl && _.isFunction(originStoreBackgroundimagecarouselCtrl.registerBackgroundImage)){
                originStoreBackgroundimagecarouselCtrl.registerBackgroundImage(scope.imageUrl);
            }
        }

        return {
            restrict: 'E',
            require: '^?originStoreBackgroundimagecarousel',
            scope: {
                imageUrl: '@'
            },
            link: originStoreBackgroundcarouselimageLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreBackgroundcarouselimage
     * @restrict E
     * @element ANY
     * @scope
     *
     * @param {ImageUrl} image-url image
     *
     * @description Merchandise an image
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-backgroundcarouselimage>
     *
     *     </origin-store-backgroundcarouselimage>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreBackgroundcarouselimage', originStoreBackgroundcarouselimage);
}());
