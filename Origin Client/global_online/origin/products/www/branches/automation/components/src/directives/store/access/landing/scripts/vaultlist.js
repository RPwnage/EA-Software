
/** 
 * @file store/access/landing/scripts/vaultlist.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-access-landing-vaultlist';
    var DISPLAY_LIMIT = 16;
    var SEARCH_LIMIT = 30;
    var DirectionEnumeration = {
        "Ascending": "asc",
        "Descending": "desc"
    };
    var SortEnumeration = {
        "Rank": "rank",
        "Release Date": "releaseDate",
        "Price": "dynamicPrice",
        "Product Name": "title"
    };

    function OriginStoreAccessLandingVaultlistCtrl($scope, $element, UtilFactory, SearchFactory, ObjectHelperFactory, OcdPathFactory, ComponentsLogFactory) {
        var filterQuery = _.get($scope, 'facet', ''),
            searchQuery = _.get($scope, 'query', ''),
            sort = _.get($scope, 'sort', SortEnumeration.Rank),
            dir = _.get($scope, 'dir', DirectionEnumeration.Descending),
            rows = _.get($scope, 'rows', SEARCH_LIMIT);

        var searchParams = {
            'filterQuery': filterQuery,
            'facetField': '',
            'sort': sort + ' ' + dir,
            'start': 0,
            'rows': rows
        };

        $scope.open = false;
        $scope.showCta = false;
        $scope.strings = {
            header: UtilFactory.getLocalizedStr($scope.header, CONTEXT_NAMESPACE, 'header'),
            more: UtilFactory.getLocalizedStr($scope.more, CONTEXT_NAMESPACE, 'more'),
            less: UtilFactory.getLocalizedStr($scope.less, CONTEXT_NAMESPACE, 'less')
        };

        function handleModels(response) {
            var limitedResponse = [];
            var i = 0;

            // show ctas if more offers than the display limit
            $scope.showCta = _.size(response) > DISPLAY_LIMIT;

            _.forEach(response, function(value) {
                if(i < DISPLAY_LIMIT) {
                    limitedResponse.push(value);
                } else {
                    return false;
                }

                i++;
            });

            $scope.models = limitedResponse;

            $scope.$watch('open', function(newValue) {
                if(newValue) {
                    $scope.models = response;
                } else {
                    $scope.models = limitedResponse;
                }
            });
        }

        function setOfferData(offers) {
            if (offers) {
                var paths = ObjectHelperFactory.toArray(ObjectHelperFactory.map(ObjectHelperFactory.getProperty('path'), offers));

                OcdPathFactory
                    .get(paths)
                    .attachToScope($scope, handleModels);
            } else {
                angular.element($element).slideUp();
            }
        }

        function errorHandler(error) {
            ComponentsLogFactory.error('[origin-store-access-landing-vaultlist SearchFactory.searchStore] failed', error.message);
        }

        SearchFactory.searchStore(searchQuery, searchParams)
            .then(ObjectHelperFactory.getProperty(['games', 'game']))
            .then(setOfferData)
            .catch(errorHandler);
    }

    function originStoreAccessLandingVaultlist(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                header: '@',
                more: '@',
                less: '@',
                query: '@',
                facet: '@',
                sort: '@',
                dir: '@',
                rows: '@'
            },
            controller: OriginStoreAccessLandingVaultlistCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/landing/views/vaultlist.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessLandingVaultlist
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} header the main title
     * @param {LocalizedString} more the more button text
     * @param {LocalizedString} less the less button text
     * @param {String} query the search string
     * @param {String} facet facet group and facet ex: subscriptionGroup:vault-games
     * @param {SortEnumeration} sort the sort value to order the results by
     * @param {DirectionEnumeration} dir the sorting direction
     * @param {Number} rows the number of results to return
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-landing-vaultlist />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAccessLandingVaultlist', originStoreAccessLandingVaultlist);
}()); 
