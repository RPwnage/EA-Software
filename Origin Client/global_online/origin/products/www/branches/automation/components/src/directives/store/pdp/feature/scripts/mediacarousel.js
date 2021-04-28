/**
 * @file store/pdp/sections/scripts/mediacarousel.js
 */
(function () {
    'use strict';
    var PDP_SECTION_WRAPPER_TITLE_CLASS = 'origin-store-pdp-carousel-header';
    /**
     * The controller
     */
    function OriginStorePdpMediaCarouselCtrl($scope, DialogFactory) {

        var media = [];
        $scope.linkFunctionReady = false;
        var stopWatchingLinkFunctionReady;

        this.openModal = function (position) {
            DialogFactory.openDirective({
                id: 'web-media-overlay',
                name: 'origin-dialog-content-media-carousel',
                size: 'xLarge',
                data: {
                    id: 'web-media-overlay',
                    class: 'origin-tealium-web-media-overlay ',
                    items: media,
                    active: position,
                    matureContent: media[position].matureContent
                }
            });
        };


        function onReady(newValue) {
            if (newValue) {
                if (stopWatchingLinkFunctionReady) {
                    stopWatchingLinkFunctionReady();
                }
                $scope.setWrapperVisibility();
            }
        }

        /**
         * Register a media object
         * @param mediaObject
         * @returns {number} index in the array.
         */
        this.register = function (mediaObject) {
            if (!$scope.linkFunctionReady) {
                stopWatchingLinkFunctionReady = $scope.$watch('linkFunctionReady', onReady);
            } else {
                $scope.setWrapperVisibility();
            }
            media.push(mediaObject);
            return media.length - 1;
        };
    }

    /**
     * The directive
     */
    function originStorePdpMediaCarousel(ComponentsConfigFactory) {

        function originStorePdpMediaCarouselLink(scope, element, attributes, originStorePdpSectionWrapperCtrl) {

            scope.setWrapperVisibility = function () {
                if (originStorePdpSectionWrapperCtrl && _.isFunction(originStorePdpSectionWrapperCtrl.setVisibility) && _.isFunction(originStorePdpSectionWrapperCtrl.setHeaderClass)) {
                    originStorePdpSectionWrapperCtrl.setVisibility(true);
                    originStorePdpSectionWrapperCtrl.setHeaderClass(PDP_SECTION_WRAPPER_TITLE_CLASS);
                }
            };

            scope.linkFunctionReady = true;
        }

        return {
            require: '^?originStorePdpSectionWrapper',
            restrict: 'E',
            link: originStorePdpMediaCarouselLink,
            controller: OriginStorePdpMediaCarouselCtrl,
            scope: {},
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/announcement/sections/views/mediacarousel.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpMediaCarousel
     * @restrict E
     * @element ANY
     * @scope
     * @description Carousel that supports multiple images and videos
     *
     * @example
     *  <origin-store-pdp-media-carousel>
     *
     *  </origin-store-pdp-media-carousel>
     */
    angular.module('origin-components')
        .directive('originStorePdpMediaCarousel', originStorePdpMediaCarousel);
}());

