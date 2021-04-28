(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-search-no-results';

    /**
     * Global Search Results Ctrl
     */
    function OriginSearchNoResultsCtrl($scope, $stateParams, UtilFactory, AuthFactory) {
        var localize = UtilFactory.getLocalizedStrGenerator(CONTEXT_NAMESPACE);

        /* Get Translated Strings */
        $scope.noResultsFoundTitleLoc = localize($scope.noResultsFoundTitleStr, 'noresultsfoundtitle');
        $scope.noResultsFoundDescLoc = localize($scope.noResultsFoundDescStr, 'noresultsfounddesc');
        $scope.gameCarouselTitleLoc = localize($scope.gameCarouselTitleStr, 'gamecarouseltitle');
        $scope.gameCarouselDescLoc = localize($scope.gameCarouselDescStr, 'gamecarouseldesc');

        $scope.searchString = decodeURI($stateParams.searchString);

        $scope.isOnline = AuthFactory.isClientOnline();
    }

    /**
     * Global Search Results Directive
     */
    function originSearchNoResults(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                noResultsFoundTitleStr: '@noresultsfoundtitle',
                noResultsFoundDescStr: '@noresultsfounddesc',
                gameCarouselTitleStr: '@gamecarouseltitle',
                gameCarouselDescStr: '@gamecarouseldesc'
            },
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('search/views/search-no-results.html'),
            controller: 'OriginSearchNoResultsCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSearchNoResults
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} noresultsfoundtitle "No results found for"
     * @param {LocalizedString} noresultsfounddesc "Make sure your words are spelled correctly, or use less or different keywords."
     * @param {LocalizedString} gamecarouseltitle "Popular Games"
     * @param {LocalizedString} gamecarouseldesc "Check out the most popular games people are searching for on Origin."
     * @description
     *
     * Global Search No Results Page.  Shows a carousel of popular games.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-search-no-results
     *             noresultsfoundtitle="No results found for"
      *            noresultsfounddesc="Make sure your words are spelled correctly, or use less or different keywords."
      *            gamecarouseltitle="Popular Games"
      *            gamecarouseldesc="Check out the most popular games people are searching for on Origin."
               ></origin-search-no-results>
     *     </file>
     * </example>
     */
    //declaration
    angular.module('origin-components')
        .controller('OriginSearchNoResultsCtrl', OriginSearchNoResultsCtrl)
        .directive('originSearchNoResults', originSearchNoResults);
}());