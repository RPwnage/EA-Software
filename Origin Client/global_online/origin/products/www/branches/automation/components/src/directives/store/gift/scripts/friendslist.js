/**
 * @file store/gift/scripts/friendlist.js
 */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-gift-friendslist';

    function originStoreGiftFriendslistCtrl($scope, GiftingWizardFactory, GiftFriendListFactory, UtilFactory) {
        $scope.strings = {
            noResultsFoundHint: UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'no-results-found-hint'),
            searchInputHint: UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'search-input-hint'),
            noResultsError: UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'no-results-error-str'),
            nextStr: UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'next-str'),
            titleStr: UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'title-str'),
            disclaimer: UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'disclaimer')
        };

        var cachedFriendsList;
        var peopleResult;
        var selectedUser;
        var currentSearchId; //To synchronize searches, only the latest will be used

        $scope.searchKeyword = null;
        $scope.displayedUsers = [];
        $scope.isUserSelected = false;

        function digestScope() {
            $scope.$digest();
        }

        /**
         * Cache and paginate friends list.
         * @param friendList
         */
        function processFriendsList(friendList) {
            cachedFriendsList = friendList;
            cachedFriendsList.getCurrentPage()
                .then(setDisplayedUsers);


        }

        function handleError() {
            GiftFriendListFactory.selectUser({});
            $scope.displayedUsers = [];
            $scope.errorOccured = true;
            $scope.searchInProgress = false;
        }

        /**
         * Load logged in user's friends & cache them.
         */
        function loadFriends() {
            $scope.searchInProgress = true;
            $scope.errorOccured = false;
            GiftFriendListFactory.getFriendList()
                .then(processFriendsList) 
                .catch(handleError);
        }

        /**
         * Process people search results
         * @param searchResult
         */
        function processPeopleSearch(searchId, searchResult) {
            //Search is synchronized with search ID.
            //Only the latest friend search request will be used
            if (searchId === currentSearchId){
                peopleResult = searchResult;
                peopleResult.getNextPage()
                    .then(setDisplayedUsers);
            }

        }


        /**
         * Whether lazy loading should be disabled.
         * @returns {boolean} true if search is in progress or reach end of results.
         */
        $scope.lazyLoadDisabled = function () {
            if (!$scope.searchInProgress) {
                if ($scope.searchKeyword) { //people search
                    return !peopleResult.hasMore();
                } else { //friend search
                    return !cachedFriendsList.hasMore();
                }
            }
            return true;
        };

        function setDisplayedUsers(users) {
            $scope.displayedUsers = users;
            $scope.searchInProgress = false;
            digestScope();

            if (!users.length) {
                Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_GIFTING_NO_SEARCH_RESULT);
            }
        }

        /**
         * Load more results.
         * @returns {Promise}
         */
        $scope.loadMore = function () {
            $scope.searchInProgress = true;
            if ($scope.searchKeyword) {
                return peopleResult.getNextPage()
                    .then(setDisplayedUsers);
            } else {
                return cachedFriendsList.getNextPage()
                    .then(setDisplayedUsers);
            }

        };

        function setFocusOnSearchInput() {
            setTimeout(function() {
                angular.element('#filter-by-text').get(0).focus();
            }, 300);
            
        }

        /**
         * Search for users based on specified keyword
         */
        $scope.searchForKeyword = function () {
            if ($scope.searchKeyword) {
                //Track each search using search ID
                currentSearchId = UtilFactory.guid();
                $scope.strings.noResultsFound = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, 'no-results-found',
                    {'%searchkeyword%': $scope.searchKeyword});
                $scope.errorOccured = false;
                $scope.searchInProgress = true;
                
                GiftFriendListFactory.getSearchResults($scope.searchKeyword)
                    .then(_.partial(processPeopleSearch, currentSearchId))
                    .catch(handleError)
                    .then(digestScope);
            } else {
                $scope.resetSearchResults();
            }
        };

        /**
         * remove search results and display cached friend list
         */
        $scope.resetSearchResults = function () {
            $scope.searchKeyword = null;
            $scope.errorOccured = false;
            if (cachedFriendsList) {
                cachedFriendsList.getCurrentPage().then(function (friends) {
                    $scope.displayedUsers = friends;
                    $scope.$digest();
                });
            } else {
                loadFriends();
            }
            setFocusOnSearchInput();
        };

        /**
         * Navigate the user to write personal message page
         */
        $scope.onNext = function () {
            if ($scope.isUserSelected) {
                GiftingWizardFactory.continueToCustomizeMessageFlow($scope.offerId, selectedUser);
            }
        };

        /**
         * pass stored user data to next page.
         * @param userData
         */
        function onFriendSelect(userData) {
            selectedUser = _.get(userData, ['userInfo']);
            var eligibilityCallInProgress = _.get(userData, ['eligibilityCallInProgress'], false);
            $scope.isUserSelected = _.get(selectedUser, 'nucleusId') && !eligibilityCallInProgress;
        }

        function loadFriendsOrSearchResult() {
            setFocusOnSearchInput();
            if (_.get(selectedUser, 'username', null)) {
                $scope.searchKeyword = selectedUser.username;
                $scope.searchForKeyword();
            } else {
                loadFriends();
            }
        }

        GiftFriendListFactory.getSelectedUserObservable().attachToScope($scope, onFriendSelect);

        loadFriendsOrSearchResult();
    }

    function originStoreGiftFriendslist(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid'
            },
            controller: originStoreGiftFriendslistCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/gift/views/friendslist.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreGiftFriendslist
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {string} offerid desc
     * @param {LocalizedString} no-results-found-hint Suggested user actions when no results found.
     * @param {LocalizedString} no-results-found No results found message (Must include %searchkeyword% to display user's search keyword)
     * @param {LocalizedString} search-input-hint place holder text for search input
     * @param {LocalizedString} next-str Next button text
     * @param {LocalizedString} title-str Overlay title text
     * @param {LocalizedString} no-results-error-str error to display when services are down
     * @param {LocalizedString} disclaimer Help text to display in footer
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-gift-friendslist offerId="OFB-EAST:123456"></origin-store-gift-friendslist>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreGiftFriendslist', originStoreGiftFriendslist);
}());
