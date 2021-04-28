/**
 * @file home/discovery/tile/recfriends.js
 */
(function() {
    'use strict';

    // constants
    var FRIEND_COMEBACK_DELAY = 600,
        CONTEXT_NAMESPACE = 'origin-home-discovery-tile-recfriends',
        TELEMETRYPREFIX = 'telemetry-';

    /**
     * Controller for originHomeDiscoveryTileRecfriends directive
     * @controller OriginHomeDiscoveryTileRecfriendsCtrl
     */
    function OriginHomeDiscoveryTileRecfriendsCtrl($scope, $timeout, UserDataFactory, UtilFactory, ComponentsLogFactory) {

        var friendsTileDisplayVisible = false;

        // strings
        $scope.addFriendString = UtilFactory.getLocalizedStr($scope.addFriendButtonStr, CONTEXT_NAMESPACE, 'addfriendcta');
        $scope.dismissString = UtilFactory.getLocalizedStr($scope.dismissButtonStr, CONTEXT_NAMESPACE, 'dismisscta');
        $scope.requestSentString = UtilFactory.getLocalizedStr($scope.requestSentStr, CONTEXT_NAMESPACE, 'requestsent');
        $scope.hideButtonString = UtilFactory.getLocalizedStr($scope.hideButtonStr, CONTEXT_NAMESPACE, 'hidecta');
        $scope.viewListButtonString = UtilFactory.getLocalizedStr($scope.viewListButtonStr, CONTEXT_NAMESPACE, 'viewlistcta');
        $scope.noRecDescriptionString = UtilFactory.getLocalizedStr($scope.noRecDescriptionStr, CONTEXT_NAMESPACE, 'norecdescription');
        $scope.viewProfileString = UtilFactory.getLocalizedStr($scope.viewProfileStr, CONTEXT_NAMESPACE, 'viewprofilecta');
        $scope.popoutTitleStr = UtilFactory.getLocalizedStr($scope.friendsrecommendedstr, CONTEXT_NAMESPACE, 'popoutfriendsrecommendedtitle');

        // telemetry data
        $scope.ctid = 'friend_rec_' + Origin.user.userPid()  + '_'  + $scope.friendNucleusId;
        $scope.numMutualFriends = $scope.friendsInCommonStr ? $scope.friendsInCommonStr.split(',').length : 0;

        // state
        $scope.friendRequestSent = false;
        $scope.noFriend = false;

        $scope.commonFriends = $scope.friendsInCommonStr?($scope.friendsInCommonStr.split(',') || []):[];


        function updateDescriptionStr() {
            $scope.descriptionString = UtilFactory.getLocalizedStr($scope.descriptionStr, CONTEXT_NAMESPACE, 'description', {
                '%friends%': $scope.numMutualFriends
            }, $scope.numMutualFriends);
        }

        /**
         * Get the origin id and full name of the user
         * @return {void}
         * @method
         */
        function getUserInfo() {
            return Origin.atom.atomUserInfoByUserIds([$scope.friendNucleusId])
                .then(function(response) {
                    $scope.originId = response[0].EAID;

                    // Build the full name
                    if (response[0].firstName || response[0].lastName) {
                        $scope.fullName = response[0].firstName ? response[0].firstName : '';
                        $scope.fullName += response[0].lastName ? ' ' + response[0].lastName : '';
                        $scope.fullName = $scope.fullName.trim();
                    }

                }).catch(function(error) {
                    ComponentsLogFactory.error('OriginHomeDiscoveryTileRecfriendsCtrl: Origin.atom.atomUserInfoByUserIds failed', error);
                });
        }

        /**
         * Get the large and small images for the user
         * @return {void}
         * @method
         */
        function getAvatarImage() {
            return UserDataFactory.getAvatar($scope.friendNucleusId, Origin.defines.avatarSizes.MEDIUM)
                .then(function(response) {
                    $scope.image = response;
                }).catch(function(error) {
                    ComponentsLogFactory.error('OriginHomeDiscoveryTileRecfriendsCtrl: UserDataFactory.getAvatar failed', error);
                });
        }


        /**
         * check if we should show the add dismiss friend buttons
         * @return {Boolean} true if we the add dismiss friend buttons
         */
        $scope.canShowAddDismissFriendButtons = function() {
            return !$scope.friendRequestSent && !$scope.noFriend && !$scope.showProfileButton;
        };

        /**
         * check if we should show the friend request sent message
         * @return {Boolean} true we should show the friend request sent message
         */
        $scope.canShowRequestButtons = function() {
            return $scope.friendRequestSent && !$scope.noFriend && !$scope.showProfileButton;
        };

        /**
         * check if we should show the view friends list button
         * @return {Boolean} true if we want to sow the view friends list button
         */
        $scope.canShowViewFriendsList = function() {
            return $scope.noFriend && !$scope.showProfileButton;
        };

        /**
         * check if we should show the profile button
         * @return {Boolean} true if we only want to show the profile button
         */
        $scope.canShowProfileButton = function() {
            return !!$scope.showProfileButton;
        };

        /**
         * check if we should show the full name
         * @return {Boolean} true if we should show the full name
         */
        $scope.canShowFullName = function() {
            return $scope.showFullName && $scope.fullName;
        };


        function nonDirectiveSupportTextVisible() {
            return (!$scope.noFriend && _.get($scope, ['originId', 'length'], 0)) ||
                ($scope.canShowFullName() && _.get($scope, ['fullName', 'length'], 0)) ||
                ($scope.friendsInCommonStr && _.get($scope, 'descriptionString.length',0));
        }

        function updateSupportTextVisible() {
            $scope.supportTextVisible = friendsTileDisplayVisible || nonDirectiveSupportTextVisible();
        }

        function setSupportTextVisible() {
             $scope.supportTextVisible = false;

             $scope.$on('friendsTileDisplayVisible', function(event, visible) {
                friendsTileDisplayVisible = visible;
                updateSupportTextVisible();
             });
        }

        /**
         * Check to see if the current device can handle touch
         * @type {Boolean}
         */
        $scope.isTouchDisable = !UtilFactory.isTouchEnabled();

        //set a class in the dom as a marker for telemetry
        $scope.telemetryTrackerClass =  TELEMETRYPREFIX + CONTEXT_NAMESPACE;

        //send impression telemetry data once dom is loaded
        setTimeout(Origin.telemetry.sendDomLoadedTelemetryEvent, 0);

        function runDigest() {
            $scope.$digest();
        }

        /**
         * Get the user's data, initialization method
         * @memberof originHomeDiscoveryTileRecfriendsCtrl
         * @method
         */
        this.getUserData = function() {
            updateDescriptionStr();
            return Promise.all([
                getUserInfo(),
                getAvatarImage()
                ])
                .then(updateSupportTextVisible)
                .then(runDigest);
        };

        setSupportTextVisible();
        updateDescriptionStr();
    }

    /**
     * Origin Discovery Recommend Friend Tile directive
     * @directive originHomeDiscoveryTileRecfriends
     */
    function originHomeDiscoveryTileRecfriends($timeout, ComponentsConfigFactory, RosterDataFactory, DiscoveryStoryFactory, ComponentsLogFactory, NavigationFactory) {

        /**
         * Link for originHomeDiscoveryTileRecfriends
         * @link originHomeDiscoveryTileRecfriendsLink
         */
        function originHomeDiscoveryTileRecfriendsLink(scope, elem, attrs, ctrl) {

            var tile = elem.parent(),
                recNextFriendTimeoutHandle = null;

            /**
             * Get the next friend recommendation and display it
             * @return {void}
             * @method
             */
            function getNextFriend() {

                var friendInfo = scope.$parent.smuggleNextTile();
                scope.friendRequestSent = false;

                if (friendInfo) {
                    scope.friendNucleusId = friendInfo.userid;
                    scope.friendsInCommonStr = friendInfo.friendsInCommonStr.join(',');
                    ctrl.getUserData().then(function() {
                        tile.addClass('origin-friendtile');
                        $timeout(function() {
                            tile.removeClass('origin-friendtile');
                        }, FRIEND_COMEBACK_DELAY, false);
                    });
                } else {
                    scope.noMoreFriends = true;
                    if(!tile.siblings().length) {
                        scope.$parent.visible = false;
                    }
                    tile.remove();
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
             * pckage additional pinEvent Data
             * @param option
             * @returns {{type: string, service: string, format: string, client_state: string, msg_id: string, status: string, content: string, destination_name: string, meta: {friend_id: string, friend_type: string}, reqs: {mutual_frd_cnt: *}, option: *}}
             */
            function packPinEventData(option) {
                return {
                    'type': 'in-game',
                    'service': 'originx',
                    'format': 'live_tile',
                    'client_state': 'foreground',
                    'msg_id': scope.ctid,
                    'status': 'click',
                    'content': 'friend_recommendation',
                    'destination_name': 'my_home',
                    'meta' : {
                        'friend_id' :  scope.friendNucleusId,
                        'friend_type': 'nucleus'
                    },
                    'reqs' : {
                        'mutual_frd_cnt' : scope.numMutualFriends
                    },
                    'option' : option
                };
            }

            /**
             * Add a friend
             * @return {void}
             * @method
             */
            scope.addFriend = function($event) {
                Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_FRIEND_RECOMMENDATION_ADD,
                    packPinEventData('yes'));

                RosterDataFactory.sendFriendRequest(scope.friendNucleusId, 'recommendation');
                //show the friendRequestSent for 2 seconds before we grab the next friend
                //prototype use 2 seconds.
                recNextFriendTimeoutHandle = setTimeout(getNextFriend, 2000);
                scope.friendRequestSent = true;
                $event.stopPropagation();
            };

            /**
             * go to a users profile
             * @return {void}
             */
            scope.goToProfile = function() {
                Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_FRIEND_RECOMMENDATION_MORE_INFO,
                    packPinEventData('more_info'));
                NavigationFactory.goUserProfile(scope.friendNucleusId);
            };

            /**
             * Dismiss the current selection
             * @return {void}
             * @method
             */
            scope.dismiss = function($event) {
                Origin.telemetry.events.fire(Origin.telemetry.events.TELEMETRY_FRIEND_RECOMMENDATION_DISMISS,
                    packPinEventData('cancel'));
                Origin.friends.friendRecommendationsIgnore(scope.friendNucleusId)
                    .then(getNextFriend)
                    .catch(function(error) {
                        ComponentsLogFactory.error('[origin-home-discovery-tile-recfriends] friend recommendaton ignore failed:', error);
                    });
                $event.stopPropagation();
            };

            // fetch the user data first
            ctrl.getUserData();

            scope.$on('$destroy', onDestroy);
        }

        return {
            restrict: 'E',
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
                friendsrecommendedstr: '@popoutfriendsrecommendedtitle',
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
     * @param {LocalizedString} popoutfriendsrecommendedtitle the title seen on popout for why this is a recommended friend, e.g "Friends In Common"
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
