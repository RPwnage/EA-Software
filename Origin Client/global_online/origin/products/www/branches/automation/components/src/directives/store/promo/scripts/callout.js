/**
 * @file store/promo/scripts/callout.js
 */
(function(){
    'use strict';
    var HeaderSizeEnumeration = {
        "small": "small",
        "medium": "medium",
        "large": "large"
    };

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
        "hero": "l-origin-store-column-hero-callout l-origin-section-divider",
        "full-width": "l-origin-store-column-full origin-store-promo-callout-full",
        "half-width": "l-origin-store-column-half origin-store-promo-callout-half"
    };

    var HeaderSizeClassMap = {
        "small": "otktitle-3-condensed" ,
        "medium": "otktitle-2-condensed",
        "large": "otktitle-1-condensed"
    };

    var TextColorEnumeration = {
        "light": "light",
        "dark": "dark"
    };

    var CONTEXT_NAMESPACE = 'origin-store-promo-callout';
    var TEXT_CLASS_PREFIX = 'origin-store-promo-callout-text-color-';


    function OriginStorePromoCalloutCtrl($scope, $element, UtilFactory, OriginStorePromoConstant, CSSUtilFactory, $sce){
        $scope.layout = $scope.layout || LayoutEnumeration.hero;
        $element.addClass(LayoutMap[$scope.layout]).addClass(OriginStorePromoConstant.layoutClass);

        // Set header HTML tag for main title
        $scope.useH1 = $scope.useH1Tag === BooleanEnumeration.true;
        $scope.useH2 = !$scope.useH1Tag;

        // Set header styles for main title
        var headerSize = $scope.headerSize || HeaderSizeEnumeration.medium;
        $scope.headerSizeClass = _.get(HeaderSizeClassMap, headerSize);
        
        function buildKeyTextHtmlMarkup(keyText){
            var openingMarkup = '<span class="callout-text otktitle-4-condensed origin-telemetry-callout-text" ',
                closingMarkup = '>'+ keyText + '</span>',
                inlineStyles = '';

            // conditionally add inline styles
            if($scope.calloutBackgroundColor || $scope.calloutBorderColor) {
                inlineStyles = 'style="background-color:'+ $scope.calloutBackgroundColor +'; border-color: '+ $scope.calloutBorderColor +';" ';
            }

            return openingMarkup + inlineStyles + closingMarkup ;
        }

        this.init = function(contextNamespace){
            $scope.backgroundColor = CSSUtilFactory.setBackgroundColorOfElement($scope.backgroundColor, $element, contextNamespace);

            $scope.calloutBackgroundColor = CSSUtilFactory.hexToRgba($scope.calloutTextBackgroundColor);

            $scope.calloutBorderColor = CSSUtilFactory.hexToRgba($scope.calloutTextBorderColor);

            $scope.textColor = TextColorEnumeration[$scope.textColor] || TextColorEnumeration.light;
            $scope.textColorClass = TEXT_CLASS_PREFIX + $scope.textColor;

            $scope.strings = {
                // $sce, because angular strips inline style tags
                textFormatted: $sce.trustAsHtml(UtilFactory.getLocalizedStr($scope.text, contextNamespace, 'text', {'%callout-text%': buildKeyTextHtmlMarkup($scope.calloutText)})),
                legalDisclaimerText: UtilFactory.getLocalizedStr($scope.legalDisclaimerText, contextNamespace, 'legal-disclaimer-text'),
                legalDisclaimerLinkText: UtilFactory.getLocalizedStr($scope.legalDisclaimerLinkText, contextNamespace, 'legal-disclaimer-link-text'),
                legalDisclaimerDialogHeaderText: UtilFactory.getLocalizedStr($scope.legalDisclaimerDialogHeaderText, contextNamespace, 'legal-disclaimer-dialog-header-text'),
                legalDisclaimerDialogBodyTitleText: UtilFactory.getLocalizedStr($scope.legalDisclaimerDialogBodyTitleText, contextNamespace, 'legal-disclaimer-dialog-body-title-text'),
                legalDisclaimerDialogBodyText: UtilFactory.getLocalizedStr($scope.legalDisclaimerDialogBodyText, contextNamespace, 'legal-disclaimer-dialog-body-text')
            };
        };
    }

    function originStorePromoCallout(ComponentsConfigFactory){
        function originStorePromoCalloutLink(scope, element, attrs, ctrl){
            ctrl.init(CONTEXT_NAMESPACE);
        }
        return {
            restrict: 'E',
            scope: {
                layout: '@',
                image : '@',
                titleStr : '@',
                href: '@',
                subtitleStr: '@',
                text: '@',
                calloutText: '@',
                calloutTextBackgroundColor: '@',
                calloutTextBorderColor: '@',
                headerSize: '@',
                backgroundColor: '@',
                useH1Tag: '@',
                legalDisclaimerText: '@',
                legalDisclaimerLinkText: '@',
                legalDisclaimerDialogHeaderText: '@',
                legalDisclaimerDialogBodyTitleText: '@',
                legalDisclaimerDialogBodyText: '@',
                textColor: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/callout.html'),
            controller: OriginStorePromoCalloutCtrl,
            link: originStorePromoCalloutLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoCallout
     * @restrict E
     * @element ANY
     * @description Merchandizes a promo module with discount information callout
     * @param {LayoutEnumeration} layout The type of layout this promo will have.
     * @param {ImageUrl} image The background image for this module.
     * @param {HeaderSizeEnumeration} header-size - The header size. The following are char limits per header size: Small {full-width/hero: DO NOT USE, half-width: 12 char). Medium {full-width/hero: 18 char, half-width: 9 char}. Large { full-width/hero: 13 char, half-width: DO NOT USE}
     * @param {LocalizedString} title-str Promo title
     * @param {LocalizedString} subtitle-str Promo subtitle
     * @param {LocalizedString} text promo text
     * @param {LocalizedString} callout-text - promo key text. Will be replaced into placeholder %callout-text% in text field
     * @param {string} callout-text-background-color Hex value of the callout text background color
     * @param {string} callout-text-border-color Hex value of the callout text border color
     * @param {string} background-color Hex value of the background (and scrim) color
     * @param {BooleanEnumeration} use-h1-tag use H1 Tag for the title to boost SEO relevance. Only one H1 should be present on a page. Optional and defaults to H2
     * @param {LocalizedString} legal-disclaimer-text legal claimer text
     * @param {LocalizedString} legal-disclaimer-link-text - legal disclaimer link text. Will be replaced into placeholder %legal-disclaimer-link-text% in legal-disclaimer-text
     * @param {LocalizedString} legal-disclaimer-dialog-header-text legal disclaimer dialog header text.
     * @param {LocalizedString} legal-disclaimer-dialog-body-title-text legal disclaimer dialog body title
     * @param {LocalizedString} legal-disclaimer-dialog-body-text legal disclaimer dialog body text
     * @param {TextColorEnumeration} text-color The text/font color of the component. Defaults to 'light'.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *      <origin-store-promo-callout
     *        layout="hero"
     *        image="https://docs.x.origin.com/proto/images/staticpromo/staticpromo_titanfall_@1x.jpg"
     *        title-str="Save Big On All Things Titanfall"
     *        href="app.store.wrapper.bundle-blackfriday"
     *        subtitle-str=""
     *        text=""
     *        callout-text=""
     *        >
     *      </origin-store-promo-callout>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePromoCalloutCtrl', OriginStorePromoCalloutCtrl)
        .directive('originStorePromoCallout', originStorePromoCallout);
}());
