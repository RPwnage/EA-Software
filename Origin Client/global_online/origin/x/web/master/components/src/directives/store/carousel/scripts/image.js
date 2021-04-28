
/**
 * @file store/carousel/scripts/image.js
 */
(function(){
    'use strict';
    var DIRECTIVE_NAME = 'origin-store-carousel-image';
    var VersionEnumeration = {'REGULAR': 'REGULAR', 'MODAL': 'MODAL'};
    var RATIO = 1.72;

    /**
    * The directive
    */
    function OriginStoreCarouselImageCtrl($scope, UtilFactory, DialogFactory) {

       $scope.order = [];
        var jsonData = {
            name: DIRECTIVE_NAME,
            data:{title: $scope.title, version: VersionEnumeration.MODAL },
            id: 'web-carousel-modal',
            size: 'xLarge',
            contentDirective:[]
        };

        $scope.$on('imageCarousel:registar', function(event, data){
            jsonData.contentDirective.push(data.json);
            $scope.order.push(data.id);
        });

        $scope.$on('imageCarousel:openModal', function(event, data) {
            jsonData.data.position = $scope.order.indexOf(data.id);
            DialogFactory.openPaginated(jsonData);
        });

        $scope.$on("$destroy", function() {
            // Here you are going to have to remove the resize event.
        });

    }


    function originStoreCarouselImage(ComponentsConfigFactory, $timeout) {


        function originStoreCarouselImageLink(scope, elem/*, attrs, ctrl*/) {

            function sizeElements() {
                $timeout(function(){
                    if(scope.version === VersionEnumeration.MODAL) {
                        var modalWidth = $('.otkmodal-content').width();

                        var w = modalWidth - 60; // 30 pixels on each side
                        var h = modalWidth / RATIO;

                        elem.find('.origin-carouselimageitem-modal').each(function(index, item) {
                            $(item).width(w).height(h);
                        });
                        elem.find('.origin-carouselvideoitem-modal').each(function(index, item) {
                            $(item).width(w).height(h);
                        });
                        elem.find('iframe').each(function(index, item) {
                            $(item).width(w).height(h);
                        });
                    }
                });
            }

            if(scope.version === VersionEnumeration.MODAL) {
                sizeElements();
            }

            if(scope.version === VersionEnumeration.MODAL) {
                console.log('Bound and event');
                $(window).resize(sizeElements);
            }
        }

        return {
            restrict: 'E',
            // /replace: true,
            link: originStoreCarouselImageLink,
            transclude: true,
            controller: 'OriginStoreCarouselImageCtrl',
            scope: {
                title: '@',
                version: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/views/image.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselImage
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {LocalizedString} title - How the carousel text should be aligned affects every slide
     * @param {LocalizedString} view-all-str - The number of milliseconds to sit on a slide without changing to the next slide
     * @param {string} view-all-href - boolean as to if auto rotation should be on or off.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-image title="Image/Video Slider"></origin-store-carousel-image>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreCarouselImageCtrl', OriginStoreCarouselImageCtrl)
        .directive('originStoreCarouselImage', originStoreCarouselImage);
}());
