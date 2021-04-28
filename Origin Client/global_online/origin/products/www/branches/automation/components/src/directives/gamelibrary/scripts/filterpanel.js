/**
 * @file filter/filterpanel.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-gamelibrary-filter-panel';

    function OriginGamelibraryFilterCtrl($scope, UtilFactory, GamesFilterFactory, EventsHelperFactory, AuthFactory) {
        var getModel = GamesFilterFactory.getFilter,
            handles = [];

        /**
         * Grabs localized label from the i18n module.
         * @param {string} id - string identifier as it's defined in the dictionary
         * @param {object} parameters - *optional* this is the optional list of parameters to substitute the string with.
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
            isPurchasedGame: localizeLabel('purchasedGamesLabel'),
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
            if (angular.isUndefined(filterId)) {
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

        $scope.searchPlaceholder = localizeLabel('filterLibraryLabel');
        $scope.clearAllLabel = localizeLabel('clearAllFiltersLabel');
        $scope.platformLabel = localizeLabel('platformLabel');

        $scope.filterIds = Object.keys(filterLabels);

        $scope.clearAll = GamesFilterFactory.dismissAll;
        $scope.getFilter = GamesFilterFactory.getFilter;
        $scope.isActive = GamesFilterFactory.isFilterActive;

        function destroy() {
            EventsHelperFactory.detachAll(handles);
        }
        $scope.$on('$destroy', destroy);

        function onClientOnlineStateChanged(onlineObj) {
            // If we go offline remove the hidden filter
            if (onlineObj && !onlineObj.isOnline) {
                $scope.dismiss('isHidden');
            }
        }

        handles[handles.length] = AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
    }

    function originGamelibraryFilterPanel(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                isVisible: '&',
                onChange: '&',
                clearAllFiltersLabel: '@clearallfilterslabel',
                filterLibraryLabel: '@filterlibrarylabel',
                favouritesLabel: '@favouriteslabel',
                hiddenLabel: '@hiddenlabel',
                playedThisWeekLabel: '@playedthisweeklabel',
                downloadingLabel: '@downloadinglabel',
                installedLabel: '@installedlabel',
                nonOriginLabel: '@nonoriginlabel',
                subscriptionLabel: '@subscriptionlabel',
                platformLabel: '@platformlabel'
            },
            controller: 'OriginGamelibraryFilterCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/filterpanel.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryFilterPanel
     * @restrict E
     * @element ANY
     * @scope
     * @param {function} is-visible functional expression indicating whether the panel is visible or not
     * @param {function} on-change function to call when any of the filters change
     * @param {LocalizedString} filterbylabel * (merchandized in defaults) "Filter by: %filterValue%",
     * @param {LocalizedString} favouriteslabel * (merchandized in defaults) "Favorites",
     * @param {LocalizedString} hiddenlabel * (merchandized in defaults) "Show Hidden",
     * @param {LocalizedString} clearallfilterslabel * (merchandized in defaults) "Clear all filters",
     * @param {LocalizedString} filterlibrarylabel * (merchandized in defaults) "Filter Library",
     * @param {LocalizedString} playedthisweeklabel * (merchandized in defaults) "Played this week",
     * @param {LocalizedString} downloadinglabel * (merchandized in defaults) "Downloading",
     * @param {LocalizedString} installedlabel * (merchandized in defaults) "Installed",
     * @param {LocalizedString} nonoriginlabel * (merchandized in defaults) "Non-Origin games",
     * @param {LocalizedString} purchasedgameslabel * (merchandized in defaults) "Purchased games",
     * @param {LocalizedString} subscriptionlabel * (merchandized in defaults) "Subscription",
     * @param {LocalizedString} pcplatformlabel * (merchandized in defaults) "PC",
     * @param {LocalizedString} macplatformlabel * (merchandized in defaults) "Mac"
     * @param {LocalizedString} morethanonefilterlabel * (merchandized in defaults) more thatn one filter is active text (should be  "+ %activeCount% other filters") which translates to +2 other filters
     * @param {LocalizedString} platformlabel * the platform subheading label
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
