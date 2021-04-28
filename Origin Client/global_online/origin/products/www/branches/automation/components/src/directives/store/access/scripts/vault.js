
/**
 * @file store/access/scripts/vault.js
 */
(function(){
    'use strict';

    var RESULTS_PER_PAGE = 30;

    function OriginStoreAccessVaultCtrl($scope, SearchFactory, ObjectHelperFactory, ComponentsLogFactory) {
        $scope.games = [];
        $scope.isLoading = true;

        var moreGamesAvailable = false,
            searchOffset = 0;

        // extract offer ids from solr response
        function extractPropertyOrDefault(data, propertyArray, defaultValue) {
            return ObjectHelperFactory.defaultTo(defaultValue)(ObjectHelperFactory.getProperty(propertyArray)(data));
        }

        function updateGamesData(searchResponse) {
            $scope.isLoading = false;

            if (searchResponse.games) {
                var gamesList = extractPropertyOrDefault(searchResponse, ['games', 'game'], []);

                if ($scope.games.length) {
                    $scope.games = $scope.games.concat(gamesList);
                } else {
                    $scope.games = gamesList;
                }
            }

            searchOffset = $scope.games.length;
            moreGamesAvailable = searchOffset < searchResponse.games.numFound;
            $scope.$digest();
        }

        /**
         * Runs search query to grab the next page of results if there are more available
         * @return {promise} The search promise
         */
        $scope.loadMore = function() {
            return Promise.resolve().then($scope.search);
        };

        /**
         * Setting flag to enable lazy-load directive to fire pagination queries
         * @return {boolean}
         */
        $scope.lazyLoadDisabled = function() {
            return $scope.isLoading || !moreGamesAvailable;
        };

        /**
         * Runs search query to grab a page of results
         */
        $scope.search = function() {
            $scope.isLoading = true;

            var searchParams = {
                filterQuery: 'subscriptionGroup:vault-games',
                facetField: '',
                sort: 'rank desc',
                start: searchOffset,
                rows: RESULTS_PER_PAGE
            };

            SearchFactory.searchStore('', searchParams)
                .then(updateGamesData)
                .catch(function(error) {
                    ComponentsLogFactory.error('[StoreSearchFactory: SearchFactory.searchStore] failed', error);
                });
        };
    }

    function originStoreAccessVault(ComponentsConfigFactory, $timeout) {
        function originStoreAccessVaultLink(scope) {
            // Get initial list of games upon load
            $timeout(scope.search, 0);
        }

        return {
            restrict: 'E',
            scope: {},
            controller: 'OriginStoreAccessVaultCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/views/vault.html'),
            link: originStoreAccessVaultLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessVault
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-vault />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreAccessVaultCtrl', OriginStoreAccessVaultCtrl)
        .directive('originStoreAccessVault', originStoreAccessVault);
}());
