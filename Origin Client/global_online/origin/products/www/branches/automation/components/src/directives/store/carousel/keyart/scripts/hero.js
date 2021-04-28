
/** 
 * @file store/carousel/keyart/scripts/hero.js
 */ 
(function(){
    'use strict';

    var shadeEnumeration = {
        "light": "#ffffff",
        "dark": "#1e262c"
    };

    var CONTEXT_NAMESPACE = 'origin-store-carousel-keyart-hero';

    function OriginStoreCarouselKeyartHeroCtrl($scope, NavigationFactory, DirectiveScope, PriceInsertionFactory) {
        function handleScope() {
            $scope.shade = $scope.shade || shadeEnumeration.light;
            $scope.ctaShade = $scope.shade === shadeEnumeration.light ? 'light' : 'dark';
            $scope.strings = {};

            PriceInsertionFactory
                .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.header, CONTEXT_NAMESPACE, 'header');

            PriceInsertionFactory
                .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.subtitle, CONTEXT_NAMESPACE, 'subtitle');

            $scope.gotoUrl = function() {
                NavigationFactory.openLink($scope.ctaHref);
            };
        }

        DirectiveScope
            .populateScope($scope, CONTEXT_NAMESPACE)
            .then(handleScope);
    }

    function originStoreCarouselKeyartHero(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                header: '@',
                subtitle: '@',
                background: '@',
                logo: '@',
                ctaText: '@',
                ctaHref: '@',
                shade: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/keyart/views/hero.html'),
            controller: OriginStoreCarouselKeyartHeroCtrl
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselKeyartHero
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedTemplateString} header The header text
     * @param {LocalizedTemplateString} subtitle the subtitle text
     * @param {ImageUrl} background the background image
     * @param {ImageUrl} logo the logo image
     * @param {LocalizedString} cta-text the CTA text
     * @param {String} cta-href the CTA URL
     * @param {shadeEnumeration} shade the text shade
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-keyart />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselKeyartHero', originStoreCarouselKeyartHero);
}()); 
