(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-search-results';

    /**
     * Global Search Results Ctrl
     */
    function OriginSearchResultsCtrl($scope, $state, $stateParams, AuthFactory, UtilFactory) {

        $scope.showViewMore = true;

        /* Get Translated Strings */
        $scope.gamesTitleLoc = UtilFactory.getLocalizedStr($scope.gamesTitle, CONTEXT_NAMESPACE, 'gamestitle');
        $scope.storeTitleLoc = UtilFactory.getLocalizedStr($scope.storeTitle, CONTEXT_NAMESPACE, 'storetitle');
        $scope.peopleTitleLoc = UtilFactory.getLocalizedStr($scope.peopleTitle, CONTEXT_NAMESPACE, 'peopletitle');
        $scope.gamesViewMoreLoc = UtilFactory.getLocalizedStr($scope.gamesViewMore, CONTEXT_NAMESPACE, 'gamesviewmore');
        $scope.storeViewMoreLoc = UtilFactory.getLocalizedStr($scope.storeViewMore, CONTEXT_NAMESPACE, 'storeviewmore');
        $scope.peopleViewMoreLoc = UtilFactory.getLocalizedStr($scope.peopleViewMore, CONTEXT_NAMESPACE, 'peopleviewmore');
        $scope.showViewMoreTextLoc = UtilFactory.getLocalizedStr($scope.showViewMoreText, CONTEXT_NAMESPACE, 'showviewmoretext');

        if (AuthFactory.isAppLoggedIn()) {
            $scope.showViewMore = false;
        }

        $scope.isShowingOnlyPeople = ($stateParams.category === 'people') ? true : false;

        /**
         * Go to Store Browse Page
         * @return {void}
         */
        $scope.goToStoreBrowse = function() {
            Origin.telemetry.sendTelemetryEvent('TRACKER_DEV', 'search_view_all', 'user', 'search_view_all', { 'category': 'store' });
            $state.go('app.store.wrapper.browse', {
                searchString: decodeURI($stateParams.searchString)
            });
        };

		/**
         * Redirects to games library search results
         * @return {void}
         * @method showAllGamesCallback
         */
        $scope.goToGamesLibrarySearchResults = function() {
            Origin.telemetry.sendTelemetryEvent('TRACKER_DEV', 'search_view_all', 'user', 'search_view_all', { 'category': 'games_library' });
            $state.go('app.game_gamelibrary', {searchString: $stateParams.searchString});
        };

    }

    /**
     * Global Search Results Directive
     */
    function originSearchResults(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                limit: '@',
                type: '@',
                results: '=',
                showAllPeopleCallback: '&',
                gamesTitle: '@gamestitle',
                storeTitle: '@storetitle',
                peopleTitle: '@peopletitle',
                gamesViewMore: '@gamesviewmore',
                storeViewMore: '@storeviewmore',
                peopleViewMore: '@peopleviewmore',
                showViewMoreText: '@showviewmoretext'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('search/views/search-results.html'),
            controller: 'OriginSearchResultsCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSearchResults
     * @restrict E
     * @element ANY
     * @scope
     *
     * @description
     * @param {number} limit - used by ng-repeat to stop at limit
     * @param {string} type - can be one of three 'games'/'store'/'people'. based on this we render results
     * @param {object} results - object representing the results for each type based on the search term
     * @param {function} show-all-people-callback - callback function to parent directive to show all people results and hide other results
     * @param {LocalizedString} gamestitle "Game Library Results"
     * @param {LocalizedString} storetitle "Store Results"
     * @param {LocalizedString} peopletitle "People Results"
     * @param {LocalizedString} gamesviewmore "View All in Game Library"
     * @param {LocalizedString} storeviewmore "View All in Store"
     * @param {LocalizedString} peopleviewmore "View All"
     * @param {LocalizedString} showviewmoretext "View More"
     * @description
     *
     * Global Search Results Page
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-search-results
     *             type="people"
     *             results="peopleResults"
     *             limit="4"
     *             gamestitle="Game Library Results"
     *             storetitle="Store Results"
     *             peopletitle="People Results"
     *             gamesviewmore="View All in Game Library"
     *             storeviewmore="View All in Store"
     *             peopleviewmore="View All"
     *             showviewmoretext="View More"
     *          ></origin-search-results>
     *     </file>
     * </example>
     */
    //declaration
    angular.module('origin-components')
        .controller('OriginSearchResultsCtrl', OriginSearchResultsCtrl)
        .directive('originSearchResults', originSearchResults);
}());