/**
 * @file shell/scripts/globalsearch.js
 */
(function () {
    'use strict';

    var isSearching = false;

    var CONTEXT_NAMESPACE = 'origin-global-search',
        STATE_SEARCH = 'app.search',
        STATE_DEFAULT = 'app.home_home';

    /**
     * Global Search Ctrl
     */
    function OriginGlobalSearchCtrl($scope, $state, $stateParams, UtilFactory) {

        var tid;

        // determien if the element is focused or not
        $scope.focused = false;

        // Register to the search providers. Move this to some other location, and read the configuration from a data source.
        $scope.searchPlaceholder = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'searchplaceholder');

        // If user directly comes to search page, setting the previous state as default state (/home)
        var previousState = {
            name : ($state.$current.name === STATE_SEARCH)? STATE_DEFAULT : $state.$current.name,
            params : ''
        };

        /**
        * We need a timeout because when you click clear it blurs
        * and then it refocuses on it
        * @return {void}
        * @method _onblur
        */
        function _onblur() {
            $scope.focused = false;
            $scope.$digest();
        }

        /**
         * Update search input box with search string - if the URL has search string
         * @return {void}
         * @method updateSearchBox
         */
        function updateSearchBox() {
            if(typeof($stateParams.searchString) !== 'undefined'){
                $scope.searchString = decodeURI($stateParams.searchString);
            }
        }

        /**
         * Update the previousState and resets search variables
         * @return {void}
         * @method handleStateChange
         */
         function handleStateChange(event, toState, toParams, fromState, fromParams) {

            updateSearchBox();

            // Store the previous state before search state.go called with params
            if(fromState.name !== STATE_SEARCH){
                previousState.name = fromState.name;
                previousState.params = fromParams;
            }

            // Adding extra condition to make sure that this is called only when we change state from search view
            if(fromState.name === STATE_SEARCH && toState.name !== STATE_SEARCH) {
                isSearching = false;
                // Do not clear the search box text if there is only 1 character.  That way, if there are 2 characters
                // in the search box, and 1 character is deleted, then we change from a search state to a non-search state,
                // but we do not automatically delete the 1 remaining character.
                if($scope.searchString.length !== 1) {
                    $scope.searchString = '';
                }
            }

            // ORPUB-1491 - clear the search box whenever the user changes tabs, in case the search box contains a single character.
            if(fromState.name !== STATE_SEARCH && toState.name !== STATE_SEARCH) {
                $scope.searchString = '';
            }

         }

        /**
         * Determine if the searchString contains at least 2 characters,
         * then go to Search Results
         * @return {void}
         * @method search
         */
        $scope.search = function () {

            var options = isSearching ? {location : 'replace'} : {};
            isSearching = true;

            // If there is more than one character in the search string, then we need to be in the search state.
            // If there are zero characters or one character in the search string *AND* we are currently in the search state,
            // then we stop showing search results and return to the previous state, ORPUB-1490.
            if ($scope.searchString.length > 1) {
                $state.go(STATE_SEARCH, {'searchString' : encodeURI($scope.searchString)}, options);
            } else if ($state.$current.name === STATE_SEARCH) {
                $state.go(previousState.name, previousState.params);
            }
        };

        /**
         * Determine if esc key is pressed then clear
         * the search term and go to previous state
         * @return {void}
         * @method escapeClearSearch
         */
        $scope.escapeClearSearch = function (keyCode) {

            if (keyCode === 27) {
                $scope.searchString = '';

                // Only return to the previous state if we are currently in the search state,
                // to avoid changing tabs when clearing a single character.
                if ($state.$current.name === STATE_SEARCH) {
                    $state.go(previousState.name, previousState.params);
                }
            }

        };

        /**
         * Clears the search term and goto previous state
         * @return {void}
         * @method clear
         */
        $scope.clear = function() {
            $scope.searchString = '';
            // Only return to the previous state if we are currently in the search state.
            // to avoid changing tabs when clearing a single character.
            if ($state.$current.name === STATE_SEARCH) {
                $state.go(previousState.name, previousState.params);
            }
        };

        /**
         * On focus of the element add a class to the parent
         * @return {void}
         * @method onfocus
         */
        $scope.onfocus = function() {
            $scope.focused = true;

            // clear out any blur if we are focused
            if (tid) {
                clearTimeout(tid);
                tid = null;
            }
        };

        /**
         * on blur of the element remove class from parent
         * @return {void}
         * @method onblur
         */
        $scope.onblur = function() {
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

            $(elem).on('click', '.origin-search-input', function() {
                $(this).select();
            });

        }
        return {
            restrict: 'E',
            controller: 'OriginGlobalSearchCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/search.html'),
            scope: {
                includes: '@includes',
                excludes: '@excludes',
                source: '@source'
            },
            link: originGlobalSearchLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellGlobalSearch
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * global search
     *
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
