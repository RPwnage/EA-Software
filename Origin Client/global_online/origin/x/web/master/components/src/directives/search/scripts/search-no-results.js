(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-global-search';

    /**
     * Global Search Results Ctrl
     */
    function OriginSearchNoResultsCtrl($scope, $stateParams, UtilFactory, ComponentsConfigFactory, AuthFactory) {

        /* Get Translated Strings */
        $scope.strings = {
            noResultsFoundTitle: UtilFactory.getLocalizedStr($scope.noResultsFoundTitle, CONTEXT_NAMESPACE, 'nosearchresultstitle'),
            noResultsFoundDesc:  UtilFactory.getLocalizedStr($scope.noResultsFoundDesc, CONTEXT_NAMESPACE, 'nosearchresultsdesc')
        };

        $scope.searchString = decodeURI($stateParams.searchString);

        $scope.noResultsImageSrc = AuthFactory.isClientOnline()? ComponentsConfigFactory.getImagePath('search\\temp-no-search-results.png') : '';
    }

    /**
     * Global Search Results Directive
     */
    function originSearchNoResults(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                noResultsFoundTitle: '@',
                noResultsFoundDesc: '@'
            },
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
     * @param {String} Search string 
     * @param {LocalizedString} noResultsFoundTitle "No results found for blahblah"
     * @param {LocalizedString} noResultsFoundDesc "Make sure your words are spelled correctly, or use less or different keywords."
     * @description
     *
     * Global Search No Results Page
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-search-results
     *             searchString="blahblah"
     *             noResultsFoundTitle="No results found for blahblah"
     *             noResultFoundDesc="Make sure your words are spelled correctly, or use less or different keywords."
     *         ></origin-search-results>
     *     </file>
     * </example>
     */
    //declaration
    angular.module('origin-components')
        .controller('OriginSearchNoResultsCtrl', OriginSearchNoResultsCtrl)
        .directive('originSearchNoResults', originSearchNoResults);
}());