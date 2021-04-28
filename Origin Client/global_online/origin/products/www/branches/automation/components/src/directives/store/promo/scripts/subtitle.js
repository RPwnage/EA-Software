/**
 * @file store/promo/scripts/subtitle.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */


    var RatingEnumeration = {
        "Light": "light",
        "Dark": "dark"
    };
    /* jshint ignore:end */

    var LayoutEnumeration = {
        "hero": "hero",
        "full-width": "full-width",
        "half-width": "half-width"
    };

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var LayoutMap = {
        "hero": "l-origin-store-column-hero l-origin-section-divider",
        "full-width": "l-origin-store-column-full",
        "half-width": "l-origin-store-column-half"
    };

    var AVAILABLE_DATE_FORMAT = 'L';

    var CONTEXT_NAMESPACE = 'origin-store-promo-subtitle';

    function OriginStorePromoSubtitleCtrl($scope, $element, moment, CSSUtilFactory, PriceInsertionFactory, OriginStorePromoConstant, UtilFactory) {
         function formatAvailableText(text, startDate, endDate){
            var startDateFormatted = startDate ? moment(startDate).format(AVAILABLE_DATE_FORMAT) : '';
            var endDateFormatted = endDate ? moment(endDate).format(AVAILABLE_DATE_FORMAT) : '';

            return text ? text.replace('%startDate%', startDateFormatted).replace('%endDate%', endDateFormatted) : '';
        }

        function getHref() {
            if (!$scope.href) {
                return  $scope.ocdPath ? 'store' + $scope.ocdPath : null;
            }
            return $scope.href;
        }

        this.onScopeResolved = function() {
            var calloutReplaceText;
            if ($scope.layout) {
                $element.addClass(LayoutMap[$scope.layout]).addClass(OriginStorePromoConstant.layoutClass);
                if ($scope.layout !== LayoutEnumeration.hero) {
                    $scope.badgeImage = null;
                }
            }
            $scope.availableTextFormatted = formatAvailableText($scope.availableText, $scope.startDate, $scope.endDate);
            $scope.backgroundColor = CSSUtilFactory.setBackgroundColorOfElement($scope.backgroundColor, $element, CONTEXT_NAMESPACE);
            $scope.strings = {};
            $scope.href = getHref();
            $scope.showOcdPricing = $scope.showPricing === BooleanEnumeration.true;
            $scope.showPriceCallout = $scope.showPriceCallout === BooleanEnumeration.true;
            $scope.showDiscountPriceCallout = $scope.showDiscountPriceCallout === BooleanEnumeration.true;
            $scope.showDIscountPercentageCallout = $scope.showDIscountPercentageCallout === BooleanEnumeration.true;
            $scope.strings.othText = UtilFactory.getLocalizedStr($scope.othText, CONTEXT_NAMESPACE, 'oth-text');

            if($scope.href) {
                $element.addClass(OriginStorePromoConstant.clickableClass);
            }

            if ($scope.titleStr) {
                calloutReplaceText = $scope.calloutText ? '<span class="callout-text origin-telemetry-callout-text">' + $scope.calloutText + '</span>' : '';
                var titleStr = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'title-str', {
                    '%callout-text%': calloutReplaceText
                });

                if ($scope.useDynamicPriceCallout){
                    PriceInsertionFactory
                        .insertPriceIntoLocalizedStr($scope, $scope.strings, titleStr, CONTEXT_NAMESPACE, 'title-str');
                }else if ($scope.useDynamicDiscountPriceCallout){
                    PriceInsertionFactory
                        .insertDiscountedPriceIntoLocalizedStr($scope, $scope.strings, titleStr, CONTEXT_NAMESPACE, 'title-str');
                }else if ($scope.useDynamicDiscountRateCallout){
                    PriceInsertionFactory
                        .insertDiscountedPercentageIntoLocalizedStr($scope, $scope.strings, titleStr, CONTEXT_NAMESPACE, 'title-str');
                }else{
                    PriceInsertionFactory
                        .insertPriceIntoLocalizedStr($scope, $scope.strings, titleStr, CONTEXT_NAMESPACE, 'title-str');
                }
            }
            if ($scope.text) {
                PriceInsertionFactory.insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.text, CONTEXT_NAMESPACE, 'text');
            }

            if ($scope.titleStr && !$scope.text && !$scope.ctaText){
                $scope.titlePositionClass = 'origin-store-promo-banner-title-position-center';
            }

            $scope.useH1 = $scope.useH1Tag === BooleanEnumeration.true;
            $scope.isPackartShown = $scope.titleImage ? BooleanEnumeration.false : $scope.showPackart;
            $scope.$watchOnce('model', function(){
                $scope.isOth = $scope.model.oth;
            });
            $scope.showAccessLogo = $scope.showAccessBadge === BooleanEnumeration.true;

            $element.addClass('origin-telemetry-store-promo');
        };
    }

    function originStorePromoSubtitle(ComponentsConfigFactory, DirectiveScope) {
        function originStorePromoSubtitleLink (scope, element, attributes, controller) {
            DirectiveScope.populateScopeWithOcdAndCatalog(scope, CONTEXT_NAMESPACE, scope.ocdPath)
                .then(controller.onScopeResolved);
        }

        return {
            restrict: 'E',
            scope: {
                layout: '@',
                image : '@',
                titleStr : '@',
                text : '@',
                href : '@',
                ctaText: '@',
                videoid : '@',
                matureContent: '@',
                startDate: '@',
                endDate: '@',
                availableText: '@',
                ocdPath: '@',
                showPricing: '@',
                showTestCheckbox: '@',
                showPackart: '@',
                backgroundColor: '@',
                badgeImage: '@',
                ratingFontColor: '@',
                useH1Tag: '@',
                calloutText: '@',
                useDynamicPriceCallout: '@',
                useDynamicDiscountPriceCallout: '@',
                useDynamicDiscountRateCallout: '@',
                othText: '@',
                titleImage: '@',
                showAccessBadge: '@'
            },
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/subtitle.html'),
            controller: 'OriginStorePromoSubtitleCtrl',
            link: originStorePromoSubtitleLink
        };
    }

    angular.module('origin-components')
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoSubtitle
     * @restrict E
     * @element ANY
     * @scope
     * @description Mercahndizes a promo module with a title/subtitle as its content
     * @param {LayoutEnumeration} layout The type of layout this promo will have.
     * @param {ImageUrl} image The background image for this module.
     * @param {ImageUrl} badge-image optional badge image. Only applicable to hero layout
     * @param {LocalizedTemplateString} title-str Promo title
     * @param {LocalizedTemplateString} text Promo subtitle
     * @param {Url} href Url that this module links to
     * @param {LocalizedString} cta-text Text for the call to action. Ex. "Learn More" (Optional).
     * @param {Video} videoid The youtube video ID of the video for this promo (Optional).
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {string} background-color Hex value of the background color
     * @param {DateTime} start-date  Date the offer may be release or go on sale (Optional). (UTC time)
     * @param {DateTime} end-date Date the offer may be available till date or sale date (Optional). (UTC time)
     * @param {LocalizedString} available-text The string that holds the dates. The dates will subbed in the place of %startDate% and %endDate%. EG: "Available %startDate% - %endDate% would yeild "Available 6/10/2015 - 6/15/2015" (Optional).
     * @param {OcdPath} ocd-path OCD Path
     * @param {BooleanEnumeration} show-pricing Show pricing from offer data path. Defaults to false.
     * @param {BooleanEnumeration} show-packart Determines whether the pack art from ocdpath will be displayed
     * @param {RatingEnumeration} rating-font-color color of the ratings font (TW only)
     * @param {String} ctid Content tracking ID
     * @param {BooleanEnumeration} use-h1-tag use H1 Tag for the title to boost SEO relevance. Only one H1 should be present on a page. Optional and defaults to H2
     * @param {LocalizedString} callout-text callout text that will be replaced into placeholder %callout-text% in title str and text
     * @param {BooleanEnumeration} use-dynamic-price-callout flag to use catalog price information that will replace placeholder %~offer-id~%
     * @param {BooleanEnumeration} use-dynamic-discount-price-callout flag to use catalog discount price information that will replace placeholder %~offer-id~%
     * @param {BooleanEnumeration} use-dynamic-discount-rate-callout flag to use catalog discount rate information that will reaplce placeholder %~offer-id~%
     * @param {LocalizedString} oth-text * on the house text.
     * @param {ImageUrl} title-image title image. Will be used only if title str is not merchandised
     * @param {BooleanEnumeration} show-access-badge shows access badge above title in the banner
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-promo-subtitle
     *         layout="hero"
     *         image="https://docs.x.origin.com/proto/images/store/wide-ggg.jpg"
     *         title-str="We Stand By Our Games."
     *         text="If you don't love it, return it."
     *         href="app.store.wrapper.bundle-blackfriday"
     *         cta-text="Learn Stuff"
     *         videoid="wS7gfJqFoA"
     *         mature-content="true"
     *         start-date="June 10 2015 00:00:00 PST"
     *         end-date="June 15 2015 01:00:00 EST"
     *         available-text="Available %startDate% - %endDate%">
     *     </origin-store-promo-subtitle>
     *     </file>
     * </example>
     */
        .controller('OriginStorePromoSubtitleCtrl', OriginStorePromoSubtitleCtrl)
        .directive('originStorePromoSubtitle', originStorePromoSubtitle);
}());
