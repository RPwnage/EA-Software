/**
 * @file game/friendsplaying/text.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-friendsplaying-text';

    function OriginGameFriendsplayingTextCtrl($scope, RosterDataFactory, UtilFactory, CommonGamesFactory, ObserverFactory) {
        var playingListObserver = null,
            playedListObserver = null;

        $scope.friendsPlaying = [];
        $scope.friendsPlayed = [];
        $scope.friendsOwned = [];
        $scope.friendsTitleStr = '';

        /**
         * Updates the friends title string
         * Called when any friends*TitleStr changes
         * @return {void}
         */
         function updateFriendsStr() {
            $scope.friendsTitleStr = $scope.requestedFriendType !== 'friendsOwned' && $scope.friendsPlaying.length ? $scope.friendsPlayingTitleStr : 
                ($scope.requestedFriendType !== 'friendsOwned' && $scope.friendsPlayed.length ? $scope.friendsPlayedTitleStr : 
                ($scope.friendsOwned.length ? $scope.friendsOwnedTitleStr : ''));

            // let 'origin-tile-support-text' element know that we are being shown
            $scope.$emit('friendsPlayingTextVisible', ($scope.friendsTitleStr.length > 0));
         }

        /**
         * Updates the friends playing title string
         * Called when changes to the friends playing observable is committed
         * @return {void}
         */
        function updateFriendsPlaying() {
            $scope.friendsPlayingTitleStr = UtilFactory.getLocalizedStr($scope.friendsPlayingDescriptionStr, CONTEXT_NAMESPACE, 'friendsplayingdescription', {
                '%friends%': $scope.friendsPlaying.length
            }, $scope.friendsPlaying.length);

            updateFriendsStr();
        }

        /**
         * Updates the friends played title string
         * Called when changes to the friends played observable is committed
         * @return {void}
         */
        function updateFriendsPlayed() {
            $scope.friendsPlayedTitleStr = UtilFactory.getLocalizedStr($scope.friendsPlayedDescriptionStr, CONTEXT_NAMESPACE, 'friendsplayeddescription', {
                '%friends%': $scope.friendsPlayed.length
            }, $scope.friendsPlayed.length);

            updateFriendsStr();
        }        

        /**
         * Updates the friends owned list and title string
         * @return {void}
         */
        function updateFriendsOwned(friendsList) {
            $scope.friendsOwned = friendsList;
            $scope.friendsOwnedTitleStr = UtilFactory.getLocalizedStr($scope.friendsOwnedDescriptionStr, CONTEXT_NAMESPACE, 'friendsowneddescription', {
                '%friends%': friendsList.length
            }, friendsList.length);

            updateFriendsStr();
        }        


        $scope.$on('$destroy', function() {
            if(playingListObserver) {
                playingListObserver.detach();
                playingListObserver = null;

            }

            if(playedListObserver) {
                playedListObserver.detach();
                playedListObserver = null;
            }
        });

        if ($scope.offerId) {

            if ($scope.requestedFriendType === 'friendsOwned') {
                // We are treating this data as static, so it doesn't need an observable, and it only fetched once
                CommonGamesFactory.getUsersWhoOwn($scope.offerId)
                    .then(updateFriendsOwned);

            } else {
                // Create observers of the roster data friends playing and played observables
                // Attach to $scope.friendsPlaying and $scope.friendsPlayed

                // Also call an updater function to refresh the title strings.                
                RosterDataFactory.getFriendsPlayingObservable($scope.offerId)
                    .then(function(observable) {
                        playingListObserver= ObserverFactory.create(observable);
                        playingListObserver.getProperty('list')
                            .attachToScope($scope, 'friendsPlaying');
                        playingListObserver.attachToScope($scope, updateFriendsPlaying);
                    });

                RosterDataFactory.getFriendsPlayedObservable($scope.offerId)
                    .then(function(observable) {
                        playedListObserver = ObserverFactory.create(observable);
                        playedListObserver.getProperty('list')
                            .attachToScope($scope, 'friendsPlayed');
                        playedListObserver.attachToScope($scope, updateFriendsPlayed);
                    });

            }

        }

        $scope.$emit('friendsTextVisible', ($scope.text.length > 0));
        $scope.$emit('friendsSubtitleVisible', $scope.subtitle ? ($scope.subtitle.length > 0) : false);
    }

    function originGameFriendsplayingText(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                subtitle: '@subtitle',
                text: '@',
                friendsText: '@friendstext',
                requestedFriendType: '@requestedfriendtype',
                friendsOwnedDescriptionStr: '@friendsowneddescription',
                friendsPlayedDescriptionStr: '@friendsplayeddescription',
                friendsPlayingDescriptionStr: '@friendsplayingdescription'
            },
            controller: 'OriginGameFriendsplayingTextCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/friendsplaying/views/text.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameFriendsplayingText
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offer ID
     * @param {LocalizedText} subtitle the subtitle text
     * @param {LocalizedText} text text prepended to the friends text
     * @param {LocalizedText} friendstext text describing how many friends are playing
     * @param {LocalizedString} friendsowneddescription * merchandized in defaults
     * @param {LocalizedString} friendsplayeddescription * merchandized in defaults
     * @param {LocalizedString} friendsplayingdescription * merchandized in defaults
     * @description
     *
     * friends playing game text
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-game-friendsplaying-text
     *             offerid="OFB-EAST:109552154" text="Now Available"
     *             friendsplayingdescription="You gots friends playing." friendsplayeddescription="Your friend played.">
     *         </origin-game-friendsplaying-text>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGameFriendsplayingTextCtrl', OriginGameFriendsplayingTextCtrl)
        .directive('originGameFriendsplayingText', originGameFriendsplayingText);
}());