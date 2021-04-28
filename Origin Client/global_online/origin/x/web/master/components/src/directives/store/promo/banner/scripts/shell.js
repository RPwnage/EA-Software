
/** 
 * @file store/promo/banner/scripts/shell.js
 */ 
(function(){
    'use strict';

    function originStorePromoBannerShell(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/banner/views/shell.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoBannerShell
     * @restrict E
     * @element ANY
     * @description Common functionality for all promo banners, currently only includes pack art, but should be the place to put future elements common to all banners.
     * Transcludes banner content (subtitle, countdown clock, game details etc.)
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-promo-banner-shell />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePromoBannerShell', originStorePromoBannerShell);
}()); 
