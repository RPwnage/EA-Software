/**
 *  * @file models/giftfriendlist.js
 */
(function () {
    'use strict';

    var FRIENDS_ROSTER = 'FRIENDS';
    var PAGE_SIZE = 10;
    var INELIGIBLE_STATUS_ARRAY = ['already_owned', 'not_eligible', 'ineligible_to_receive', 'unknown'];

    function GiftFriendListFactory(RosterDataFactory, SearchFactory, ObservableFactory, UserDataFactory, ObserverFactory, UtilFactory, AppCommFactory) {

        var cachedFriendsList = [];
        var peopleResult = [];
        var selectedUser = ObservableFactory.observe({});

        /**
         * Break the result into pages
         * @param list
         * @param pageSize
         */
        function paginateResult(list, pageSize) {
            if (list.length > pageSize) {
                return _.slice(list, 0, pageSize);
            } else {
                return list;
            }
        }

        /**
         * Determines gifting eligibility of given list of friends for a game
         * @param {string} offerId
         * @param {Array} nucleusIds list of nucleusIds
         */
        function getFriendsGiftingEligibility(offerId, nucleusIds) {
            if (offerId && _.isArray(nucleusIds) && nucleusIds.length > 0) {
                return Origin.gifts.getGiftingEligibility(offerId, nucleusIds.join(','));
            }
            return Promise.resolve([]);
        }

        /**
         * Returns true if user eligibility status is eligible
         * @param eligibilityStatus
         */
        function isFriendEligibleToReceive(eligibilityStatus) {
            return INELIGIBLE_STATUS_ARRAY.indexOf(eligibilityStatus) < 0;
        }


        function createPaginatedResult(list, totalCount, getMoreFunction) {

            var originalList = list,
                startIndex = 0,
                currentPage;


            function getPage() {
                var pageSize = Math.min(originalList.length, startIndex + PAGE_SIZE);
                currentPage = paginateResult(originalList, pageSize);
                startIndex += PAGE_SIZE;
                return currentPage;
            }

            return {


                /**
                 *
                 * @returns {boolean}
                 */
                hasMore: function () {
                    return startIndex < totalCount;
                },

                /**
                 *
                 * @returns {Promise}
                 */
                getCurrentPage: function () {
                    if (currentPage) {
                        return Promise.resolve(currentPage);
                    }
                    return Promise.resolve(this.getNextPage());
                },

                /**
                 *
                 * @returns {Promise}
                 */
                getNextPage: function () {
                    var nextPageStartIndex = startIndex + PAGE_SIZE;
                    if (_.isFunction(getMoreFunction) && nextPageStartIndex > originalList.length && nextPageStartIndex < totalCount) {
                        return getMoreFunction(startIndex).then(function (result) {
                            originalList = result;
                            return getPage();
                        });
                    } else {
                        return Promise.resolve(getPage());
                    }
                }
            };

        }

        /**
         * get user info if necessary
         * @param existingUserData
         */
        function getUserInfo(existingUserData) {
            if (existingUserData.nucleusId) {
                if (!(existingUserData.firstname || existingUserData.lastname) && !existingUserData.username) {
                    return UserDataFactory.getUserInfo(existingUserData.nucleusId)
                        .then(function (userData) {
                            return {
                                nucleusId: _.get(userData, 'userId'),
                                firstname: _.get(userData, 'firstName', ''),
                                lastname: _.get(userData, 'lastName', ''),
                                username: _.get(userData, 'EAID', '')
                            };
                        }).catch(function () {
                            return 'Could not retrieve user';
                        });
                } else {
                    return Promise.resolve(existingUserData);
                }
            } else {
                return Promise.reject('nucleusId is required');
            }


        }

        function reset() {
            cachedFriendsList.length = 0;
            peopleResult.length = 0;
            selectUser(null);
        }




        function processFriendsList(friends) {

            var sortedList = _.map(friends).sort(UtilFactory.compareOriginId);

            //must use native for loop to preserve order
            for (var i = 0, length = sortedList.length; i < length; i++) {
                cachedFriendsList.push(sortedList[i].nucleusId);
            }

            return createPaginatedResult(cachedFriendsList, cachedFriendsList.length);
        }

        function processPeopleSearch(searchKeyword, searchResult) {
            var userInfoList = _.get(searchResult, 'infoList', []);
            var totalCount = _.get(searchResult, 'totalCount', 0);
            peopleResult = _.pluck(userInfoList, 'friendUserId');

            function getMoreFunction(startIndex) {
                return getMoreSearchResults(searchKeyword, startIndex).then(function (moreResults) {
                    userInfoList = _.get(moreResults, 'infoList', []);
                    peopleResult = [].concat(peopleResult, _.pluck(userInfoList, 'friendUserId'));
                    return peopleResult;
                }).catch(function () {
                    return peopleResult;
                });
            }

            return createPaginatedResult(peopleResult, totalCount, getMoreFunction);
        }

        function handleError() {
            return [];
        }

        function getFriendList() {
            if (cachedFriendsList.length) {
                return Promise.resolve(cachedFriendsList)
                    .then(_.partial(createPaginatedResult, cachedFriendsList, cachedFriendsList.length))
                    .catch(handleError);
            }
            return RosterDataFactory.getRoster(FRIENDS_ROSTER)
                .then(processFriendsList)
                .catch(handleError);
        }

        function getMoreSearchResults(searchKeyword, startIndex) {
            if (searchKeyword) {
                return SearchFactory.searchPeople(searchKeyword, startIndex);
            } else {
                return Promise.reject('missing-searchkeyword');
            }
        }

        function getSearchResults(searchKeyword) {
            return getMoreSearchResults(searchKeyword, 0)
                .then(_.partial(processPeopleSearch, searchKeyword));
        }

        function selectUser(userData) {
            selectedUser.data = userData;
            selectedUser.commit();
        }

        function getSelectedUserObservable() {
            return ObserverFactory.create(selectedUser);
        }

        function getSelectedUser() {
            return _.get(selectedUser, 'data');
        }

        AppCommFactory.events.on('uiRouter:stateChangeSuccess', reset);


        return {
            getSearchResults: getSearchResults,
            getFriendList: getFriendList,
            getUserInfo: getUserInfo,
            selectUser: selectUser,
            isFriendEligibleToReceive: isFriendEligibleToReceive,
            getFriendsGiftingEligibility: getFriendsGiftingEligibility,
            reset: reset,
            getSelectedUser: getSelectedUser,
            getSelectedUserObservable: getSelectedUserObservable
        };
    }


    /**
     * @ngdoc service
     * @name origin-components.factories.GiftFriendListFactory
     * @description
     *
     * GiftFriendListFactory
     */
    angular.module('origin-components')
        .factory('GiftFriendListFactory', GiftFriendListFactory);
}());
