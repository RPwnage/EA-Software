
/**
 * @file store/carousel/image/scripts/imageitem.js
 */
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStoreCarouselImageImageitem(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                imageUrl: "@"
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/image/views/imageitem.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselImageImageitem
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {Url} imageUrl The url for the image in the carousel.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-image-imageitem />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselImageImageitem', originStoreCarouselImageImageitem);
}());
