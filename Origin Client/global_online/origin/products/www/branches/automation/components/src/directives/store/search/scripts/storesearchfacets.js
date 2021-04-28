/**
 * @file store/storesearchfacets.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-search-facets';

    function originStoreSearchFacets(ComponentsConfigFactory, UtilFactory, LayoutStatesFactory, ScreenFactory, AnimateFactory) {

        function originStoreSearchFacetsLink(scope) {
            scope.sortFilterString = UtilFactory.getLocalizedStr(scope.sortFilterStr, CONTEXT_NAMESPACE, 'sort-filter-str');

            function setEmbeddedFilterpanelState() {
                LayoutStatesFactory.setEmbeddedFilterpanel(!ScreenFactory.isSmall() );
            }

            /*
            * Create observaberlabblerblerabler
            */
            LayoutStatesFactory
                .getObserver()
                .attachToScope(scope, 'states');

            setEmbeddedFilterpanelState();
            AnimateFactory.addResizeEventHandler(scope, setEmbeddedFilterpanelState, 300);


            scope.toggleFilterPanel = function () {
                LayoutStatesFactory.toggleEmbeddedFilterpanel();
            };
        }

        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {
                sortFilterStr: '@'
            },
            link: originStoreSearchFacetsLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/search/views/storesearchfacets.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreSearchFacets
     * @restrict E
     * @element ANY
     * @scope
     *
     *
     * @param {LocalizedString} sort-filter-str Sort/Filter override
     *
     * @description Creates a list of facets defined in StoreSearchFactory to be used for narrowing down results in search/browse page
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-search-facets sort-filter-str="Sort"></origin-store-search-facets>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originStoreSearchFacets', originStoreSearchFacets);

}());
