

/**
 * @file store/browse.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-browse',
        RESULTS_PER_PAGE = 30,
        NO_INDEX_FOLLOW = 'noindex,nofollow',
        NO_INDEX_NO_FOLLOW_FACETS_COUNT = 2;

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    function OriginStoreBrowseCtrl($scope, StoreSearchQueryFactory, UtilFactory, StoreSearchFactory, AppCommFactory, LayoutStatesFactory, PageThinkingFactory, StoreSearchHelperFactory, OriginSeoFactory) {

        $scope.games = [];
        $scope.isLoading = true;
        var queryString = null,
            moreGamesAvailable = false,
            searchOffset = 0;

        $scope.facebookPageTitle =  UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'title-str');

        function triggerSearch() {
            $scope.games = [];
            PageThinkingFactory.blockUIPromise()
                .then($scope.search)
                .then(PageThinkingFactory.unblockUIPromise);
        }

        function updateGamesData(searchResponse, resetScroll) {
            $scope.isLoading = false;
            if (searchResponse.games) {
                if ($scope.games.length) {
                    $scope.games = $scope.games.concat(searchResponse.games);
                } else {
                    $scope.games = searchResponse.games;
                }
            }
            if (!queryString) {
                $scope.pageTitle = $scope.facebookPageTitle;
            } else {
                $scope.pageTitle = UtilFactory.getLocalizedStr($scope.resultsFound, CONTEXT_NAMESPACE, 'results-found', {
                    '%totalResults%': searchResponse.totalResults,
                    '%queryString%': queryString
                });
            }

            searchOffset = $scope.games.length;
            moreGamesAvailable = searchOffset < searchResponse.totalResults;
            $scope.$digest();
            if (resetScroll){
                angular.element(window).scrollTop(0);
            }
        }

        /**
         * If more than 1 facets are selected, we will apply an no index and no follow rule for meta tag robots
         * 
         */
        function checkAndApplySeoIndexationRule(){
            OriginSeoFactory.unsetRobotsRule();
            if (StoreSearchHelperFactory.getActiveFacetsCount() >= NO_INDEX_NO_FOLLOW_FACETS_COUNT){
                OriginSeoFactory.setRobotsRule(NO_INDEX_FOLLOW);
            }
        }

        /**
         * Runs search query to grab the next page of results if there are more available
         * @return {promise} The search promise
         */
        $scope.loadMore = function() {
            $scope.isLoading = true;
            return setParamAndSearch()
                .then(_.partialRight(updateGamesData))
                .catch(handleError);
        };

        /**
         * Setting flag to enable lazy-load directive to fire pagination queries
         * @return {boolean}
         */
        $scope.lazyLoadDisabled = function() {
            return $scope.isLoading || !moreGamesAvailable;
        };

        function setParamAndSearch(){
            var storeSearchInstance = setSearchParams();
            checkAndApplySeoIndexationRule();
            return storeSearchInstance.search();
        }

        /**
         * Runs search query to grab a page of results
         */
        $scope.search = function() {
            searchOffset = 0;

            return setParamAndSearch()
                .then(_.partialRight(updateGamesData, true))
                .catch(handleError);
        };

        function handleError() {
            $scope.isLoading = false;
            $scope.searchRequestFailed = true;
            resetSavedSortString();
            $scope.$digest();
        }

        function getUserNamespace(namespace){
            return _.compact([namespace, Origin.user.userPid()]).join('_');
        }

        function getSavedSortString(){
            var userNamespace = getUserNamespace(CONTEXT_NAMESPACE);
            var sortString = StoreSearchQueryFactory.getSortQueryParam() || sessionStorage.getItem(userNamespace) || '';
            if (sortString){
                //Setting to the new sort string
                sessionStorage.setItem(userNamespace, sortString);
            }
            return sortString;
        }

        function resetSavedSortString() {
            var userNamespace = getUserNamespace(CONTEXT_NAMESPACE);
            var sortString = sessionStorage.getItem(userNamespace);
            if (sortString){
                //Setting to the new sort string
                sessionStorage.removeItem(userNamespace);
            }
        }

        function setSearchParams() {
            $scope.isLoading = true;
            $scope.searchRequestFailed = false;
            $scope.$digest(); //make sure view is updated so we don't see no result message
            queryString = StoreSearchQueryFactory.getSearchStringQueryParam();
            return StoreSearchFactory
                .reset()
                .setSearchString(queryString)
                .setFacetOverride($scope.filterQuery)
                .setFacets(StoreSearchQueryFactory.getFacetsQueryParam())
                .setSortBy(getSavedSortString())
                .setCustomSort($scope.customRank)
                .setLimit(RESULTS_PER_PAGE)
                .setOffset(searchOffset);
        }

        AppCommFactory.events.on('storeSearchHelper:triggerSearch', triggerSearch);

        function onDestroy() {
            AppCommFactory.events.off('storeSearchHelper:triggerSearch', triggerSearch);
            OriginSeoFactory.unsetRobotsRule();
            LayoutStatesFactory.setEmbeddedFilterpanel(false);
        }

        $scope.$on('$destroy', onDestroy);

    }

    function originStoreBrowse($timeout, ComponentsConfigFactory, DirectiveScope) {
        function originStoreBrowseLink(scope){
            // Get initial list of games upon load
            $timeout(scope.search, 0);

            DirectiveScope.populateScope(scope, CONTEXT_NAMESPACE).then(function() {
                scope.isFacebookHidden = scope.hideFacebook === BooleanEnumeration.true;
            });
        }
        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {
                titleStr: '@',
                hideFacebook: '@',
                resultsFound: '@',
                noResultFound: '@'
            },
            controller: 'OriginStoreBrowseCtrl',
            link: originStoreBrowseLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/browse.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreBrowse
     * @restrict E
     * @element ANY
     * @scope
     *
     * @param {LocalizedString} title-str title text. Set up once as default. E.G., Browse Games
     * @param {BooleanEnumeration} hide-facebook Hide facebook share icon
     * @param {LocalizedString} results-found results found text. Set up once as default. E.G., "%totalResults% results found for \"%queryString%\""
     *
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
