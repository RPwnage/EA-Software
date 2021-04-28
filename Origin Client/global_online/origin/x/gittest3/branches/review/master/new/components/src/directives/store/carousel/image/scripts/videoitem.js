
/**
 * @file store/carousel/image/scripts/videoitem.js
 */
(function(){
    'use strict';
    var VIDEO_THUMBNAIL = "https://img.youtube.com/vi/%videoId%/0.jpg";

    /**
    * The directive
    */
    function originStoreCarouselImageVideoitem(ComponentsConfigFactory) {
        /**
        * The directive link
        */
        function originStoreCarouselImageVideoitemLink(scope/*, elem, attrs, ctrl*/) {
            scope.imageURL =  VIDEO_THUMBNAIL.replace("%videoId%", scope.videoId);
        }
        return {
            restrict: 'E',
            scope: {
                videoId: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/image/views/videoitem.html'),
            link: originStoreCarouselImageVideoitemLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselImageVideoitem
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {string} videoId The youTube video id
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-image-videoitem videoId="sdfas"> </origin-store-carousel-image-videoitem>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselImageVideoitem', originStoreCarouselImageVideoitem);
}());
