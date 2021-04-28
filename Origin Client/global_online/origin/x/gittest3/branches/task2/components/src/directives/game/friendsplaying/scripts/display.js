/**
 * @file game/friendsplaying/display.js
 */
(function() {
    'use strict';

    // constants
    var AVATAR_WIDTH = 48;

    /**
     * Friends Playing Game Ctrl
     * @controller originGameFriendsplayingDisplayCtrl
     */
    function OriginGameFriendsplayingDisplayCtrl($scope, $sce, RosterDataFactory, DialogFactory) {

        var self = this,
            friendsOnline = true;

        this.friends = [];
        this.maxFriends = 1; // default this to 1 so we can get the size of the avatar .. might be a better way to do this.

        // these are the visible friends
        $scope.friends = [];
        $scope.hiddenFriends = 0;

        /**
         * Update your list of friends and then display them
         * @param {Array} list - your list of friends
         * @return {void}
         */
        function updateFriends(list) {
            if (list.length) {
                self.friends = list;
                friendsOnline = true;
            } else {
                self.friends = RosterDataFactory.getFriendsWhoPlayed($scope.offerId);
                friendsOnline = false;
            }
            self.displayFriends();
        }

        /**
         * Clean up
         * @method onDestroy
         * @return {void}
         */
        function onDestroy() {
            RosterDataFactory.events.off('socialFriendsPlayingListUpdated:' + $scope.offerId, updateFriends, self);
        }

        /**
         * Get the list of friends and then display them. Subscribe to any changes
         * @return {void}
         */
        this.initFriendsPlaying = function() {
            self.friends = RosterDataFactory.friendsWhoArePlaying($scope.offerId);
            self.displayFriends();
            RosterDataFactory.events.on('socialFriendsPlayingListUpdated:' + $scope.offerId, updateFriends, self);
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
                max--;
                $scope.hiddenFriends = friendLength - max;
            } else {
                $scope.hiddenFriends = 0;
            }
            $scope.friends = self.friends.slice(0, max);
        };

        /**
         * Display the friends modal
         * @return {void}
         */
        $scope.showFriendsModal = function() {
            DialogFactory.openDirective({
                id: 'web-show-friends-modal',
                name: friendsOnline ? 'origin-friends-playing-dialog' : 'origin-friends-played-dialog',
                data: {
                    offerId: $scope.offerId
                }
            });
        };

        $scope.$on('$destroy', onDestroy);

    }

    /**
     * Friends Playing Game directive
     * @directive originGameFriendsplayingDisplay
     */
    function originGameFriendsplayingDisplay(ComponentsConfigFactory, $window) {

        /**
         * Friends Playing Game Link
         * @link originGameFriendsplayingDisplayLink
         */
        function originGameFriendsplayingDisplayLink(scope, element, attrs, ctrl) {

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
            ctrl.initFriendsPlaying();
            angular.element($window).on('resize', onResize);
        }

        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                userId: '@userid'
            },
            controller: 'OriginGameFriendsplayingDisplayCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/friendsplaying/views/display.html'),
            link: originGameFriendsplayingDisplayLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameFriendsplayingDisplay
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offer id
     * @param {string} userid friends nucleus id
     *
     * friends playing display
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-friendsplaying-display offerid="OFB-EAST:109552154" userid="1000080156081"></origin-game-friendsplaying-display>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGameFriendsplayingDisplayCtrl', OriginGameFriendsplayingDisplayCtrl)
        .directive('originGameFriendsplayingDisplay', originGameFriendsplayingDisplay);
}());