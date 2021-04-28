
/**
 * @file store/carousel/product/scripts/pdpmediacarousel.js
 */
(function(){
    'use strict';

    function OriginStorePdpMediaCarouselCtrl($scope, PdpUrlFactory, ObjectHelperFactory, ProductFactory) {

        var takeHead = ObjectHelperFactory.takeHead;
        ProductFactory
            .filterBy(PdpUrlFactory.getCurrentEditionSelector())
            .addFilter(takeHead)
            .attachToScope($scope, 'model');
    }

    /**
    * The directive
    */
    function originStorePdpMediaCarousel(ComponentsConfigFactory) {
        return {
            replace: true,
            restrict: 'E',
            controller: 'OriginStorePdpMediaCarouselCtrl',
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/product/views/pdpmediacarousel.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreMediaCarousel
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {string} offerid the offer id of the product
     *
     *
     * @example
     *  <origin-store-pdp-media-carousel>
     *      <origin-store-media-carousel-items media="" />
     *  </origin-store-pdp-media-carousel>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpMediaCarouselCtrl', OriginStorePdpMediaCarouselCtrl)
        .directive('originStorePdpMediaCarousel', originStorePdpMediaCarousel);
}());
