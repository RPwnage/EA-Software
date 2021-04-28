
/**
 * @file store/carousel/scripts/productsearch.js
 */
(function(){
    'use strict';

    var sortDirectionEnumeration = {
        "ascending": "asc",
        "descending": "desc"
    };

    var SETUP_EVENT = 'carousel:setup';
    var FINISHED_EVENT = 'carousel:finished';

    function OriginStoreCarouselProductsearchCtrl($scope, $element, $attrs, ObjectHelperFactory, SearchFactory, ComponentsLogFactory) {

        // set search-parameter defaults
        $scope.searchString = $attrs.searchString || '';
        $scope.facetData = $attrs.facetData || '';
        $scope.orderBy = $attrs.orderBy || 'title';
        $scope.direction = $attrs.direction || sortDirectionEnumeration.ascending;
        $scope.limit = $attrs.limit || 30;
        $scope.offset = $attrs.offset || 0;

        var searchParams = {
            fq: $scope.facetData,
            sort: $scope.orderBy + ' ' + $scope.direction,
            start: $scope.offset,
            rows: $scope.limit
        };

        function setOfferData(offers) {
            if (offers) {
                $scope.offers = offers;
                // Tell the carousel to recalculate its sizing, as new items were added
                $element.trigger(SETUP_EVENT);
            } else {
                $scope.offers = [];
                // Tell the carousel "we're done whether there's items or not", in the case of an error or empty response,
                // otherwise the carousel will show a "loading" spinner forever.
                $element.trigger(FINISHED_EVENT);
            }
        }

        function errorHandler(error) {
            ComponentsLogFactory.error('[origin-store-carousel-productsearch SearchFactory.searchStore] failed', error.message);
        }

        SearchFactory.searchStore($scope.searchString, searchParams)
            .then(ObjectHelperFactory.getProperty(['result', 'games', 'game']))
            .then(setOfferData)
            .catch(errorHandler);

    }

    /**
    * The directive
    */
    function originStoreCarouselProductsearch(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                title: '@',
                viewAllStr: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/views/productsearch.html'),
            controller: OriginStoreCarouselProductsearchCtrl
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
     * @param {string} search-string The keyword to do the search for.
     * @param {string} facet-data JSON array of data of the form {platform: [pc-dowlnload], genre: [shooter], gametype: [basegame]}
     * @param {sortDirectionEnumeration} direction The direction to sort the data
     * @param {string} order-by the feild to do the ordering on. Should be one of the data fields returned by solr.
     * @param {LocalizedString} title Main title for the component
     * @param {LocalizedString} view-all-str The text for the view all link
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
        .controller('OriginStoreCarouselProductsearchCtrl', OriginStoreCarouselProductsearchCtrl)
        .directive('originStoreCarouselProductsearch', originStoreCarouselProductsearch);
}());
