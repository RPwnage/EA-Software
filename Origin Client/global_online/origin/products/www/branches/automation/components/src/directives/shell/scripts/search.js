/**
 * @file shell/scripts/globalsearch.js
 */
(function () {
    'use strict';

    var isSearching = false,
        previousSearchString = '';

    var CONTEXT_NAMESPACE = 'origin-global-search',
        STATE_SEARCH = 'app.search',
        STATE_DEFAULT = 'app.home_home';

    /**
     * Global Search Ctrl
     */
    function OriginGlobalSearchCtrl($scope, $state, $stateParams, UtilFactory, AppCommFactory, LayoutStatesFactory, ScreenFactory) {

        var tid;

        $scope.isCompactView = ScreenFactory.isSmall();

        $scope.isUnderAge = function() {
            return Origin.user.underAge();
        };

        // determine if the element is focused or not
        $scope.focused = false;

        // Initialize the searchString
        $scope.searchString = '';

        $scope.searchPlaceholderLoc = UtilFactory.getLocalizedStr($scope.searchPlaceholderStr, CONTEXT_NAMESPACE, 'searchplaceholder');

        // If user directly comes to search page, setting the previous state as default state (/home)
        var previousState = {
            name : ($state.$current.name === STATE_SEARCH)? STATE_DEFAULT : $state.$current.name,
            params : ''
        };

        /**
        * We need a timeout because when you click clear it blurs
        * and then it refocuses on it
        * @method _onblur
        */
        function _onblur() {
            $scope.focused = false;
            $scope.$digest();
        }

        /**
         * Update search input box with search string - if the URL has search string
         * @method updateSearchBox
         */
        function updateSearchBox() {
            if(typeof($stateParams.searchString) !== 'undefined' && $state.$current.name === STATE_SEARCH){
                $scope.searchString = decodeURI($stateParams.searchString);
            }
        }

        /**
         * Update the previousState and resets search variables
         * @method handleStateChange
         */
         function handleStateChange(event, toState, toParams, fromState, fromParams) {
            updateSearchBox();

            // Store the previous state before search state.go called with params
            // ORPUB-2553 - only modify previousState if fromState is not empty
            if(fromState.name && fromState.name !== STATE_SEARCH){
                previousState.name = fromState.name;
                previousState.params = fromParams;
            }

            // Adding extra condition to make sure that this is called only when we change state from search view
            if(fromState.name === STATE_SEARCH && toState.name !== STATE_SEARCH) {
                isSearching = false;
                previousSearchString = $scope.searchString;
                $scope.searchString = '';
                $scope.onblur();
            }

            // ORPUB-1491 - clear the search box whenever the user changes tabs, in case the search box contains a single character.
            if(fromState.name !== STATE_SEARCH && toState.name !== STATE_SEARCH) {
                $scope.searchString = '';
                $scope.onblur();
            }
         }

        /**
         * Determine if the searchString contains at least 2 characters,
         * then go to Search Results
         * @method search
         */
        $scope.search = function () {

            var options = isSearching ? {location : 'replace'} : {};
            isSearching = true;

            if ($scope.searchString.length) {
                if ($scope.searchString !== previousSearchString) {
                    previousSearchString = $scope.searchString;
                    Origin.telemetry.sendTelemetryEvent('TRACKER_DEV', 'search_initiated', 'user', 'search_initiated', {
                        'searchString': decodeURI($scope.searchString),
                        'currentPage': $state.$current.name
                    });
                }
                AppCommFactory.events.fire('uiRouter:go', STATE_SEARCH, {
                    'searchString' : encodeURI($scope.searchString),
                    'category': null
                }, options);
                LayoutStatesFactory.setEmbeddedFilterpanel(false);
            } else if ($state.$current.name === STATE_SEARCH) {
                AppCommFactory.events.fire('uiRouter:go', previousState.name, previousState.params);
            }
        };

        /**
         * Determine if esc key is pressed then clear
         * the search term and go to previous state
         * @method escapeClearSearch
         */
        $scope.escapeClearSearch = function (keyCode) {

            if (keyCode === 27) {
                $scope.searchString = '';
                previousSearchString = $scope.searchString;

                // Only return to the previous state if we are currently in the search state,
                // to avoid changing tabs when clearing a single character.
                if ($state.$current.name === STATE_SEARCH) {
                    AppCommFactory.events.fire('uiRouter:go', previousState.name, previousState.params);
                }
            }

        };

        /**
         * Clears the search term and goto previous state
         * @method clear
         */
        $scope.clear = function() {
            $scope.searchString = '';
            previousSearchString = $scope.searchString;

            // Only return to the previous state if we are currently in the search state.
            // to avoid changing tabs when clearing a single character.
            if ($state.$current.name === STATE_SEARCH) {
                AppCommFactory.events.fire('uiRouter:go', previousState.name, previousState.params);
            }

            if($scope.isCompactView) {
                $scope.$emit('globalsearch:toggle', false);
                $scope.focused = false;
                $scope.$digest();
            }
        };

        /**
         * On focus of the element add a class to the parent
         * @method onfocus
         */
        $scope.onfocus = function() {
            $scope.focused = true;
            $scope.$emit('globalsearch:toggle', true);

            // clear out any blur if we are focused
            if (tid) {
                clearTimeout(tid);
                tid = null;
            }
        };

        /**
         * on blur of the element remove class from parent
         * @method onblur
         */
        $scope.onblur = function() {
            if ($scope.searchString.length) {
                return;
            }
            $scope.$emit('globalsearch:toggle', false);
            if (tid) {
                clearTimeout(tid);
            }
            tid = setTimeout(_onblur, 100);
        };

        //handle state change and reset search vars
        $scope.$on('$stateChangeSuccess', handleStateChange);

        //update search box in case user directly lands on search page
        updateSearchBox();

    }

    /**
     * Global Search Directive
     */
    function originGlobalSearch(ComponentsConfigFactory) {

        /**
        * The directive link
        */
        function originGlobalSearchLink(scope, elem) {
            /**
             * Set focus on the search box
             * @method focusOnSearch
             */
            
            var input = angular.element(elem[0].querySelector('.otkinput > input')),
                timeoutHandle;

            function focusOnSearch() {
                elem.find('input').focus();
            }

            function onFocus() {
                var self = this;
                timeoutHandle = setTimeout(function(){self.setSelectionRange(0, self.value.length);}, 1);
            }


            input.on('focus', onFocus);

            scope.submit = function () {
                try {
                    input.blur();
                } catch (err) {
                    // ignore errors
                }

                scope.search();
            };

            function onDestroy() {
                Origin.events.off(Origin.events.CLIENT_NAV_FOCUSONSEARCH, focusOnSearch);
                input.off('focus', onFocus);
                if (timeoutHandle) {
                    clearTimeout(timeoutHandle);
                }

            }

            scope.$on('$destroy', onDestroy);

            //listen for client emit to focus event (embedded only)
            Origin.events.on(Origin.events.CLIENT_NAV_FOCUSONSEARCH, focusOnSearch);
        }
        return {
            restrict: 'E',
            controller: 'OriginGlobalSearchCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/search.html'),
            scope: {
                includes: '@includes',
                excludes: '@excludes',
                source: '@source',
                searchPlaceholderStr: '@searchplaceholder'
            },
            link: originGlobalSearchLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGlobalSearch
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} searchplaceholder "Search Games and More"
     * @description
     *
     * Global Search
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-global-search></origin-global-search>
     *     </file>
     * </example>
     *
     */
    // declaration
    angular.module('origin-components')
        .controller('OriginGlobalSearchCtrl', OriginGlobalSearchCtrl)
        .directive('originGlobalSearch', originGlobalSearch);

}());
