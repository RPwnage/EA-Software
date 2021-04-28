/**
 * @file store/scripts/browse-noresult.js
 */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-browse-noresult';

    function originStoreBrowseNoresult(ComponentsConfigFactory, UtilFactory, StoreSearchHelperFactory) {

        function originStoreBrowseNoresultLink(scope) {
            scope.noResultFoundStr =  UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'no-results-found');
            scope.noResultFoundLinkStr =  UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'no-result-found-link-label');

            scope.onButtonClick = function() {
                StoreSearchHelperFactory.dismissAllFacets();
                StoreSearchHelperFactory.reloadCurrentPage().then(StoreSearchHelperFactory.triggerSearch);
            };
        }

        return {
            restrict: 'E',
            scope: {},
            link: originStoreBrowseNoresultLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/browse-noresult.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreBrowseNoresult
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {LocalizedString} no-results-found No results found, possibly because of invalid facet/sort option or search service is down
     * @param {LocalizedString} no-result-found-link-label Link text
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-browse-noresult></origin-store-browse-noresult>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreBrowseNoresult', originStoreBrowseNoresult);
}());
