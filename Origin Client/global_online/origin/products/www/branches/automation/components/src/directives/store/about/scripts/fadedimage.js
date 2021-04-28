
/** 
 * @file store/about/scripts/fadedimage.js
 */ 
(function(){
    'use strict';
    function originStoreAboutFadedimage(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/fadedimage.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutFadedimage
     * @restrict E
     * @element ANY
     * @scope
     * @description Fades an image into the background. Fades from the left horizontally at sizes >1000px. Fades from the bottom <1000px 
     * @param {ImageUrl} image The image to be faded
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-about-fadedimage />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAboutFadedimage', originStoreAboutFadedimage);
}()); 
