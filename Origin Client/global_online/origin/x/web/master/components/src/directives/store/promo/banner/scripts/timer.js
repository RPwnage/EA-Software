
/** 
 * @file store/promo/banner/scripts/timer.js
 */ 
(function(){
    'use strict';

    function originStorePromoBannerTimer(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/banner/views/timer.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoBannerTimer
     * @restrict E
     * @element ANY
     * @description Promo banner for countdown timer
     */
    angular.module('origin-components')
        .directive('originStorePromoBannerTimer', originStorePromoBannerTimer);
}()); 
