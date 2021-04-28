
/** 
 * @file store/promo/scripts/cta.js
 */ 
(function(){
    'use strict';
    function originStorePromoCta(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/cta.html'),
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoCta
     * @restrict E
     * @element ANY
     * @description Makes a CTA link/button that is intended to live inside a 
     * promo banner. This CTA is an inline link at < 'large desktop size' and turns into a 
     * cta-secondary transparent button aligned right at > 'large desktop size'. 
     * @param {Url} href Url link
     * @param {LocalizedString} text Text for the link
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *       <h3 class="origin-store-promo-banner-subtitle otktitle-3">{{ ::text }}<origin-store-promo-cta></origin-store-promo-cta></h3>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePromoCta', originStorePromoCta);
}()); 
