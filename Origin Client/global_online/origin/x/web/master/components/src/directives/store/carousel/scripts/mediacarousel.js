
/**
 * @file store/carousel/scripts/mediacarousel.js
 */
(function(){
    'use strict';

    /**
     * The controller
     */
    function OriginStoreMediaCarouselCtrl($scope, DialogFactory) {
        $scope.isLoading = true;

        var stopWatchingMedia = $scope.$watch('media', function(newValue) {
            if (angular.isDefined(newValue)) {
                $scope.isLoading = false;
                stopWatchingMedia();
            }
        });

        $scope.openModal = function(position) {
            DialogFactory.openDirective({
                id: 'web-media-overlay',
                name: 'origin-dialog-content-media-carousel',
                size: 'xLarge',
                data: {
                    items: $scope.media,
                    active: position,
                    restrictedAge: $scope.restrictedAge
                }
            });
        };
    }

    /**
     * The directive
     */
    function originStoreMediaCarousel(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginStoreMediaCarouselCtrl',
            scope: {
                media: '='
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/views/mediacarousel.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreMediaCarousel
     * @restrict E
     * @element ANY
     * @scope
     * @description Carousel that supports multiple images and videos
     * @param {Array} media - list of media (image or video) to be displayed in carousel.
     *
     * @example
     *  <origin-store-pdp-media-carousel>
     *      <origin-store-media-carousel media="" />
     *  </origin-store-pdp-media-carousel>
     */
    angular.module('origin-components')
        .controller('OriginStoreMediaCarouselCtrl', OriginStoreMediaCarouselCtrl)
        .directive('originStoreMediaCarousel', originStoreMediaCarousel);
}());
