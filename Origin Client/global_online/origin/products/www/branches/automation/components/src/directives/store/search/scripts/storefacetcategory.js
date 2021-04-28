/**
 * @file filter/filtersubmenu.js
 */
(function () {
    'use strict';

    var CategoryEnumeration = {
        "Genre": "genre",
        "Price": "price",
        "Franchise": "franchise",
        "Game Type": "gameType",
        "Platform": "platform",
        "Availability": "availability",
        "Players": "players",
        "Rating": "rating",
        "Developer": "developer",
        "Origin Access": "subscriptionGroup",
        "Language": "language"
    };
    var DEFAULT_OPEN_CATEGORIES = ['genre', 'price'];
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    var FILTER_LIST_CLASS = '.origin-filter-list';

    function OriginStoreFacetCategoryCtrl($scope, StoreSearchHelperFactory, AppCommFactory) {
        $scope.hasSelectableFilters = true;
        $scope.categoryExpanded = false;

        /**
         * Update this facet category's open/closed state when a new search has been performed
         * @param {boolean} triggeredByLazyLoad If the search was only a "pagination" search triggered by a lazy-load/scroll
         */
        function checkFlags(triggeredByLazyLoad) {
            //then don't check in case user is looking at facets accidentally triggers infinite scroll search.
            $scope.hasSelectableFilters = StoreSearchHelperFactory.hasSelectableFilters($scope.categoryId);
            
            if (!triggeredByLazyLoad) {
                $scope.categoryExpanded = (DEFAULT_OPEN_CATEGORIES.indexOf($scope.categoryId) >= 0 || StoreSearchHelperFactory.hasActiveFilters($scope.categoryId));
            }
        }

        function onDestroy() {
            AppCommFactory.events.off('storesearch:completed', checkFlags);
        }

        /**
         * Toggles open/closed state.
         * @return {void}
         */
        $scope.toggle = function () {
            $scope.categoryExpanded = !$scope.categoryExpanded;
        };

        $scope.isVisible = function () {
            return true;
        };

        AppCommFactory.events.on('storesearch:completed', checkFlags);

        $scope.$on('$destroy', onDestroy);
    }

    function originStoreFacetCategory(ComponentsConfigFactory, LocaleFactory) {
        function originStoreFacetCategoryLink(scope, element) {
            // get the value to sort this child element by
            function getNodeValue(childElement) {
                if(scope.categoryId === CategoryEnumeration.Language) {
                    return LocaleFactory.getLanguage(_.get(childElement, ['attributes', 'facet-id', 'nodeValue'], ''));
                } else {
                    return _.get(childElement, ['attributes', 'label', 'nodeValue'], '');
                }
            }

            // sort this facet group by its childrens facet-id if option is selected
            if(scope.alphaSort === BooleanEnumeration.true) {
                var facetListItem = element.find(FILTER_LIST_CLASS),
                    facetListItems = facetListItem.children();

                facetListItems.sort(function(a, b) {
                    return getNodeValue(a).localeCompare(getNodeValue(b), Origin.locale.languageCode());
                });

                if(_.size(facetListItems) > 0) {
                    facetListItem.html(facetListItems);
                }
            }
        }

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                label: '@',
                categoryId: '@',
                alphaSort: '@'
            },
            controller: 'OriginStoreFacetCategoryCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/search/views/storefacetcategory.html'),
            link: originStoreFacetCategoryLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreFacetCategory
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} label of the sub menu
     * @param {CategoryEnumeration} category-id Type of this category. Must be an enum
     * @param {BooleanEnumeration} alpha-sort Check to sort this facet group alphabetically
     * @description
     *
     * Filter panel item that can be toggled on and off.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-facet-category alpha-sort="true" label="Genre" categoryId="Genre"></origin-store-facet-category>
     *     </file>
     * </example>
     *
     */

        // directive declaration
    angular.module('origin-components')
        .controller('OriginStoreFacetCategoryCtrl', OriginStoreFacetCategoryCtrl)
        .directive('originStoreFacetCategory', originStoreFacetCategory);
}());
