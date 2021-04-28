/**
 * @file user/info.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-user-info';

    function OriginUserInfoCtrl($scope, $timeout, RosterDataFactory, UtilFactory, ComponentsLogFactory, ComponentsConfigHelper, AppCommFactory, ConversationFactory, DialogFactory, JoinGameFactory, NavigationFactory, NucleusIdObfuscateFactory) {
        $scope.originId = '';
        $scope.messageLoc = UtilFactory.getLocalizedStr($scope.messageStr, CONTEXT_NAMESPACE, 'messagecta');
        $scope.joinGameLoc = UtilFactory.getLocalizedStr($scope.joinGameStr, CONTEXT_NAMESPACE, 'joingamecta');
        $scope.isJoinable = false;

        $scope.isSocialEnabled = ComponentsConfigHelper.enableSocialFeatures();
        $scope.hasValidNucleusId = UtilFactory.isValidNucleusId($scope.nucleusId);

        $scope.joinGameStrings = {
            unableToJoinText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'join-game-unable-text'),
            notOwnedText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'join-game-not-owned-text'),
            viewInStoreText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'join-game-view-in-store-button-text'),
            closeText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'join-game-close-button-text'),
            notInstalledText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'join-game-not-installed-text'),
            downloadText: UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'join-game-download-button-text')
        };


        /**
         * go to the user's profile
         * @return {void}
         * @method loadProfile
         */
        $scope.loadProfile = function($event) {
            $event.preventDefault();
            NavigationFactory.goUserProfile($scope.nucleusId);
            DialogFactory.close('web-show-friends-modal');
        };

        NucleusIdObfuscateFactory.encode($scope.nucleusId).then(function(encodedId){
            $scope.userProfileUrl = NavigationFactory.getAbsoluteUrl('app.profile.user', {
                'id': encodedId.id
            });
        });

        /**
         * @return {void}
         * @method closeDialog
         */
        $scope.closeDialog = function() {
            DialogFactory.close('web-show-friends-modal');
        };

        /**
         * open chat message dialog
         * @return {void}
         * @method createMessage
         */
        $scope.createMessage = function() {
            ConversationFactory.openConversation($scope.nucleusId);
            DialogFactory.close('web-show-friends-modal');
        };

        /**
         * Join game
         * @return {void}
         * @method joinGame
         */
        $scope.joinGame = function() {
            if (!!$scope.gameActivity) {
                JoinGameFactory.joinFriendsGame(
                    $scope.nucleusId,
                    $scope.gameActivity.productId,
                    $scope.gameActivity.multiplayerId,
                    $scope.gameActivity.title,
                    $scope.joinGameStrings
                );
            }
            DialogFactory.close('web-show-friends-modal');
        };

        /**
         * logs any errors from the getFriendInfo call
         * @param  {Error} error the error object
         */
        function handleGetFriendInfoError(error) {
            ComponentsLogFactory.error('[origin-user-info][getUserInfo]', error);
        }

        /**
         * take the results of the get friend info an populate the bindings
         * @param  {object} friendInfo the results of the getFriendInfo call
         */
        function populateBindings(friendInfo) {
            $scope.originId = friendInfo.originId;
            $scope.gameActivity = friendInfo.presence.gameActivity;
            var isPlayingGame = (Object.keys(friendInfo.presence.gameActivity).length && !!friendInfo.presence.gameActivity.title);
            if (isPlayingGame) {
                $scope.isJoinable = friendInfo.presence.gameActivity.joinable;
                $scope.currentlyPlayingLoc = UtilFactory.getLocalizedStr($scope.currentlyPlayingStr, CONTEXT_NAMESPACE, 'currentlyplaying', {
                    '%game%': '<strong>' + friendInfo.presence.gameActivity.title + '</strong>'
                });
            }
            $scope.$digest();
        }
        /**
         * Get the user's info and bind it
         * @return {void}
         * @method getUserInfo
         */
        this.getUserInfo = function() {
            RosterDataFactory.getFriendInfo($scope.nucleusId)
                .then(populateBindings)
                .catch(handleGetFriendInfoError);
        };

    }

    function originUserInfo(ComponentsConfigFactory) {

        function originUserInfoLink(scope, element, args, ctrl) {
            ctrl.getUserInfo();
        }

        return {
            restrict: 'E',
            scope: {
                'nucleusId': '@nucleusid',
                'messageStr': '@messagecta',
                'joinGameStr': '@joingamecta',
                'joinGroupStr': '@joingroupcta',
                'joinGameAndGroupStr': '@joingameandgroupcta',
                'currentlyPlayingStr': '@currentlyplaying'
            },
            controller: 'OriginUserInfoCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('user/views/info.html'),
            link: originUserInfoLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originUserInfo
     * @restrict E
     * @element ANY
     * @param {string} nucleusid the nucleusId of the user
     * @param {LocalizedString} messagecta text on button that messages user
     * @param {LocalizedString} joingamecta text on button that joins game
     * @param {LocalizedString} joingroupcta text on button that joins group
     * @param {LocalizedString} joingameandgroupcta text on button that joins game and group
     * @param {LocalizedString} currentlyplaying the game a user is playing
     * @scope
     * @description
     *
     * userinfo
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-user-info nucleusid="12305105408"></origin-user-info>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginUserInfoCtrl', OriginUserInfoCtrl)
        .directive('originUserInfo', originUserInfo);
}());
