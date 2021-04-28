/**
 * @file filter/filtermultiple.js
 */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-gamelibrary-filter-panel'; //need to tap into parent's controller for translations
    var FILTER_APPLIED_EVENT = 'OriginGamelibraryLoggedin:filterApplied';

    function originFilterPillMultiple(ComponentsConfigFactory, AppCommFactory, UtilFactory) {

        function originFilterPillMultipleLink(scope) {
            var filterEventHandle = null;

            var moreThanOneFilterLabel = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'morethanonefilterlabel', {});

            /**
             * Create a filter model
             * @param filterId
             * @returns filter object
             */
            function createFilter(filterId) {

                return {
                    id:  filterId,
                    filterIds: [filterId],
                    label: scope.getLabel()(filterId)
                };
            }

            /**
             * Creates a filter out of multiple filters with label +3 active filters
             * @param filterIds
             * @param activeCount
             * @returns {{filterIds: *, label: string}}
             */
            function createAggregateFilter(filterIds, activeCount) {

                return {
                    id:  filterIds.join(''),
                    filterIds: filterIds,
                    label: moreThanOneFilterLabel.replace('%activeCount%', (activeCount - 1))
                };
            }

            /**
             * Determines how filters are presented to user.
             *  if filterBy text is active -> all other active filters are aggregated into one filter object
             *  if filterBy text is inactive -> first filter is displayed as is, rest are aggregated into one filter object
             *
             * @param isFilterbyTextActive
             * @param activeFilters
             * @param activeCount
             */
            function createFilters(isFilterbyTextActive, activeFilters, activeCount) {
                var filters = [];
                var numActiveFilters = activeFilters.length;
                if (!isFilterbyTextActive) {

                    if (numActiveFilters > 2) {
                        filters.push(createFilter(activeFilters[0]));
                        filters.push(createAggregateFilter(activeFilters.slice(1), activeCount));
                    } else {
                        _.forEach(activeFilters, function(activeFilter) {
                            filters.push(createFilter(activeFilter));
                        });
                    }

                } else if (isFilterbyTextActive) {

                    if (numActiveFilters === 1) {
                        filters.push(createFilter(activeFilters[0]));
                    }

                    if (numActiveFilters > 1) {
                        filters.push(createAggregateFilter(activeFilters, activeCount));
                    }
                }

                scope.filters = filters;
            }

            /**
             * Count number of active filters.
             */
            function evaluateActiveFilters() {

                var activeFilters = [];
                var activeCount = 0;
                var isFilterbyTextActive = scope.isActive()('displayName');

                if (isFilterbyTextActive) {
                    activeCount++;
                }

                _.forEach(scope.filterIds, function(filterId) {
                    if (scope.isActive()(filterId)) {
                        activeFilters.push(filterId);
                        activeCount++;
                    }
                });

                createFilters(isFilterbyTextActive, activeFilters, activeCount);
            }

            /**
             * dismiss one or many filters.
             * @param filter
             */
            scope.dismissFilters = function (filter) {
                _.forEach(filter.filterIds,  scope.onDismiss());
            };

            evaluateActiveFilters();

            //reevaluate filters when model is updated.
            filterEventHandle = AppCommFactory.events.on(FILTER_APPLIED_EVENT, evaluateActiveFilters);

            scope.$on('$destroy', function() {
                if (filterEventHandle) {
                    filterEventHandle.detach();
                }
            });

        }

        return {
            restrict: 'E',
            scope: {
                getLabel: '&',
                isActive: '&',
                onDismiss: '&',
                filterIds: '='
            },
            link: originFilterPillMultipleLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('filter/views/filtermultiple.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originFilterPillMultiple
     * @restrict E
     * @element ANY
     * @scope

     * @description
     *
     * Dismissable active filter bubble. This directive is a partial not directly merchandised.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-filterbar>
     *             <origin-filter-pill-multiple label="Filter by name" is-active="myFilter.active" on-dismiss="dismissFilter()"></origin-filter-pill-multiple>
     *         </origin-filterbar>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .directive('originFilterPillMultiple', originFilterPillMultiple);
}());