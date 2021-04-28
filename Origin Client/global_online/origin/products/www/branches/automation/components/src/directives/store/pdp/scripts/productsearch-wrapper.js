/**
 * @file store/pdp/scripts/productsearch-wrapper.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */
    var SortDirectionEnumeration = {
        "ascending": "asc",
        "descending": "desc"
    };
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var OrderByEnumeration = {
        "dynamicPrice": "dynamicPrice",
        "offerType": "offerType",
        "onSale": "onSale",
        "rank": "rank",
        "releaseDate": "releaseDate",
        "title": "title"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-pdp-carousel-productsearch-wrapper';
    var REQUIRED_ATTRIBUTES = [
        'carouselTitle',
        'searchString',
        'facetData',
        'orderBy',
        'direction',
        'limit',
        'offset'
    ];

    function originStorePdpCarouselProductsearchWrapper(ComponentsConfigFactory, DirectiveScope) {

        function originStorePdpCarouselProductsearchWrapperLink(scope) {
            DirectiveScope.populateScope(scope, CONTEXT_NAMESPACE)
                .then(function() {
                    scope.ocdDataReady = _.find(REQUIRED_ATTRIBUTES, function(key) {
                        return angular.isDefined(scope[key]) && scope[key] !== null;
                    });
                    scope.$digest();
                });
        }

        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                anchorName: '@',
                showOnNav: '@',
                headerTitle: '@',
                carouselTitle: '@',
                searchString: '@',
                facetData: '@',
                orderBy: '@',
                direction: '@',
                limit: '@',
                offset: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/productsearch-wrapper.html'),
            link: originStorePdpCarouselProductsearchWrapperLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpCarouselProductsearchWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @param {LocalizedString} header-title title of this section
     * @param {string} search-string The keyword to do the search for.
     * @param {string} facet-data JSON data of the form-  platform: pc-download, genre: shooter, gameType: basegame
     * @param {SortDirectionEnumeration} direction The direction to sort the data
     * @param {OrderByEnumeration} order-by the feild to do the ordering on. Should be one of the data fields returned by solr.
     * @param {number} limit The max number of elements that should be returned.
     * @param {number} offset  The offset into the result set.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-productsearch-wrapper />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpCarouselProductsearchWrapper', originStorePdpCarouselProductsearchWrapper);
}());
