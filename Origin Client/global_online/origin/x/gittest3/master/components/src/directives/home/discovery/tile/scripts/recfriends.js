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
        // strings
        $scope.addFriendString = UtilFactory.getLocalizedStr($scope.addFriendButtonStr, CONTEXT_NAMESPACE, 'addfriendcta');
        $scope.dismissString = UtilFactory.getLocalizedStr($scope.dismissButtonStr, CONTEXT_NAMESPACE, 'dismisscta');
        $scope.requestSentString = UtilFactory.getLocalizedStr($scope.requestSentStr, CONTEXT_NAMESPACE, 'requestsent');
        $scope.hideButtonString = UtilFactory.getLocalizedStr($scope.hideButtonStr, CONTEXT_NAMESPACE, 'hidecta');
        $scope.viewListButtonString = UtilFactory.getLocalizedStr($scope.viewListButtonStr, CONTEXT_NAMESPACE, 'viewlistcta');
        $scope.descriptionString = UtilFactory.getLocalizedStr($scope.descriptionStr, CONTEXT_NAMESPACE, 'description');
        $scope.noRecDescriptionString = UtilFactory.getLocalizedStr($scope.noRecDescriptionStr, CONTEXT_NAMESPACE, 'norecdescription');

        // state
        $scope.friendRequestSent = false;
        $scope.noMoreFriends = false;

        /**
        * Get the origin id for the user
        * @return {void}
        * @method
        */
        function getOriginId() {
            Origin.atom.atomUserInfoByUserIds([$scope.friendNucleusId])
                .then(function(response) {
                    $scope.originId = response[0].EAID;
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
        * Get the user's data, initialization method
        * @memberof originHomeDiscoveryTileRecfriendsCtrl
        * @method
        */
        this.getUserData = function() {
            getOriginId();
            getAvatarImage();
        };
    }

    /**
    * Origin Discovery Recommend Friend Tile directive
    * @directive originHomeDiscoveryTileRecfriends
    */
    function originHomeDiscoveryTileRecfriends($timeout, ComponentsConfigFactory, DiscoveryStoryFactory) {

        /**
        * Link for originHomeDiscoveryTileRecfriends
        * @link originHomeDiscoveryTileRecfriendsLink
        */
        function originHomeDiscoveryTileRecfriendsLink(scope, elem, attrs, ctrl) {

            var tile = angular.element(elem[0].querySelectorAll('.origin-friendtile')[0]);

            /**
            * Get the next friend recommendation and display it
            * @return {void}
            * @method
            */
            function getNextFriend() {

                var friend = DiscoveryStoryFactory.getNextRecFriend();
                scope.friendRequestSent = false;

                tile.addClass('origin-friendtile-isnewfriend');
                $timeout(function() {
                    tile.removeClass('origin-friendtile-isnewfriend');
                }, FRIEND_COMEBACK_DELAY, false);

                if (!!friend) {
                    scope.friendNucleusId = friend;
                    ctrl.getUserData();
                } else {
                    scope.noMoreFriends = true;
                    scope.image = ComponentsConfigFactory.getImagePath('user-placeholder.jpg');
                    scope.descriptionString = scope.noRecDescriptionString;
                }
            }

            /**
            * Add a friend
            * @return {void}
            * @method
            */
            scope.addFriend = function() {
                scope.friendRequestSent = true;
                getNextFriend();
            };

            /**
            * Dismiss the current selection
            * @return {void}
            * @method
            */
            scope.dismiss = function() {
                getNextFriend();
            };

            // fetch the user data first
            ctrl.getUserData();

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
                descriptionStr: '@description',
                noRecDescriptionStr: '@norecdescription'
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
     * @param {LocalizedString|OCD} dismisscta the dismiss action button text
     * @param {LocalizedString} description the tile description
     * @param {LocalizedText} norecdescription message shown when there are no more friends to connect
     * @param {LocalizedString} addfriendcta the add friend action button text
     * @param {LocalizedString} requestsent the request sent confirmation message
     * @param {LocalizedString} hidecta the hide action button text
     * @param {LocalizedString} viewlistcta the view friends list action button text
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
