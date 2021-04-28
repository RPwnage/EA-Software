(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-global-search';

    /**
     * Global Search Results Ctrl
     */
    function OriginSearchResultsCtrl($scope, UtilFactory) {

        /* Get Translated Strings */
        $scope.strings = {
            gamesTitle: UtilFactory.getLocalizedStr($scope.gamesTitle, CONTEXT_NAMESPACE, 'gamestitle'),
            storeTitle: UtilFactory.getLocalizedStr($scope.storeTitle, CONTEXT_NAMESPACE, 'storetitle'),
            peopleTitle: UtilFactory.getLocalizedStr($scope.peopleTitle, CONTEXT_NAMESPACE, 'peopletitle'),
            gamesViewMore: UtilFactory.getLocalizedStr($scope.gamesViewMore, CONTEXT_NAMESPACE, 'gamesviewmore'),
            storeViewMore: UtilFactory.getLocalizedStr($scope.storeViewMore, CONTEXT_NAMESPACE, 'storeviewmore'),
            peopleViewMore: UtilFactory.getLocalizedStr($scope.peopleViewMore, CONTEXT_NAMESPACE, 'peopleviewmore')
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
                gamesTitle: '@',
                storeTitle: '@',
                peopleTitle: '@',
                gamesViewMore: '@',
                storeViewMore: '@',
                peopleviewmore: '@'                
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
     * @param {limit} used by ng-repeat to stop at limit
     * @param {type} type can be one of three 'games'/'store'/'people'. based on this we render results
     * @param {results} results object representing the results for each type based on the search term
     * @param {showAllPeopleCallback} callback function to parent directive to show all people results and hide other results   
     * @param {LocalizedString} gamesTitle "Game Library Results"
     * @param {LocalizedString} storeTitle "Store Results"
     * @param {LocalizedString} peopleTitle "People Results"
     * @param {LocalizedString} gamesViewMore "View All in Game Library"
     * @param {LocalizedString} storeViewMore "View All in Store"
     * @param {LocalizedString} peopleViewMore "View All"
     * @description
     * 
     * GLobal Search Results Page
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-search-results 
     *             type="people" 
     *             results="peopleResults" 
     *             limit="4"
     *             gamesTitle="Game Library Results"
     *             storeTitle="Store Results"
     *             peopleTitle="People Results"
     *             gamesViewMore="View All in Game Library"
     *             storeViewMore="View All in Store"
     *             peopleViewMore="View All"
     *          ></origin-search-results>
     *     </file>
     * </example>
     */
    //declaration
    angular.module('origin-components')
        .controller('OriginSearchResultsCtrl', OriginSearchResultsCtrl)
        .directive('originSearchResults', originSearchResults);
}());