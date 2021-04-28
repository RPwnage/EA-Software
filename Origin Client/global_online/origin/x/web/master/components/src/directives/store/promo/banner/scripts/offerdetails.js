
/** 
 * @file store/promo/banner/scripts/offerdetails.js
 */ 
(function(){
    'use strict';

    function originStorePromoBannerOfferdetails(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/banner/views/offerdetails.html'),
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoBannerOfferdetails
     * @restrict E
     * @element ANY
     * @description Promo banner for offer details
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-promo-banner-offerdetails />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePromoBannerOfferdetails', originStorePromoBannerOfferdetails);
}()); 
