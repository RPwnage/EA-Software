/**
 * @file gamelibrary/scripts/ogdfriendswhoplay.js
 */
(function () {

    'use strict';
    /**
     * Game Library OGD Friends Who Play Ctrl
     * @controller OriginGamelibraryOgdFriendswhoplayCtrl
     */
    function OriginGamelibraryOgdFriendswhoplayCtrl($scope, RosterDataFactory, UtilFactory, CommonGamesFactory, ObjectHelperFactory, FindFriendsFactory, ObserverFactory) {
        var CONTEXT_NAMESPACE = 'origin-gamelibrary-ogd-friendswhoplay',
            SHOW_FRIENDS_LIMIT = 9,
            mapWith = ObjectHelperFactory.mapWith,
            getRosterUser = RosterDataFactory.getRosterUser,
            fetchPromises = ObjectHelperFactory.fetchPromises;

        $scope.friendsWhoPlayTitleLoc = UtilFactory.getLocalizedStr($scope.friendswhoplaystr, CONTEXT_NAMESPACE, 'friendswhoplaystr');
        $scope.friendsPlayingTitleLoc = UtilFactory.getLocalizedStr($scope.friendsplayingstr, CONTEXT_NAMESPACE, 'friendsplayingstr');

        $scope.showMoreLoc = UtilFactory.getLocalizedStr($scope.showmorestr, CONTEXT_NAMESPACE, 'showmorestr');
        $scope.showLessLoc = UtilFactory.getLocalizedStr($scope.showlessstr, CONTEXT_NAMESPACE, 'showlessstr');
        $scope.noFriendsCtaTitleLoc = UtilFactory.getLocalizedStr($scope.nofriendsctatitlestr, CONTEXT_NAMESPACE, 'nofriendsctatitlestr');
        $scope.noFriendsCtaDescLoc = UtilFactory.getLocalizedStr($scope.nofriendsctadescstr, CONTEXT_NAMESPACE, 'nofriendsctadescstr');
        $scope.noFriendsCtaButtonLoc = UtilFactory.getLocalizedStr($scope.nofriendsctabuttonstr, CONTEXT_NAMESPACE, 'nofriendsctabuttonstr');

        $scope.friendsWhoPlay = [];
        $scope.friendsPlayingLimit = $scope.friendsWhoPlayLimit = $scope.SHOW_FRIENDS_LIMIT = SHOW_FRIENDS_LIMIT;
        $scope.loading = true;

        /**
          * attaches $scope.friendslist to the data
          * @param {Array} playerslist - the list of friends who play
          * 
          * @method setFriendsList
          */
        function setFriendsList(playerslist) {
            $scope.loading = false;
            $scope.friendsWhoPlay = playerslist;
            $scope.$digest();
        }
        
        $scope.findFriends = function() {
            FindFriendsFactory.findFriends();
        };

        CommonGamesFactory
            .getUsersWhoOwn($scope.offerId)
            .then(mapWith(getRosterUser))
            .then(fetchPromises)
            .then(setFriendsList);

        /**
          * attaches the "friends playing" observable to $scope.friendslist
          * @param {Object} observable
          * 
          * @method attachToScope
          */
        function attachToScope(observable) {
            ObserverFactory.create(observable)
                .getProperty('friends')
                .attachToScope($scope, 'friendsPlaying');
        }

        RosterDataFactory.getFriendsPlayingObservable($scope.offerId).then(attachToScope);

    }

    /**
      * directive function 
      * @param {Object} ComponentsConfigFactory
      * @return {Object} directive object
      * 
      * @method originGamelibraryOgdFriendswhoplay
      */
    function originGamelibraryOgdFriendswhoplay(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            controller: 'OriginGamelibraryOgdFriendswhoplayCtrl',
            scope: {
                offerId: '@offerid',
                friendswhoplaystr: '@',
                friendsplayingstr: '@',
                showmorestr: '@',
                showlessstr: '@',
                nofriendsctatitlestr: '@',
                nofriendsctadescstr: '@',
                nofriendsctabuttonstr: '@'
            },                
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdfriendswhoplay.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdFriendswhoplay
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid of the game
     * @param {LocalizedString} friendswhoplaystr "Friends Who Play (%number%)"
     * @param {LocalizedString} friendsplayingstr "Friends Playing Now (%number%)"
     * @param {LocalizedString} showmorestr "Show More"
     * @param {LocalizedString} showlessstr "Show Less"  
     * @param {LocalizedString} nofriendsctatitlestr "Gaming is better with friends"
     * @param {LocalizedString} nofriendsctadescstr "Build your friends list to chat with friends, see what they're playing, join multiplayer games, and more."
     * @param {LocalizedString} nofriendsctabuttonstr "Find Friends"
     *
     *
     * @description
     *
     * OGD - Friends playing tab
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-ogd-friendswhoplay offerid='OFR-ID-1234'
     *             friendswhoplaystr="Friends Who Play (%number%)"
     *             friendsplayingstr="Friends Playing Now (%number%)"
     *             showmorestr="Show More"
     *             showlessstr="Show Less"
     *             nofriendsctatitlestr="Gaming is better with friends"
     *             nofriendsctadescstr="Build your friends list to chat with friends, see what they're playing, join multiplayer games, and more."
     *             nofriendsctabuttonstr="Find Friends"     
     *         ></origin-gamelibrary-ogd-friendswhoplay>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryOgdFriendswhoplayCtrl', OriginGamelibraryOgdFriendswhoplayCtrl)
        .directive('originGamelibraryOgdFriendswhoplay', originGamelibraryOgdFriendswhoplay);
}());

