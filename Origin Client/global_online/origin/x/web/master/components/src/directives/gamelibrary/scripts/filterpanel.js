/**
 * @file filter/filterbar.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-gamelibrary-filters';

    function OriginGamelibraryFilterCtrl($scope, UtilFactory, GamesFilterFactory) {
        var getModel = GamesFilterFactory.getFilter;

        /**
         * Grabs localized label from the i18n module.
         * @param {string} id - string identifier as it's defined in the dictionary
         * @param {object} params - *optional* this is the optional list of parameters to substitute the string with.
         * @return {string}
         */
        function localizeLabel(id, parameters) {
            return UtilFactory.getLocalizedStr($scope[id], CONTEXT_NAMESPACE, id.toLowerCase(), parameters);
        }

        var filterLabels = {
            isPlayedLastWeek: localizeLabel('playedThisWeekLabel'),
            isFavorite: localizeLabel('favouritesLabel'),
            isDownloading: localizeLabel('downloadingLabel'),
            isHidden: localizeLabel('hiddenLabel'),
            isInstalled: localizeLabel('installedLabel'),
            isNonOrigin: localizeLabel('nonOriginLabel'),
            isSubscription: localizeLabel('subscriptionLabel'),
            isPcGame: localizeLabel('pcPlatformLabel'),
            isMacGame: localizeLabel('macPlatformLabel')
        };

        /**
         * Generates generic label for filters with string value (search by game name, for example)
         * @param {string} filterId - filter id
         * @return {string}
         */
        function getGenericFilterLabel(filterId) {
            var model = getModel(filterId);

            return localizeLabel('filterByLabel', {
                '%filterValue%': model.value
            });
        }

        /**
         * Returns label for the given filter ID
         * @param {string} filterId - filter id
         * @return {string}
         */
        $scope.getLabel = function (filterId) {
            // Guard condition to support filter bar that's also running on this controller
            if (filterId === undefined) {
                return '';
            }

            if (filterLabels.hasOwnProperty(filterId)) {
                return filterLabels[filterId];
            } else {
                return getGenericFilterLabel(filterId);
            }
        };

        /**
         * Returns true if there are any active filters currently on the panel.
         * @return {boolean}
         */
        $scope.hasActiveFilters = function () {
            return GamesFilterFactory.countActiveFilters() > 0;
        };

        /**
         * Toggles a filter off
         * @param {string} filterId - filter id
         * @return {string}
         */
        $scope.dismiss = function (filterId) {
            GamesFilterFactory.dismissFilter(filterId);
            $scope.onFilterChange();
        };

        /**
         * Wrapper function that allows passing the callback down to the child directives.
         * When doing expression binding (&) Angular wraps the expression (in our case
         * the callback function onChange) in a special context-bound object that we can
         * use as a proxy to execute the callback. In our case we need to pass that callback
         * down to another directive. If we simply pass the onChange scope variable to that
         * other directive, Angular would wrap it in 2 layes of proxy objects that be
         * troublesome to unwrap.
         *
         * @return {string}
         */
        $scope.onFilterChange = function () {
            $scope.onChange();
        };

        $scope.searchPlaceholder = localizeLabel('filterlibrarylabel');
        $scope.clearAllLabel = localizeLabel('clearAllFiltersLabel');

        $scope.filterIds = Object.keys(filterLabels);

        $scope.clearAll = GamesFilterFactory.dismissAll;
        $scope.getFilter = GamesFilterFactory.getFilter;
        $scope.isActive = GamesFilterFactory.isFilterActive;
    }

    function originGamelibraryFilterPanel(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                isVisible: '&',
                onChange: '&',
                clearAllFiltersLabel: '@clearallfilters',
                filterLibraryLabel: '@filterlibrary',
                favouritesLabel: '@favourites',
                hiddenLabel: '@hidden',
                playedThisWeekLabel: '@playedthisweek',
                downloadingLabel: '@downloading',
                installedLabel: '@installed',
                nonOriginLabel: '@nonorigin',
                subscriptionLabel: '@subscription'
            },
            controller: 'OriginGamelibraryFilterCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/filterpanel.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryFilterPanel
     * @restrict E
     * @element ANY
     * @scope
     * @param {function} isVisible functional expression indicating whether the panel is visible or not
     * @param {function} onChange function to call when any of the filters change
     * @param {LocalizedString} favourites 'favourites' filter item label
     * @param {LocalizedString} hidden 'hidden' filter item label
     * @param {LocalizedString} clearallfilters 'Clear all filters' link text
     * @param {LocalizedString} filterlibrary placeholder text for the filter panel's search field
     * @param {LocalizedString} playedthisweek 'Played this week' filter item label
     * @param {LocalizedString} downloading 'Downloading' filter item label
     * @param {LocalizedString} installed 'Installed' filter item label
     * @param {LocalizedString} nonorigin 'Non-Origin' filter item label
     * @param {LocalizedString} subscription 'Subscription' filter item label
     * @description
     *
     * Game library filter panel.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-filter-panel is-visible="isFilterPanelOpen" on-change="refreshView()"></origin-gamelibrary-filter-panel>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .controller('OriginGamelibraryFilterCtrl', OriginGamelibraryFilterCtrl)
        .directive('originGamelibraryFilterPanel', originGamelibraryFilterPanel);
}());
