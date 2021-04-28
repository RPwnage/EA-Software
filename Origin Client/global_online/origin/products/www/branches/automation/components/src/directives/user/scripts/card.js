/**
 * @file user/card.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-user-card';
    var VALID_NUCLEUS_ID = /^\d+$/;

    function OriginUserCardCtrl($scope, $sce, RosterDataFactory, ComponentsConfigHelper, UtilFactory, ConversationFactory, AppCommFactory, JoinGameFactory, ObserverFactory, NavigationFactory, NucleusIdObfuscateFactory) {
        var joinGameStrings = {
            unableToJoinText: UtilFactory.getLocalizedStr($scope.joinGameUnableTextStr, CONTEXT_NAMESPACE, 'joingameunabletext'),
            notOwnedText: UtilFactory.getLocalizedStr($scope.joinGameNotOwnedTextStr, CONTEXT_NAMESPACE, 'joingamenotownedtext'),
            viewInStoreText: UtilFactory.getLocalizedStr($scope.joinGameViewInStoreButtonTextStr, CONTEXT_NAMESPACE, 'joingameviewinstorebuttontext'),
            closeText: UtilFactory.getLocalizedStr($scope.joinGameCloseButtonTextStr, CONTEXT_NAMESPACE, 'joingameclosebuttontext'),
            notInstalledText: UtilFactory.getLocalizedStr($scope.joinGameNotInstalledTextStr, CONTEXT_NAMESPACE, 'joingamenotinstalledtext'),
            downloadText: UtilFactory.getLocalizedStr($scope.joinGameDownloadButtonTextStr, CONTEXT_NAMESPACE, 'joingamedownloadbuttontext')
        };

        $scope.presence = {};
        $scope.messageString = UtilFactory.getLocalizedStr($scope.messageStr, CONTEXT_NAMESPACE, 'messagecta');
        $scope.watchGameString = UtilFactory.getLocalizedStr($scope.watchGameStr, CONTEXT_NAMESPACE, 'watchgamecta');
        $scope.joinGameString = UtilFactory.getLocalizedStr($scope.joinGameStr, CONTEXT_NAMESPACE, 'joingamecta');
        $scope.joinGroupString = UtilFactory.getLocalizedStr($scope.joinGameStr, CONTEXT_NAMESPACE, 'joingroupcta');
        $scope.joinGameAndGroupString = UtilFactory.getLocalizedStr($scope.joinGameStr, CONTEXT_NAMESPACE, 'joingameandgroupcta');
        $scope.watchOrJoinGameString = UtilFactory.getLocalizedStr(
            $scope.watchOrJoinGameStr,
            CONTEXT_NAMESPACE,
            'watchorjoingame', {
                '%joingame%': "<a href='javascript:void(0)' ng-click='joinGame()' ng-if='presence.isJoinable && !presence.isGroupJoinable' class='otka'>{{::joinGameString}}</a><a href='javascript:void(0)' ng-click='joinGame()' ng-if='!presence.isJoinable && presence.isGroupJoinable' class='otka'>{{joinGroupString}}</a><a href='javascript:void(0)' ng-click='joinGame()' ng-if='presence.isJoinable && presence.isGroupJoinable' class='otka'>{{joinGameAndGroupString}}</a>",
                '%watch%': "<a ng-href='{{presence.broadcastingUrl}}' ng-click='openLink($event, presence.broadcastingUrl)' class='otka'>{{::watchGameString}}</a>"
            }
        );

        $scope.isPrivate = !VALID_NUCLEUS_ID.test($scope.nucleusId);
        $scope.privateProfileString = UtilFactory.getLocalizedStr($scope.privateProfileString,
            CONTEXT_NAMESPACE, 'privateprofilestring');

        $scope.enableSocialFeatures = ComponentsConfigHelper.enableSocialFeatures() && (!$scope.isPrivate);
        $scope.openLink = function($event, href){
            $event.preventDefault();
            NavigationFactory.openLink(href);
        };


        RosterDataFactory.getRosterUser($scope.nucleusId).then(function(user) {
            if (!!user) {
                ObserverFactory.create(user._presence)
                    .defaultTo({
                        jid: $scope.nucleusId,
                        presenceType: '',
                        show: 'UNAVAILABLE',
                        gameActivity: {},
                        groupActivity: {},
                        isBroadcasting: false,
                        isJoinable: false,
                        isGroupJoinable: false
                    })
                    .attachToScope($scope, 'presence');

                ObserverFactory.create(user.name)
                    .defaultTo({
                        firstName: '',
                        lastName: '',
                        originId: ''
                    })
                    .getProperty('originId')
                    .attachToScope($scope, 'originId');
            }
        });

        $scope.trustAsHtml = function (html) {
            return $sce.trustAsHtml(html);
        };


        function buildCurrentlyPlayingGameTitleHtml(gameTitle){
            return '<strong class="otktitle-4-strong">' + gameTitle + '</strong>';
        }

        $scope.getCurrentlyPlayingString = function(presence) {
            var currentlyPlaying = '';
            if (presence.show === 'INGAME') {
                currentlyPlaying = UtilFactory.getLocalizedStr($scope.currentlyPlayingStr, CONTEXT_NAMESPACE, 'currentlyplaying', {
                    '%game%': buildCurrentlyPlayingGameTitleHtml(presence.gameActivity.title)
                });
            }

            if (presence.isBroadcasting) {
                currentlyPlaying = UtilFactory.getLocalizedStr($scope.currentlyBroadcastingStr, CONTEXT_NAMESPACE, 'currentlybroadcasting', {
                    '%game%': buildCurrentlyPlayingGameTitleHtml(presence.gameActivity.title)
                });
            }

            return currentlyPlaying;
        };

        /**
         * go to the user's profile
         * @return {void}
         * @method
         */
        $scope.loadProfile = function() {
            NavigationFactory.goUserProfile($scope.nucleusId);
        };

        NucleusIdObfuscateFactory.encode($scope.nucleusId).then(function(encodedId){
            $scope.userProfileUrl = NavigationFactory.getAbsoluteUrl('app.profile.user', {
                'id': encodedId.id
            });
        });

        /**
         * Function that should be used to build out request to load a create message dialog
         * @return {void}
         * @method
         */
        $scope.createMessage = function() {
            ConversationFactory.openConversation($scope.nucleusId);
        };

        /**
         * Join your friend's game
         * @return {void}
         * @method
         */
        $scope.joinGame = function() {
            JoinGameFactory.joinFriendsGame(
                $scope.nucleusId,
                $scope.presence.gameActivity.productId,
                $scope.presence.gameActivity.multiplayerId,
                $scope.presence.gameActivity.title,
                joinGameStrings
            );
        };
    }

    function originUserCard(ComponentsConfigFactory, $compile, $timeout) {

        function originUserCardLink(scope, element) {

            /**
             * Show the watch or join cta
             * @method showWatchOrJoinCta
             * @return {void}
             */
            function showWatchOrJoinCta() {
                if (scope.presence.isBroadcasting && scope.presence.isJoinable && scope.enableSocialFeatures) {
                    $timeout(function() {
                        var elm = $compile(scope.watchOrJoinGameString)(scope),
                            parent = element[0].querySelector('.origin-usercard-ctas');
                        if (parent && !parent.innerHTML) {
                            angular.element(parent).append(elm);
                        }
                        scope.$digest();
                    }, 0, false);
                }
            }

            scope.$watch('presence.isBroadcasting', showWatchOrJoinCta);
            scope.$watch('presence.isJoinable', showWatchOrJoinCta);
        }


        return {
            restrict: 'E',
            scope: {
                'nucleusId': '@nucleusid',
                'offerId': '@offerid',
                'messageStr': '@messagecta',
                'watchGameStr': '@watchgamecta',
                'joinGameStr': '@joingamecta',
                'joinGroupStr': '@joingroupcta',
                'joinGameAndGroupStr': '@joingameandgroupcta',
                'watchOrJoinGameStr': '@watchorjoingame',
                'currentlyPlayingStr': '@currentlyplaying',
                'currentlyBroadcastingStr': '@currentlybroadcasting',
                'joinGameCloseButtonTextStr': '@joingameclosebuttontext',
                'joinGameDownloadButtonTextStr': '@joingamedownloadbuttontext',
                'joinGameNotInstalledTextStr': '@joingamenotinstalledtext',
                'joinGameUnableTextStr': '@joingameunabletext',
                'joinGameNotOwnedTextStr': '@joingamenotownedtext',
                'joinGameViewInStoreButtonTextStr': '@joingameviewinstorebuttontext',
                'privateProfileString' : '@privateProfileString'
            },
            controller: 'OriginUserCardCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('user/views/card.html'),
            link: originUserCardLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originUserCard
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} nucleusid the nucleusId of the user
     * @param {string} offerid the offerid of the game
     * @param {LocalizedString} messagecta text on button that messages user
     * @param {LocalizedString} watchgamecta text on button that watches game
     * @param {LocalizedString} joingroupcta  text on button that allows joining a group
     * @param {LocalizedString} joingameandgroupcta  text on button that allows joining a game and group
     * @param {LocalizedString} watchorjoingame text on button that watches game or joins game
     * @param {LocalizedString} currentlyplaying the game a user is playing
     * @param {LocalizedString} currentlybroadcasting the game a user is broadcasting
     * @param {LocalizedString} joingamecta * merchandized in defaults
     * @param {LocalizedString} joingameclosebuttontext * merchandized in defaults
     * @param {LocalizedString} joingamedownloadbuttontext * merchandized in defaults
     * @param {LocalizedString} joingamenotinstalledtext * merchandized in defaults
     * @param {LocalizedString} joingameunabletext * merchandized in defaults
     * @param {LocalizedString} joingamenotownedtext * merchandized in defaults
     * @param {LocalizedString} joingameviewinstorebuttontext * merchandized in defaults
     * @param {LocalizedString} privateprofilestring * merchandized in defaults show when user profile is private
     *
     * @description
     *
     * user card
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-user-card nucleusid="12305105408" offerid="OFB-EAST:109549060"></origin-user-card>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginUserCardCtrl', OriginUserCardCtrl)
        .directive('originUserCard', originUserCard);
}());
