/**
 * @file home/discovery/tile/recfriends.js
 */
(function() {
    'use strict';

    // constants
    var AVATAR_SIZE_MEDIUM = 'AVATAR_SZ_MEDIUM',
        FRIEND_COMEBACK_DELAY = 600,
        CONTEXT_NAMESPACE = 'origin-home-discovery-tile-recfriends';

    /**
     * Controller for originHomeDiscoveryTileRecfriends directive
     * @controller OriginHomeDiscoveryTileRecfriendsCtrl
     */
    function OriginHomeDiscoveryTileRecfriendsCtrl($scope, UserDataFactory, UtilFactory, ComponentsLogFactory) {

        function updateDescriptionStr() {
            var numMutualFriends = $scope.friendsInCommonStr ? $scope.friendsInCommonStr.split(',').length : 0;
            $scope.descriptionString = UtilFactory.getLocalizedStr($scope.descriptionStr, CONTEXT_NAMESPACE, 'description', {
                '%friends%': numMutualFriends
            }, numMutualFriends);
        }

        // strings
        $scope.addFriendString = UtilFactory.getLocalizedStr($scope.addFriendButtonStr, CONTEXT_NAMESPACE, 'addfriendcta');
        $scope.dismissString = UtilFactory.getLocalizedStr($scope.dismissButtonStr, CONTEXT_NAMESPACE, 'dismisscta');
        $scope.requestSentString = UtilFactory.getLocalizedStr($scope.requestSentStr, CONTEXT_NAMESPACE, 'requestsent');
        $scope.hideButtonString = UtilFactory.getLocalizedStr($scope.hideButtonStr, CONTEXT_NAMESPACE, 'hidecta');
        $scope.viewListButtonString = UtilFactory.getLocalizedStr($scope.viewListButtonStr, CONTEXT_NAMESPACE, 'viewlistcta');
        $scope.noRecDescriptionString = UtilFactory.getLocalizedStr($scope.noRecDescriptionStr, CONTEXT_NAMESPACE, 'norecdescription');
        $scope.viewProfileString = UtilFactory.getLocalizedStr($scope.viewProfileStr, CONTEXT_NAMESPACE, 'viewprofilecta');
        $scope.popoutTitleStr = UtilFactory.getLocalizedStr($scope.friendsrecommendedstr, CONTEXT_NAMESPACE, 'popoutfriendsrecommendedtitle');

        // state
        $scope.friendRequestSent = false;
        $scope.noMoreFriends = false;


        $scope.commonFriends = $scope.friendsInCommonStr?($scope.friendsInCommonStr.split(',') || []):[];

        /**
         * Get the origin id and full name of the user
         * @return {void}
         * @method
         */
        function getUserInfo() {
            Origin.atom.atomUserInfoByUserIds([$scope.friendNucleusId])
                .then(function(response) {
                    $scope.originId = response[0].EAID;

                    // Build the full name
                    if (response[0].firstName || response[0].lastName) {
                        $scope.fullName = response[0].firstName ? response[0].firstName : '';
                        $scope.fullName += response[0].lastName ? ' ' + response[0].lastName : '';
                        $scope.fullName = $scope.fullName.trim();
                    }

                    $scope.$digest();
                }).catch(function(error) {
                    ComponentsLogFactory.error('OriginHomeDiscoveryTileRecfriendsCtrl: Origin.atom.atomUserInfoByUserIds failed', error.stack);
                });
        }

        /**
         * Get the large and small images for the user
         * @return {void}
         * @method
         */
        function getAvatarImage() {
            UserDataFactory.getAvatar($scope.friendNucleusId, AVATAR_SIZE_MEDIUM)
                .then(function(response) {
                    $scope.image = response;
                    $scope.$digest();
                }).catch(function(error) {
                    ComponentsLogFactory.error('OriginHomeDiscoveryTileRecfriendsCtrl: UserDataFactory.getAvatar failed', error.stack);
                });
        }


        /**
         * check if we should show the add dismiss friend buttons
         * @return {Boolean} true if we the add dismiss friend buttons
         */
        $scope.canShowAddDismissFriendButtons = function() {
            return !$scope.friendRequestSent && !$scope.noMoreFriends && !$scope.showProfileButton;
        };

        /**
         * check if we should show the friend request sent message
         * @return {Boolean} true we should show the friend request sent message
         */
        $scope.canShowRequestButtons = function() {
            return $scope.friendRequestSent && !$scope.noMoreFriends && !$scope.showProfileButton;
        };

        /**
         * check if we should show the view friends list button
         * @return {Boolean} true if we want to sow the view friends list button
         */
        $scope.canShowViewFriendsList = function() {
            return $scope.noMoreFriends && !$scope.showProfileButton;
        };

        /**
         * check if we should show the profile button
         * @return {Boolean} true if we only want to show the profile button
         */
        $scope.canShowProfileButton = function() {
            return $scope.showProfileButton;
        };

        /**
         * check if we should show the full name
         * @return {Boolean} true if we should show the full name
         */
        $scope.canShowFullName = function() {
            return $scope.showFullName && $scope.fullName;
        };

        /**
         * Check to see if the current device can handle touch
         * @type {Boolean}
         */
        $scope.isTouchDisable = !UtilFactory.isTouchEnabled();

        /**
         * Get the user's data, initialization method
         * @memberof originHomeDiscoveryTileRecfriendsCtrl
         * @method
         */
        this.getUserData = function() {
            updateDescriptionStr();
            getUserInfo();
            getAvatarImage();
        };


        updateDescriptionStr();
    }

    /**
     * Origin Discovery Recommend Friend Tile directive
     * @directive originHomeDiscoveryTileRecfriends
     */
    function originHomeDiscoveryTileRecfriends($timeout, ComponentsConfigFactory, RosterDataFactory, DiscoveryStoryFactory, ComponentsLogFactory, $state) {

        /**
         * Link for originHomeDiscoveryTileRecfriends
         * @link originHomeDiscoveryTileRecfriendsLink
         */
        function originHomeDiscoveryTileRecfriendsLink(scope, elem, attrs, ctrl) {

            var tile = angular.element(elem[0].querySelectorAll('.origin-friendtile')[0]),
                recNextFriendTimeoutHandle = null;

            /**
             * Get the next friend recommendation and display it
             * @return {void}
             * @method
             */
            function getNextFriend() {

                var friendInfo = DiscoveryStoryFactory.getNextRecFriend();
                scope.friendRequestSent = false;

                tile.addClass('origin-friendtile-isnewfriend');
                $timeout(function() {
                    tile.removeClass('origin-friendtile-isnewfriend');
                }, FRIEND_COMEBACK_DELAY, false);

                if (!!friendInfo) {
                    scope.friendNucleusId = friendInfo.userid;
                    scope.friendsInCommonStr = friendInfo.friendsInCommonStr;
                    ctrl.getUserData();
                } else {
                    scope.noMoreFriends = true;
                    scope.friendsInCommonStr = '';
                    scope.friendNucleusId = '';
                    scope.image = ComponentsConfigFactory.getImagePath('user-placeholder.jpg');
                    scope.descriptionString = scope.noRecDescriptionString;
                    scope.$digest();
                }
            }


            /**
             * Clean up
             * @method onDestroy
             * @return {void}
             */
            function onDestroy() {
                clearTimeout(recNextFriendTimeoutHandle);
            }

            /**
             * Add a friend
             * @return {void}
             * @method
             */
            scope.addFriend = function() {
                RosterDataFactory.sendFriendRequest(scope.friendNucleusId);
                scope.friendRequestSent = true;
                //show the friendRequestSent for 3 seconds before we grab the next friend
                recNextFriendTimeoutHandle = setTimeout(getNextFriend, 3000);
            };

            /**
             * go to a users profile
             * @return {void}
             */
            scope.goToProfile = function() {
                $state.go('app.profile.user', {
                    id: scope.friendNucleusId
                });
            };

            /**
             * Dismiss the current selection
             * @return {void}
             * @method
             */
            scope.dismiss = function() {
                Origin.friends.friendRecommendationsIgnore(scope.friendNucleusId)
                    .then(getNextFriend)
                    .catch(function(error) {
                        ComponentsLogFactory.error('[origin-home-discovery-tile-recfriends] friend recommendaton ignore failed:', error.message);
                    });
            };

            // fetch the user data first
            ctrl.getUserData();

            scope.$on('$destroy', onDestroy);
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                friendNucleusId: '@userid',
                dismissButtonStr: '@dismisscta',
                addFriendButtonStr: '@addfriendcta',
                requestSentStr: '@requestsent',
                hideButtonStr: '@hidecta',
                viewListButtonStr: '@viewlistcta',
                viewProfileStr: '@viewprofilecta',
                descriptionStr: '@description',
                noRecDescriptionStr: '@norecdescription',
                friendsInCommonStr: '@friendsincommonstr',
                showProfileButton: '@showprofilebutton',
                showFullName: '@showfullname'
            },
            controller: 'OriginHomeDiscoveryTileRecfriendsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/friend.html'),
            link: originHomeDiscoveryTileRecfriendsLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileRecfriends
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} userid the nucleus id of the recommended friend
     * @param {LocalizedString} dismisscta the dismiss action button text
     * @param {LocalizedString} description the tile description
     * @param {LocalizedText} norecdescription message shown when there are no more friends to connect
     * @param {LocalizedString} addfriendcta the add friend action button text
     * @param {LocalizedString} requestsent the request sent confirmation message
     * @param {LocalizedString} hidecta the hide action button text
     * @param {LocalizedString} viewlistcta the view friends list action button text
     * @param {LocalizedString} viewprofilecta the profile action button text
     * @param {string} friendsincommonstr a comma delimited string of the friends in common
     * @param {boolean} showprofilebutton true if we want to see only the profile button
     * @param {boolean} showfullname true if we want to see the full name
     *
     * recommended friends discovery tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-recfriends userid="12295990004"</origin-home-discovery-tile-recfriends>
     *     </file>
     * </example>
     */
    // directive declaration
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileRecfriendsCtrl', OriginHomeDiscoveryTileRecfriendsCtrl)
        .directive('originHomeDiscoveryTileRecfriends', originHomeDiscoveryTileRecfriends);
}());