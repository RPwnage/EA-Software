/**
 * @file store/scripts/storefacetoption.js
 */
(function () {
    'use strict';

    function originStoreFacetOption(ComponentsConfigFactory, StoreSearchHelperFactory, StoreFacetFactory, AppCommFactory, EventsHelperFactory, LocaleFactory, StoreSearchQueryFactory, NavigationFactory) {
        function originStoreFacetOptionLink(scope) {
            scope.count = 0;
            scope.countVisible = true;
            var isEnabled = true;


            StoreFacetFactory.registerFacetOption(scope.categoryId, scope.facetId);

            /**
             * Determines whether the facet is selected or not.
            */
            function setActiveStatus() {
                scope.isActive = StoreSearchHelperFactory.isFilterActive(scope.categoryId, scope.facetId);
                var filter = StoreSearchHelperFactory.getFilter(scope.categoryId, scope.facetId);

                if (filter) {
                    scope.count = parseInt(filter.count, 10);
                }
                populateLink();
            }

            function setEnableStatus(facetOverrideIds) {
                var filterCategory;
                //Don't enable if facet ID belongs to one of the presets
                isEnabled = facetOverrideIds.indexOf(scope.facetId) < 0;

                //Set the filter visibility so categories can track their filters
                if (scope.categoryId) {
                    filterCategory = StoreFacetFactory.getFacets()[scope.categoryId];
                    if (filterCategory) {
                        filterCategory.setFilterVisibility(scope.facetId, isEnabled);
                    }
                }
            }

            /**
             * Server does not provide translation for language facets. for 10.0, we take care of translation
             * We might revisit this later.
             */
            function setLabel() {
                if (!scope.label && scope.categoryId === 'language') {
                    scope.label = LocaleFactory.getLanguage(scope.facetId);
                }
            }

            scope.enabled = function () {
                return isEnabled && (scope.isActive || scope.count > 0);
            };

            /**
             * Toggles active/inactive state. Doesn't affect disabled items
             * @return {void}
             */
            scope.toggle = function ($event) {
                $event.preventDefault();
                if (scope.enabled()) {
                    scope.isActive = !scope.isActive;
                    if (scope.categoryId) {
                        StoreSearchHelperFactory.applyFilter(scope.categoryId, scope.facetId);
                    }
                }
            };

            var handles = [
                AppCommFactory.events.on('storesearch:completed', setActiveStatus),
                AppCommFactory.events.on('storeBundle:hideFacet', setEnableStatus)
            ];

            function onDestroy() {
                EventsHelperFactory.detachAll(handles);
            }
            
             function populateLink(){
                 if (scope.isActive){
                     scope.filterUrl = NavigationFactory.getAbsoluteUrl(StoreSearchQueryFactory.getUrlWithoutFacet(scope.categoryId, scope.facetId));
                 }else{
                     scope.filterUrl = NavigationFactory.getAbsoluteUrl(StoreSearchQueryFactory.getUrlWithNewFacet(scope.categoryId, scope.facetId));
                 }
            }
            
            function init() {

                EventsHelperFactory.attachAll(handles);

                setLabel();

                populateLink();

                scope.$on('$destroy', onDestroy);
            }

            init();
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                label: '@',
                facetId: '@',
                categoryId: '@'
            },
            link: originStoreFacetOptionLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/search/views/storefacetoption.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreFacetOption
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} label of the facet option
     * @param {string} facet-id Type of this facet. Must be a solr facet key
     * @param {string} category-id Type of this facet group. Must be a solr category key
     * @description
     *
     * Filter panel item that can be toggled on and off.
     *
     * Requires parent directive originStoreFacetCategory
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-facet-category label="Genre" category-id="genre">
     *           <origin-store-facet-option label="Action" facet-id="action"></origin-store-facet-option>
     *         </origin-store-facet-category>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originStoreFacetOption', originStoreFacetOption);
}());

