/**
 * @file store/pdp/feature/scripts/accoladehero.js
 */
(function(){
    'use strict';

    function originStorePdpFeatureAccoladeHero(ComponentsConfigFactory) {

        function originStorePdpFeatureAccoladeHeroLink(scope, element, attributes, originStorePdpSectionWrapperCtrl) {
            if (scope.backgroundImage && scope.quote1Str) {
                originStorePdpSectionWrapperCtrl.setVisibility(true);
            }
        }

        return {
            restrict: 'E',
            require: '^originStorePdpSectionWrapper',
            scope: {
                quote1Str: '@',
                quote1SourceStr: '@',
                quote2Str: '@',
                quote2SourceStr: '@',
                quote3Str: '@',
                quote3SourceStr: '@',
                backgroundImage: '@'
            },
            link: originStorePdpFeatureAccoladeHeroLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/feature/views/accoladehero.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureAccoladeHero
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} quote1-str Promo quote 1 - should be wrapped in quotes - use \&#8220; text \&#8221;
     * @param {LocalizedString} quote1-source-str Promo quote 1 source
     * @param {LocalizedString} quote2-str Promo quote 2 - not visible on small screens - should be wrapped in quotes - use  \&#8220; text \&#8221;
     * @param {LocalizedString} quote2-source-str Promo quote 2 source - not visible on small screens
     * @param {LocalizedString} quote3-str Promo quote 3 - not visible on small screens - should be wrapped in quotes - use  \&#8220; text \&#8221;
     * @param {LocalizedString} quote3-source-str Promo quote 3 source - not visible on small screens
     * @param {ImageUrl} background-image background image URL
     *
     *
     * Large PDP feature image with parallax effect
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-feature-accolade-hero
     *             image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/dai_intro_bg.png"
     *         </origin-store-pdp-feature-accolade-hero>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureAccoladeHero', originStorePdpFeatureAccoladeHero);
}());
