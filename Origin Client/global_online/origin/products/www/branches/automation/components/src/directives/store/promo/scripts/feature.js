/**
 * @file store/promo/scripts/feature.js
 */
(function(){
    'use strict';
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var LayoutEnumeration = {
        "hero": "hero",
        "full-width": "full-width",
        "half-width": "half-width"
    };

    var LayoutMap = {
        "hero": "l-origin-store-column-hero l-origin-section-divider",
        "full-width": "l-origin-store-column-full",
        "half-width": "l-origin-store-column-half"
    };

    var TextColorEnumeration = {
        "light": "light",
        "dark": "dark"
    };

    var CONTEXT_NAMESPACE = 'origin-store-promo-feature';

    function originStorePromoFeature(ComponentsConfigFactory, UtilFactory, OriginStorePromoConstant, CSSUtilFactory, NavigationFactory, ScreenFactory, AnimateFactory) {

        function originStorePromoFeatureLink (scope, element) {
            scope.layout = scope.layout || LayoutEnumeration.hero;
            element.addClass(LayoutMap[scope.layout]).addClass(OriginStorePromoConstant.layoutClass);
            
            scope.useH1 = scope.useH1Tag === BooleanEnumeration.true;
            scope.useH2 = !scope.useH1;
            scope.textColor = TextColorEnumeration[scope.textColor] || TextColorEnumeration.light;
            scope.textColorClass = 'text-color-' + scope.textColor;

            scope.openLink = function($event, href){
                $event.preventDefault();
                NavigationFactory.openLink(href);
            };

            scope.strings = {
                titleStr: UtilFactory.getLocalizedStr(scope.titleStr, CONTEXT_NAMESPACE, 'title-str'),
                subtitleStr: UtilFactory.getLocalizedStr(scope.subtitleStr, CONTEXT_NAMESPACE, 'subtitle-str'),
                text: UtilFactory.getLocalizedStr(scope.text, CONTEXT_NAMESPACE, 'text')
            };

            scope.backgroundColor = UtilFactory.getLocalizedStr(scope.backgroundColor, CONTEXT_NAMESPACE, 'background-color');

            if (scope.backgroundColor){
                CSSUtilFactory.setBackgroundColorOfElement(scope.backgroundColor, element, CONTEXT_NAMESPACE);
            }

            function updateBreakPoint(){
                scope.isSmallScreen = ScreenFactory.isSmall();
            }

            function updateBreakPointDigest(){
                updateBreakPoint();
                scope.$digest();
            }

            AnimateFactory.addResizeEventHandler(scope, updateBreakPointDigest, 300);

            updateBreakPoint();

            scope.isHrefExternal = UtilFactory.isAbsoluteUrl(scope.href);

        }

        return {
            restrict: 'E',
            scope: {
                layout: '@',
                backgroundImage : '@',
                featureImage: '@',
                titleStr : '@',
                href: '@',
                subtitleStr: '@',
                backgroundColor: '@',
                useH1Tag: '@',
                ctaText: '@',
                titleImage: '@',
                textColor: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/feature.html'),
            link: originStorePromoFeatureLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoFeature
     * @restrict E
     * @element ANY
     * @description Merchandizes a promo module with a countdown clock as its content.
     * @param {LayoutEnumeration} layout The type of layout this promo will have.
     * @param {ImageUrl} background-image The background image for this module.
     * @param {ImageUrl} feature-image the content image
     * @param {LocalizedString} title-str Promo title
     * @param {LocalizedString} subtitle-str Promo subtitle
     * @param {LocalizedString} cta-text cta text. CTA will not show if not configured
     * @param {ImageUrl} title-image title image will be used only if title-str is not configured
     * @param {Url} href the href for the CTA
     * @param {string} background-color Hex value of the background color
     * @param {BooleanEnumeration} use-h1-tag use H1 Tag for the title to boost SEO relevance. Only one H1 should be present on a page. Optional and defaults to H2
     * @param {TextColorEnumeration} text-color The text/font color of the component. Defaults to 'light'
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *      <origin-store-promo-feature
     *        layout="hero"
     *        image="https://docs.x.origin.com/proto/images/staticpromo/staticpromo_titanfall_@1x.jpg"
     *        title-str="Save Big On All Things Titanfall"
     *        href="app.store.wrapper.bundle-blackfriday"
     *        subtitle-str=""
     *        text=""
     *        callout-text=""
     *        >
     *      </origin-store-promo-feature>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePromoFeature', originStorePromoFeature);
}());
