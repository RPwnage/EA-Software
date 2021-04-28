/**
 * @file store/promo/scripts/calloutcta.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var LayoutMap = {
        "hero": "l-origin-store-column-hero-callout l-origin-section-divider",
        "full-width": "l-origin-store-column-full",
        "half-width": "l-origin-store-column-half"
    };
    /* jshint ignore:end */

    var HeaderSizeEnumeration = {
        "small": "small",
        "medium": "medium",
        "large": "large"
    };

    var LayoutEnumeration = {
        "hero": "hero",
        "full-width": "full-width",
        "half-width": "half-width"
    };

    var TextColorEnumeration = {
        "light" : "light",
        "dark" : "dark"
    };

    var HeaderSizeClassMap = {
        "small": "otktitle-3-condensed" ,
        "medium": "otktitle-2-condensed",
        "large": "otktitle-1-condensed"
    };
    
    var CONTEXT_NAMESPACE = 'origin-store-promo-calloutcta';
    var TEXT_CLASS_PREFIX = 'origin-store-promo-calloutcta-text-color-';

    function originStorePromoCalloutcta(ComponentsConfigFactory, NavigationFactory, UtilFactory) {
        function originStorePromoCalloutctaLink(scope, element, attrs, OriginStorePromoCalloutCtrl){
            OriginStorePromoCalloutCtrl.init(CONTEXT_NAMESPACE);
            scope.textColor = TextColorEnumeration[scope.textColor] || TextColorEnumeration.light;
            scope.textColorClass = TEXT_CLASS_PREFIX + scope.textColor;
            scope.hrefAbsoluteUrl = NavigationFactory.getAbsoluteUrl(scope.href);
            
            scope.openLink = function($event, href){
                $event.preventDefault();
                $event.stopPropagation();
                NavigationFactory.openLink(href);
            };

            var heroLayout = scope.layout === LayoutEnumeration.hero;
            scope.shellHref = scope.ctaText && scope.heroLayout ? '' : scope.href;
            scope.shouldShowCta = heroLayout && scope.ctaText;
            scope.isExternaLink = UtilFactory.isAbsoluteUrl(scope.href);
            // Set header styles for main title
            var headerSize = scope.headerSize || HeaderSizeEnumeration.medium;
            scope.headerSizeClass = _.get(HeaderSizeClassMap, headerSize);
        }
        
        return {
            restrict: 'E',
            scope: {
                layout: '@',
                image : '@',
                titleStr : '@',
                headerSize: '@',
                href: '@',
                subtitleStr: '@',
                backgroundColor: '@',
                useH1Tag: '@',
                ctaText: '@',
                textColor: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/calloutcta.html'),
            controller: 'OriginStorePromoCalloutCtrl',
            link: originStorePromoCalloutctaLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoCalloutcta
     * @restrict E
     * @element ANY
     * @description Merchandizes a promo module with callout and a CTA
     * @param {LayoutEnumeration} layout The type of layout this promo will have.
     * @param {ImageUrl} image The background image for this module.
     * @param {LocalizedString} title-str Promo title
     * @param {HeaderSizeEnumeration} header-size The header size. The following are char limits per header size: Small {full-width/hero: DO NOT USE, half-width: 12 char). Medium {full-width/hero: 18 char, half-width: 9 char}. Large { full-width/hero: 13 char, half-width: DO NOT USE}
     * @param {LocalizedString} subtitle-str Promo subtitle
     * @param {string} background-color Hex value of the background color
     * @param {BooleanEnumeration} use-h1-tag use H1 Tag for the title to boost SEO relevance. Only one H1 should be present on a page. Optional and defaults to H2
     * @param {LocalizedString} cta-text CTA text. CTA will not show if this is absent
     * @param {TextColorEnumeration} text-color light or dark to match background image. Default to light
     * @param {Url} href cta href
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *      <origin-store-promo-callout-cta
     *        layout="hero"
     *        image="https://docs.x.origin.com/proto/images/staticpromo/staticpromo_titanfall_@1x.jpg"
     *        title-str="Save Big On All Things Titanfall"
     *        href="app.store.wrapper.bundle-blackfriday"
     *        subtitle-str=""
     *        text=""
     *        callout-text=""
     *        >
     *      </origin-store-promo-callout-cta>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePromoCalloutcta', originStorePromoCalloutcta);
}());
