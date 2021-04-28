/**
 * @file friends/scripts/tiledisplay.js
 */
(function() {
    'use strict';

    // constants
    var AVATAR_WIDTH = 48;

    /**
     * Friends Playing Game Ctrl
     * @controller originFriendsTiledisplayCtrl
     */
    function OriginFriendsTiledisplayCtrl($scope, DialogFactory) {

        var self = this;

        /**
         * Update your list of friends and then display them
         * @param {Array} list - your list of friends
         * @return {void}
         */
        function updateFriends() {
            self.friends = $scope.friendslist || [];
            self.displayFriends();
        }

        /**
         * Get the list of friends and then display them. Subscribe to any changes
         * @return {void}
         */
        this.initFriendsList = updateFriends;

        /**
         * This is the function called the +1, +2 friends icon is clickedn when there is not enough
         * space to display the full list
         * @return {void}
         */
        $scope.showFriendsModal = function() {
            DialogFactory.openDirective({
                id: 'web-show-friends-modal',
                name: 'origin-dialog-content-friendspopout',
                data: {
                    friendspopouttitle: $scope.popoutTitleStr,
                    friendsliststr: self.friends.join(',')
                }
            });
        };

        /**
         * Display the list of friends.  Note that if you are over the max
         * length then we display an optional counter.  In the future
         * the counter will be its own component as it has a list of friends
         * that show when you hover or something but we don't have the designs
         * for that.
         * @return {void}
         */
        this.displayFriends = function() {

            var max = self.maxFriends,
                friendLength = self.friends.length;
            if (friendLength > max) {
                if (max > 0) {
                    max--;
                }
                $scope.hiddenFriends = friendLength - max;
            } else {
                $scope.hiddenFriends = 0;
            }
            $scope.friends = self.friends.slice(0, max);
        };

        $scope.$watch('friendslist', updateFriends);
        this.friends = [];
        this.maxFriends = 1; // default this to 1 so we can get the size of the avatar .. might be a better way to do this.

        // these are the visible friends
        $scope.hiddenFriends = 0;
    }

    /**
     * Friends Playing Game directive
     * @directive originFriendsTiledisplay
     */
    function originFriendsTiledisplay(ComponentsConfigFactory, $window) {

        /**
         * Friends Playing Game Link
         * @link originFriendsTiledisplayLink
         */
        function originFriendsTiledisplayLink(scope, element, attrs, ctrl) {

            var tid;

            /**
             * Determine the maximum number of friends that you can
             * display in the tile
             * @return {void}
             * @method
             */
            function determineMaxFriends() {
                var listWidth = element.find('.origin-avatarlist')[0].offsetWidth;
                ctrl.maxFriends = Math.floor(listWidth / AVATAR_WIDTH);
            }


            /**
             * On resize, determine the number of maximum friends
             * that you can show, and then show those friends, updating
             * the counter.
             * @return {void}
             * @method
             */
            function updateNumVisibleFriends() {
                determineMaxFriends();
                ctrl.displayFriends();
                scope.$digest();
            }

            /**
             * On resize, determine the number of maximum friends
             * that you can show, and then show those friends, updating
             * the counter.  This function just ensures we are not spamming
             * the callback function
             * @return {void}
             * @method
             */
            function onResize() {
                if (tid) {
                    window.clearTimeout(tid);
                    tid = null;
                }
                tid = window.setTimeout(updateNumVisibleFriends, 30);
            }

            // initialization
            determineMaxFriends();
            ctrl.initFriendsList();
            angular.element($window).on('resize', onResize);

        }

        return {
            restrict: 'E',
            scope: {
                friendslist: '=',
                popoutTitleStr: '@popouttitlestr'
            },
            controller: 'OriginFriendsTiledisplayCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('friends/views/tiledisplay.html'),
            link: originFriendsTiledisplayLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originFriendsTiledisplay
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} friendsliststr a comma deliminated list of nucleus ids to show
     * @param {function} showfriendsmodal a function called when we click on the "+ button"
     * @param {string} offerid an offer id of the game passed on to the the usercard
     *
     * friends playing display
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-friends-tiledisplay  friendsstrlist="1000080156081,1000080156451,1000080126082" showfriendsmodal=somefunction></origin-friends-tiledisplay>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginFriendsTiledisplayCtrl', OriginFriendsTiledisplayCtrl)
        .directive('originFriendsTiledisplay', originFriendsTiledisplay);
}());