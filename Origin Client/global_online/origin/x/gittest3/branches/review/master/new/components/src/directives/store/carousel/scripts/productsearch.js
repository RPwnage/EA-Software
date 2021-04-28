
/**
 * @file store/carousel/scripts/productsearch.js
 */
(function(){
    'use strict';
    var SEARCH_PROVIDER= 'store';
    var sortDirectionEnumeration = {
        "ascending": "asc",
        "descending": "desc"
    };
    var SETUP_EVENT = 'carousel:setup';

    /**
    * The directive
    */
    function originStoreCarouselProductsearch(ComponentsConfigFactory, SearchFactory) {

        var searchDefaults = {
            'searchString': '',
            'facetData': '',
            'orderBy': 'title',
            'direction': sortDirectionEnumeration.ascending,
            'limit': 30,
            'offeset': 0
        };

        function setDefaults(scope) {
            scope.direction = sortDirectionEnumeration[scope.direction];
            for(var param in searchDefaults) {
                if(scope[param] === undefined) {
                    scope[param] = searchDefaults[param];
                }
            }
        }

        /**
        * The directive link
        */
        function originStoreCarouselProductsearchLink(scope, elem) {
            var searchCtx;
            // Add Scope functions and call the controller from here
            setDefaults(scope);
            searchCtx = SearchFactory.createSearchContext();
            searchCtx.events.on('search:finished', function() {
                var results = searchCtx.getResults(SEARCH_PROVIDER);
                scope.offers = results.game;
                scope.$apply();
                elem.trigger(SETUP_EVENT);
            });

            var params = {
                fq: scope.facetData,
                sort: scope.orderBy + ' ' + scope.direction,
                start: scope.offset,
                rows: scope.limit
            };

            SearchFactory.search(searchCtx, {
                searchString: scope.searchString,
                includes: scope.includes,
                store: params
            });
        }
        return {
            restrict: 'E',
            replace: true,
            scope: {
                searchString: '@',
                facetData: '@',
                orderBy: '@',
                direction: '@',
                title: '@',
                viewAllStr: '@',
                limit: '@',
                offset: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/views/productsearch.html'),
            link: originStoreCarouselProductsearchLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselProductsearch
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {string} searchString The keyword to do the search for.
     * @param {string} facetData JSON array of data of the form {platform: [pc-dowlnload], genre: [shooter], gametype: [basegame]}
     * @param {sortDirectionEnumeration} direction The direction to sort the data
     * @param {string} orderBy the feild to do the ordering on. Should be one of the data fields returned by solr.
     * @param {LocalizedString} title Main title for the component
     * @param {LocalizedString} viewAllStr The text for the view all link
     * @param {number} limit The max number of elements that should be returned.
     * @param {number} offset  The offset into the result set.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-productsearch />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselProductsearch', originStoreCarouselProductsearch);
}());
