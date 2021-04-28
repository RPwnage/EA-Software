
/** 
 * @file store/promo/banner/scripts/subtitle.js
 */ 
(function(){
    'use strict';

    function originStorePromoBannerSubtitle(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/banner/views/subtitle.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoBannerSubtitle
     * @restrict E
     * @element ANY
     * @description Used to create the 'subtitle' promo banner with a title, subtitle, optional release date and call to action 
     * This uses a non isolated scope so it needs to have title, text (and optionally start-date/end-date/available-text/cta-text) on its parents scope.
     * 
     *
     * @example
     * <example module="origin-components">
     *      <origin-store-promo-banner-subtitle></origin-store-promo-banner-subtitle>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePromoBannerSubtitle', originStorePromoBannerSubtitle);
}()); 
