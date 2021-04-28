
/**
 * @file store/carousel/scripts/featured.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-carousel-featured';

    var TextColorEnumeration = {
        "light": "light",
        "dark": "dark"
    };

    var CtaType = {
        "transparent": "transparent",
        "light": "light"
    };
    var TEXT_CLASS_PREFIX = 'origin-store-carousel-featured-text-color-';
    /**
    * The directive
    */
    function originStoreCarouselFeatured(ComponentsConfigFactory, CSSUtilFactory, PriceInsertionFactory) {

        function OriginStoreCarouselFeaturedLink(scope, element) {
            scope.backgroundColor = CSSUtilFactory.setBackgroundColorOfElement(scope.backgroundColor, element, CONTEXT_NAMESPACE);
            scope.strings = {};
            PriceInsertionFactory
                .insertPriceIntoLocalizedStr(scope, scope.strings, scope.titleStr, CONTEXT_NAMESPACE, 'title-str');
            PriceInsertionFactory
                .insertPriceIntoLocalizedStr(scope, scope.strings, scope.text, CONTEXT_NAMESPACE, 'text');
            scope.textColor = TextColorEnumeration[scope.textColor] || TextColorEnumeration.light;
            scope.textColorClass = TEXT_CLASS_PREFIX + scope.textColor;
            scope.ctaType = (scope.textColor === TextColorEnumeration.light) ? CtaType.transparent : CtaType.light;
        }

        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {
                titleStr: '@',
                text: '@',
                description: '@',
                href: '@',
                imageSrc: '@',
                backgroundColor: '@',
                textColor: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/views/featured.html'),
            link: OriginStoreCarouselFeaturedLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselFeatured
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {LocalizedTemplateString} title-str The title of the module
     * @param {LocalizedTemplateString} text Descriptive text describing the products
     * @param {LocalizedString} description The text in the call to action
     * @param {Url} href the The destination of the cta
     * @param {ImageUrl} image-src The sorce for the image
     * @param {string} background-color Hex value of the background color
     * @param {TextColorEnumeration} text-color The text/font color of the component
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-featured />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselFeatured', originStoreCarouselFeatured);
}());
