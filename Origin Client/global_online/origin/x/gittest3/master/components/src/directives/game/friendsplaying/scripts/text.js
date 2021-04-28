/**
 * @file game/friendsplaying/text.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-game-friendsplaying-text';

    function OriginGameFriendsplayingTextCtrl($scope, RosterDataFactory, UtilFactory) {
        $scope.isLoaded = false;
        $scope.showFriendsIcon = false;

        /**
         * Localizes the 'Friends playing (-ed) string for a given number of friends
         * @param {string} stringId localized string ID as defined in the dictionary
         * @param {integer} friendsCount number of friends
         * @return {string}
         */
        function localize(stringId, friendsCount) {
            return UtilFactory.getLocalizedStr(
                $scope[stringId],
                CONTEXT_NAMESPACE,
                stringId.toLowerCase(), {
                    '%friends%': friendsCount
                },
                friendsCount
            );
        }

        /**
         * Updates the friend playing description based on the current number of friends
         * @return {string}
         */
        function updateDescription() {
            var result = '',
                friendsPlayingCount = RosterDataFactory.friendsWhoArePlaying($scope.offerId).length,
                friendsPlayedCount = RosterDataFactory.getFriendsWhoPlayed($scope.offerId).length;

            if (friendsPlayingCount) {
                result = localize('friendsPlayingDescription', friendsPlayingCount);
            } else if (friendsPlayedCount && $scope.isLoaded) {
                result = localize('friendsPlayedDescription', friendsPlayedCount);
            }

            $scope.description = $scope.text + result;
            $scope.showFriendsIcon = (friendsPlayingCount + friendsPlayedCount) > 0;
            $scope.isLoaded = true;
        }

        /**
        * On playing list updated callback handler
        * @return {void}
        * @method onPlayingListUpdated
        */
        function onPlayingListUpdated() {
            updateDescription();
            $scope.$digest();
        }

        /**
        * clean up after yourself
        * @return {void}
        * @method onDestroy
        */
        function onDestroy() {
            RosterDataFactory.events.off('socialFriendsPlayingListUpdated:' + $scope.offerId, onPlayingListUpdated);
        }

        // subscribe to event changes
        RosterDataFactory.events.on('socialFriendsPlayingListUpdated:' + $scope.offerId, onPlayingListUpdated);
        $scope.$on('$destroy', onDestroy);

        updateDescription();
    }

    function originGameFriendsplayingText(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                text: '@',
                friendsPlayingDescription: '@friendsplayingdescription',
                friendsPlayedDescription: '@friendsplayeddescription'
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
     * @param {LocalizedText} friendsplayingdescription string that tells the user friends are playing
     * @param {LocalizedText} friendsplayeddescription string that tells the user friends played
     * @param {string} offerid the offer ID
     * @param {string} text string to display the number of the playing friends with
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