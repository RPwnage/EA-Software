
/**
 * @file store/carousel/media/scripts/videoitem.js
 */
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStoreMediaCarouselVideoitem(YoutubeFactory, ComponentsConfigFactory) {

        /**
        * The directive link
        */
        function originStoreMediaCarouselVideoitemLink(scope/*, elem, attrs, ctrl*/) {
            scope.src =  YoutubeFactory.getThumbnailImage(scope.videoId);
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                videoId: '@',
                restrictedAge: '@',
                position: '=',
                onClick: '&'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/media/views/videoitem.html'),
            link: originStoreMediaCarouselVideoitemLink
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
     * @param {Video} video-id The youTube video id
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-media-carousel-videoitem videoId="sdfas"> </origin-store-media-carousel-videoitem>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreMediaCarouselVideoitem', originStoreMediaCarouselVideoitem);
}());
