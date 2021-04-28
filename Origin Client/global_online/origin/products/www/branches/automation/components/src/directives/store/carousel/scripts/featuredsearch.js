
/**
 * @file store/carousel/scripts/featuredsearch.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var SortDirectionEnumeration = {
        "ascending": "asc",
        "descending": "desc"
    };
    var OrderByEnumeration = {
        "On Sale": "onSale",
        "Price": "dynamicPrice",
        "Product Name": "title",
        "Rank": "rank",
        "Release Date": "releaseDate"
    };

    var TextColorEnumeration = {
        "light": "light",
        "dark": "dark"
    };
    var CtaType = {
        "transparent": "transparent",
        "light": "light"
    };

    var TEXT_CLASS_PREFIX = 'origin-store-carousel-featured-text-color-';

    var SETUP_EVENT = 'carousel:setup',
        FINISHED_EVENT = 'carousel:finished',
        CONTEXT_NAMESPACE = 'origin-store-carousel-featuredsearch';

    function OriginStoreCarouselFeaturedsearchCtrl(
        $attrs,
        $element,
        $scope,
        ComponentsLogFactory,
        ObjectHelperFactory,
        SearchFactory
    ) {

        // set search-parameter defaults
        $scope.direction = $attrs.direction || SortDirectionEnumeration.ascending;
        $scope.facetData = $attrs.facetData || '';
        $scope.limit = $attrs.limit || 30;
        $scope.offset = $attrs.offset || 0;
        $scope.orderBy = $attrs.orderBy || OrderByEnumeration.Rank;
        $scope.searchString = $attrs.searchString || '';
        $scope.textColor = TextColorEnumeration[$scope.textColor] || TextColorEnumeration.light;
        $scope.textColorClass = TEXT_CLASS_PREFIX + $scope.textColor;
        $scope.ctaType = ($scope.textColor === TextColorEnumeration.light) ? CtaType.transparent : CtaType.light;

        var searchParams = {
            filterQuery: $scope.facetData,
            rows: $scope.limit,
            sort: $scope.orderBy + ' ' + $scope.direction,
            start: $scope.offset
        };

        function setOfferData(offers) {
            var carousel = $element.find('.origin-store-carousel-featured-carousel-slider');
            if (offers) {
                $scope.offers = offers;
                carousel.trigger(SETUP_EVENT);
            } else {
                $scope.offers = [];
                carousel.trigger(FINISHED_EVENT);
            }
        }

        function errorHandler(error) {
            ComponentsLogFactory.error('[origin-store-carousel-featuredsearch SearchFactory.searchStore] failed', error);
        }

        SearchFactory.searchStore($scope.searchString, searchParams)
            .then(ObjectHelperFactory.getProperty(['games', 'game']))
            .then(setOfferData)
            .catch(errorHandler);
    }


    /**
    * The directive
    */
    function originStoreCarouselFeaturedsearch(ComponentsConfigFactory, CSSUtilFactory, PriceInsertionFactory) {

        function OriginStoreCarouselFeaturedsearchLink(scope, element) {
            scope.backgroundColor = CSSUtilFactory.setBackgroundColorOfElement(scope.backgroundColor, element, CONTEXT_NAMESPACE);
            scope.strings = {};
            PriceInsertionFactory
                .insertPriceIntoLocalizedStr(scope, scope.strings, scope.titleStr, CONTEXT_NAMESPACE, 'title-str');
            PriceInsertionFactory
                .insertPriceIntoLocalizedStr(scope, scope.strings, scope.text, CONTEXT_NAMESPACE, 'text');
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                description: '@',
                endColor: '@',
                href: '@',
                imageSrc: '@',
                backgroundColor: '@',
                text: '@',
                titleStr: '@',
                overrideVault: '@',
                textColor: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/views/featuredsearch.html'),
            controller: OriginStoreCarouselFeaturedsearchCtrl,
            link: OriginStoreCarouselFeaturedsearchLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselFeaturedsearch
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
     * @param {string} search-string The keyword to do the search for.
     * @param {string} facet-data JSON data of the form-  platform: pc-download, genre: shooter, gameType: basegame
     * @param {SortDirectionEnumeration} direction The direction to sort the data
     * @param {OrderByEnumeration} order-by the field to do the ordering on. Should be one of the data fields returned by solr.
     * @param {number} limit The max number of elements that should be returned.
     * @param {number} offset  The offset into the result set.
     * @param {string} background-color Hex value of the background color
     * @param {BooleanEnumeration} override-vault Override the "included with vault" text
     * @param {TextColorEnumeration} text-color The text/font color of the component. Defaults to 'light'.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-featuredsearch />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreCarouselFeaturedsearchCtrl', OriginStoreCarouselFeaturedsearchCtrl)
        .directive('originStoreCarouselFeaturedsearch', originStoreCarouselFeaturedsearch);
}());
