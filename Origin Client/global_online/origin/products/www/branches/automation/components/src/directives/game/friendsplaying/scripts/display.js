/**
 * @file game/friendsplaying/display.js
 */
(function() {
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-game-friendsplaying-display';

    /**
     * Friends Playing Game Ctrl
     * @controller originGameFriendsplayingDisplayCtrl
     */
    function OriginGameFriendsplayingDisplayCtrl($scope, RosterDataFactory, ComponentsLogFactory, UtilFactory, ObserverFactory, CommonGamesFactory, GamesDataFactory) {
        var playingListObserver = null,
            playedListObserver = null;

        $scope.friendsPlaying = [];
        $scope.friendsPlayed = [];
        $scope.friendsOwned = [];

        /**
         * Sets the title strings with the i18n display name of the game
         * @param {Object} catalog
         * @return {void}
         */
        function setTitleStrings(catalog) {
            var catalogInfo = catalog[$scope.offerId];

            if (!!catalogInfo && !!catalogInfo.i18n) {
                $scope.friendsOwnedTitleStr = UtilFactory.getLocalizedStr($scope.popoutFriendsOwnedGameStr, CONTEXT_NAMESPACE, 'popoutfriendsownedgame', {
                    '%game%': catalogInfo.i18n.displayName
                });

                $scope.friendsPlayingTitleStr = UtilFactory.getLocalizedStr($scope.popoutFriendsPlayingGameStr, CONTEXT_NAMESPACE, 'popoutfriendsplayinggame', {
                    '%game%': catalogInfo.i18n.displayName
                });

                $scope.friendsPlayedTitleStr = UtilFactory.getLocalizedStr($scope.popoutFriendsPlayedGameStr, CONTEXT_NAMESPACE, 'popoutfriendsplayedgame', {
                    '%game%': catalogInfo.i18n.displayName
                });
            }
        }

        function createListObserver(obs) {
            return ObserverFactory.create(obs);
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

            GamesDataFactory.getCatalogInfo([$scope.offerId]).then(setTitleStrings);

            if ($scope.requestedFriendType === 'friendsOwned') {

                CommonGamesFactory
                    .getUsersWhoOwn($scope.offerId)
                    .then(function(list) {
                        $scope.friendsOwned = list;

                    });

            } else {

                RosterDataFactory.getFriendsPlayingObservable($scope.offerId)
                    .then(function(obs) {
                        playingListObserver = createListObserver(obs);
                        playingListObserver.getProperty('list').attachToScope($scope, 'friendsPlaying');
                    });

                RosterDataFactory.getFriendsPlayedObservable($scope.offerId)
                    .then(function(obs) {
                        playedListObserver = createListObserver(obs);
                        playedListObserver.getProperty('list').attachToScope($scope, 'friendsPlayed');

                    });

            }

        }
    }

    /**
     * Friends Playing Game directive
     * @directive originGameFriendsplayingDisplay
     */
    function originGameFriendsplayingDisplay(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                popoutTitle: '@popouttitle',
                requestedFriendType: '@requestedfriendtype',
                popoutFriendsOwnedGameStr: '@popoutfriendsownedgame',
                popoutFriendsPlayingGameStr: '@popoutfriendsplayinggame',
                popoutFriendsPlayedGameStr: '@popoutfriendsplayedgame'
            },
            controller: 'OriginGameFriendsplayingDisplayCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/friendsplaying/views/display.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameFriendsplayingDisplay
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offer id
     * @param {LocalizedText} popouttitle string used in the title
     * @param {LocalizedString} popoutfriendsownedgame * merchandized in defaults
     * @param {LocalizedString} popoutfriendsplayinggame * merchandized in defaults
     * @param {LocalizedString} popoutfriendsplayedgame * merchandized in defaults
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
