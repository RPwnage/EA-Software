/**
 * @file store/pdp/feature/scripts/badgehero.js
 */
(function(){
    'use strict';

    function originStorePdpFeatureBadgeHero(ComponentsConfigFactory) {

        function originStorePdpFeatureBadgeHeroLink(scope, element, attributes, originStorePdpSectionWrapperCtrl) {
            if (scope.backgroundImage && scope.quoteStr && scope.badgeImage) {
                originStorePdpSectionWrapperCtrl.setVisibility(true);
            }
        }

        return {
            restrict: 'E',
            require: '^originStorePdpSectionWrapper',
            scope: {
                quoteStr: '@',
                badgeImage: '@',
                backgroundImage: '@'
            },
            link: originStorePdpFeatureBadgeHeroLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/feature/views/badgehero.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureBadgeHero
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} quote-str quote text
     * @param {ImageUrl} background-image background image URL
     * @param {ImageUrl} badge-image badge image URL
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-feature-badge-hero
     *             quote-str="Great game"
     *             badge-image="some image.png"
     *             background-image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/dai_intro_bg.png"
     *         </origin-store-pdp-feature-badge-hero>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureBadgeHero', originStorePdpFeatureBadgeHero);
}());
