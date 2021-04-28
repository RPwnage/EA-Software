/**
 * @file store/about/scripts/hero.js
 */

(function(){
    'use strict';

    function originStoreAboutHero(ComponentsConfigFactory) {

        function originStoreAboutHeroLink(scope, element) {

            setTimeout(function() {
                element.find('.origin-about-hero-header').addClass('origin-about-hero-delayedslide');
            }, 100);

        }

        return {
            restrict: 'E',
            scope: {
                titleStr: '@'
            },
            transclude: true,
            link: originStoreAboutHeroLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/hero.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutHero
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str (optional) title override
     * @description
     *
     * PDP hero (header container)
     *
     * @example
     * <origin-store-row>
     *     <origin-store-about-hero background-image="https://docs.x.origin.com/proto/images/keyart/battlefront.jpg"></origin-store-about-hero>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStoreAboutHero', originStoreAboutHero);
}());
