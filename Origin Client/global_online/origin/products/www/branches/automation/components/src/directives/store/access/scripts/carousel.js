/**
 * @file store/access/scripts/carousel.js
 */
(function(){
    'use strict';

    var sortDirectionEnumeration = {
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

    var SETUP_EVENT = 'carousel:setup';
    var FINISHED_EVENT = 'carousel:finished';

    function OriginStoreAccessCarouselCtrl($scope, $element, $attrs, ObjectHelperFactory, SearchFactory, ComponentsLogFactory, GamesCatalogFactory, OcdPathFactory) {

        // set search-parameter defaults
        $scope.searchString = $scope.searchString || '';
        $scope.facetData = $scope.facetData || '';
        $scope.orderBy = $scope.orderBy || OrderByEnumeration.Rank;
        $scope.direction = $scope.direction || sortDirectionEnumeration.descending;
        $scope.limit = $scope.limit || 10;
        $scope.offset = $scope.offset || 0;
        $scope.models = [];

        var searchParams = {
            filterQuery: $scope.facetData,
            sort: $scope.orderBy + ' ' + $scope.direction,
            start: $scope.offset,
            rows: $scope.limit
        };

        function setOfferData(offers) {
            if (offers) {
                $scope.offers = offers;
                // Tell the carousel to recalculate its sizing, as new items were added
                $element.trigger(SETUP_EVENT);

                var offerIds = _.values(ObjectHelperFactory.map(ObjectHelperFactory.getProperty('gameId'), offers));

                GamesCatalogFactory
                    .getCatalogInfo(offerIds)
                    .then(function(data) {
                        $scope.models = data;
                    });
            } else {
                $scope.offers = [];
                // Tell the carousel "we're done whether there's items or not", in the case of an error or empty response,
                // otherwise the carousel will show a "loading" spinner forever.
                $element.trigger(FINISHED_EVENT);

                angular.element($element).slideUp();
            }
        }

        function errorHandler(error) {
            ComponentsLogFactory.error('[origin-store-access-carousel SearchFactory.searchStore] failed', error);
        }

        function handleHandMerchandising(data) {
            // Tell the carousel to recalculate its sizing, as new items were added
            $element.trigger(SETUP_EVENT);
            $scope.models = data;
        }

        // use hand merchandised offers over solr search
        if($scope.paths) {
            $scope.paths = $scope.paths.split(',');

            OcdPathFactory
                .get($scope.paths)
                .attachToScope($scope, handleHandMerchandising);
        } else {
            SearchFactory.searchStore($scope.searchString, searchParams)
                .then(ObjectHelperFactory.getProperty(['games', 'game']))
                .then(setOfferData)
                .catch(errorHandler);
        }
    }

    /**
    * The directive
    */
    function originStoreAccessCarousel(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                searchString: '@',
                facetData: '@',
                orderBy: '@',
                direction: '@',
                limit: '@',
                offset: '@',
                paths: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/views/carousel.html'),
            controller: OriginStoreAccessCarouselCtrl
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessCarousel
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {string} paths Comma seperated list of hand merchandised offer paths ex: /battlefield/battlefield-4/standard-edition,/battlefield/battlefield-4/premium-edition,/battlefield/battlefield-4/digital-deluxe-edition
     * @param {string} search-string The keyword to do the search for.
     * @param {string} facet-data JSON array of data of the form {genre: action}
     * @param {sortDirectionEnumeration} direction The direction to sort the data
     * @param {string} order-by the feild to do the ordering on. Should be one of the data fields returned by solr.
     * @param {number} limit The max number of elements that should be returned.
     * @param {OrderByEnumeration} order-by the field to do the ordering on. Should be one of the data fields returned by solr.
     * @param {number} offset The offset into the result set.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-carousel
     *          facet-data="genre:action"
     *          limit="10"
     *          offset="0"
     *          order-by="rank"
     *          search-string="some game"
     *          direction="asc">
     *      </origin-store-access-productsearch>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreAccessCarouselCtrl', OriginStoreAccessCarouselCtrl)
        .directive('originStoreAccessCarousel', originStoreAccessCarousel);
}());
