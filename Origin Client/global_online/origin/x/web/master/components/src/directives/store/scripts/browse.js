/**
 * @file store/browse.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-browse',
        RESULTS_PER_PAGE = 30;

    function OriginStoreBrowseCtrl($scope, StoreSearchQueryFactory, UtilFactory, StoreSearchFactory) {
        $scope.localizedStrings = {
            'titleStr': UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'titlestr')
        };

        $scope.games = [];
        $scope.isLoading = true;
        var moreGamesAvailable = false,
            searchOffset = 0;

        function setGamesData(searchResponse) {
            $scope.isLoading = false;
            if (searchResponse.games) {
                if ($scope.games.length) {
                    $scope.games = $scope.games.concat(searchResponse.games);
                } else {
                    $scope.games = searchResponse.games;
                }
            }
            searchOffset = $scope.games.length;
            moreGamesAvailable = searchOffset < searchResponse.totalResults;
            $scope.$digest();
        }

        /**
         * Runs search query to grab the next page of results if there are more available
         * @return {void}
         */
        $scope.loadMore = function() {
            search();
        };

        /**
         * Setting flag to enable lazy-load directive to fire pagination queries
         * @return {boolean}
         */
        $scope.lazyLoadDisabled = function() {
            return $scope.isLoading || !moreGamesAvailable;
        };

        function search() {
            $scope.isLoading = true;
            StoreSearchFactory
                .reset()
                .setSearchString(StoreSearchQueryFactory.getSearchStringQueryParam())
                .setFacets(StoreSearchQueryFactory.getFacetsQueryParam())
                .setSortBy(StoreSearchQueryFactory.getSortQueryParam())
                .setLimit(RESULTS_PER_PAGE)
                .setOffset(searchOffset)
                .search()
                .then(setGamesData);
        }

        // Get initial list of games upon load
        search();
    }

    function originStoreBrowse(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {
                titleStr: '@'
            },
            controller: OriginStoreBrowseCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/browse.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreBrowse
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Wrapper for Store Browse/Search page layout - place search facets & tile list inside this to build Browse page
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-browse>
     *              <origin-store-search-facets>
     *                  <origin-store-facet-category label="Sort" category-id="sort">
     *                      <origin-store-facet-option category-id="sort" label="A to Z" facet-id="title asc"></origin-store-facet-option>
     *                      <origin-store-facet-option category-id="sort" label="Z to A" facet-id="title desc"></origin-store-facet-option>
     *                  </origin-store-facet-category>
     *              </origin-store-search-facets>
     *         <origin-store-browse>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginStoreBrowseCtrl', OriginStoreBrowseCtrl)
        .directive('originStoreBrowse', originStoreBrowse);

}());
