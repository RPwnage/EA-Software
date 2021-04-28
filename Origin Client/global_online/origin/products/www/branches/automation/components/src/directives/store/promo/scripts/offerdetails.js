/**
 * @file store/promo/scripts/offerdetails.js
 */
(function () {
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

    var CONTEXT_NAMESPACE = 'origin-store-promo-offerdetails';
    
    
    function originStorePromoOfferdetailsCtrl($scope, $element, CSSUtilFactory, PriceInsertionFactory, OriginStorePromoConstant, UtilFactory) {
        function getHref() {
            if (!$scope.href) {
                return  $scope.ocdPath ? 'store' + $scope.ocdPath : null;
            }
            return $scope.href;
        }

        this.onScopeResolved = function() {
            if ($scope.layout) {
                $element.addClass(LayoutMap[$scope.layout]).addClass(OriginStorePromoConstant.layoutClass);
                if ($scope.layout !== LayoutEnumeration.hero) {
                    $scope.badgeImage = null;
                }
            }

            $scope.backgroundColor = CSSUtilFactory.setBackgroundColorOfElement($scope.backgroundColor, $element, CONTEXT_NAMESPACE);
            $scope.strings = {};
            $scope.strings.othText = UtilFactory.getLocalizedStr($scope.othText, CONTEXT_NAMESPACE, 'oth-text');

            $scope.href = getHref();

            if ($scope.titleStr) {
                PriceInsertionFactory.insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.titleStr, CONTEXT_NAMESPACE, 'title-str');
            }
            if ($scope.bulletOne) {
                PriceInsertionFactory.insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.bulletOne, CONTEXT_NAMESPACE, 'bullet-one');
            }
            if ($scope.bulletTwo) {
                PriceInsertionFactory.insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.bulletTwo, CONTEXT_NAMESPACE, 'bullet-two');
            }

            $scope.useH1 = $scope.useH1Tag === BooleanEnumeration.true;
            $scope.useH2 = !$scope.useH1;

            $scope.isPackartShown = $scope.titleImage ? BooleanEnumeration.false : $scope.showPackart;
            $scope.$watchOnce('model', function(){
                $scope.isOth = $scope.model.oth;
            });
            $scope.showAccessLogo = $scope.showAccessBadge === BooleanEnumeration.true;

			$element.addClass('origin-telemetry-store-promo');
        };
    }

    function originStorePromoOfferdetails(ComponentsConfigFactory, DirectiveScope) {
        function originStorePromoOfferdetailsLink(scope, element, attributes, controller) {

            DirectiveScope.populateScopeWithOcdAndCatalog(scope, CONTEXT_NAMESPACE, scope.ocdPath)
                .then(controller.onScopeResolved);
        }

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                layout: '@',
                image: '@',
                titleStr: '@',
                href: '@',
                ctaText: '@',
                videoid: '@',
                matureContent: '@',
                ocdPath: '@',
                showPackart: '@',
                bulletOne: '@',
                bulletTwo: '@',
                backgroundColor: '@',
                badgeImage: '@',
                ratingFontColor: '@',
                useH1Tag: '@',
                titleImage: '@',
                othText: '@',
                showAccessBadge: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/promo/views/offerdetails.html'),
            controller: 'originStorePromoOfferdetailsCtrl',
            link: originStorePromoOfferdetailsLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePromoOfferdetails
     * @restrict E
     * @element ANY
     * @description Merchandizes a promo module with offer details as its content.
     * @param {LayoutEnumeration} layout The type of layout this promo will have.
     * @param {ImageUrl} image The background image for this module.
     * @param {ImageUrl} badge-image optional badge image. Only applicable to hero layout
     * @param {LocalizedTemplateString} title-str Promo title
     * @param {ImageUrl} title-image title image. To be used in replacement of the title string.
     * @param {Url} href Url that this module links to
     * @param {LocalizedString} cta-text Text for the call to action. Ex. "Learn More" (Optional).
     * @param {Video} videoid The youtube video ID of the video for this promo (Optional).
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {string} background-color Hex value of the background color
     * @param {OcdPath} ocd-path OCD Path
     * @param {BooleanEnumeration} show-packart Determines whether the pack art from ocdpath will be displayed
     * @param {LocalizedTemplateString} bullet-one First bullet point (Optional)
     * @param {LocalizedTemplateString} bullet-two Second bullet point (Optional)
     * @param {RatingEnumeration} rating-font-color color of the ratings font (TW only)
     * @param {String} ctid Content tracking ID
     * @param {BooleanEnumeration} use-h1-tag use H1 Tag for the title to boost SEO relevance. Only one H1 should be present on a page. Optional and defaults to H2
     * @param {BooleanEnumeration} show-access-badge shows access badge above title in the banner
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-promo-offerdetails
     *         layout="hero"
     *         image="https://docs.x.origin.com/proto/images/store/wide-ggg.jpg"
     *         header="We Stand By Our Games."
     *         href="app.store.wrapper.bundle-blackfriday"
     *         cta-text="Learn Stuff"
     *         videoid="wS7gfJqFoA"">
     *     </origin-store-promo-offerdetails>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originStorePromoOfferdetailsCtrl', originStorePromoOfferdetailsCtrl)
        .directive('originStorePromoOfferdetails', originStorePromoOfferdetails);
}());
