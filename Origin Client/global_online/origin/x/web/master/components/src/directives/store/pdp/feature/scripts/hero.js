/**
 * @file store/pdp/feature/hero.js
 */
(function(){
    'use strict';

    function originStorePdpFeatureHero(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/feature/views/hero.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureHero
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {ImageUrl} image background image URL
     *
     * Large PDP feature image with parallax effect
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-feature-hero
     *             image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/dai_intro_bg.png"
     *         </origin-store-pdp-feature-hero>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureHero', originStorePdpFeatureHero);
}());
