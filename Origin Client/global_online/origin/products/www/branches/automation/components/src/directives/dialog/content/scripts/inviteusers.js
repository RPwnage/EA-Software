/**
 * @file dialog/content/inviteusers.js
 */
(function () {

    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-inviteusers';

    function OriginDialogContentInviteusersCtrl($scope, UtilFactory, RosterDataFactory, DialogFactory, ObserverFactory, FindFriendsFactory) {
        $scope.filterPlaceholderLoc = UtilFactory.getLocalizedStr($scope.filterPlaceholderStr, CONTEXT_NAMESPACE, 'filterplaceholderstr');
        $scope.titleLoc = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'titlestr').replace('%gamename%', $scope.gametitlestr);
        $scope.friendsOfflineLoc = UtilFactory.getLocalizedStr($scope.friendsOfflineStr, CONTEXT_NAMESPACE, 'friendsofflinestr');
        $scope.noFriendsLoc = UtilFactory.getLocalizedStr($scope.noFriendsStr, CONTEXT_NAMESPACE, 'nofriendsstr');
        $scope.userList = [];
        
        $scope.filterByNameAndPlatform = function(element) {
            var isNonEmptyName = !!element.originId && (element.originId.length > 0),
                isCompatiblePlatform = angular.isDefined(element.isVoiceSupported) && (element.isVoiceSupported === Origin.voice.supported());
            return isNonEmptyName && isCompatiblePlatform;
        };

        RosterDataFactory.getRoster('ONLINE').then(function (roster) {
            $scope.roster = roster;
            $scope.$digest();
        });

        this.addUser = function (nucleusId) {
            $scope.userList.push(nucleusId);
            // enable/disable the Invite button
            DialogFactory.update({ acceptEnabled: $scope.userList.length });
        };

        this.removeUser = function (nucleusId) {
            var index = $scope.userList.indexOf(nucleusId);
            if (index > -1) {
                $scope.userList.splice(index, 1);
                // enable/disable the Invite button
                DialogFactory.update({ acceptEnabled: $scope.userList.length });
            }
        };

        this.isSelected = function (nucleusId) {
            var index = $scope.userList.indexOf(nucleusId);
            if (index > -1) {
                return true;
            }
            return false;
        };

        this.onFindFriends = function () {
            DialogFactory.close('origin-dialog-content-invite-users-id');

            // Show the Find Friends dialog
            FindFriendsFactory.findFriends();
        };

        $scope.clearFilter = function () {
            $scope.filterText = '';
        };

        // Watch for friends being online
        ObserverFactory.create(RosterDataFactory.getOnlineCountWatch())
            .attachToScope($scope, 'onlineCount');

        // Watch our total friend count
        var rosterStatusObserver = ObserverFactory.create(RosterDataFactory.getRosterStatusWatch());
        rosterStatusObserver.attachToScope($scope, 'rosterLoadStatus');
    }

    function originDialogContentInviteusers(ComponentsConfigFactory) {
        function originDialogContentInviteusersLink(scope, element, attrs, ctrl) {
            var thisCtrl = ctrl[1],
                promptCtrl = ctrl[0];

            // Clicked the "find friends" hyperlink
            $(element).on('click', '.otka', function () {
                thisCtrl.onFindFriends();
            });

            promptCtrl.setContentDataFunc(function () {
                return {
                    userList: scope.userList
                };
            });
        }

        return {
            restrict: 'E',
            require: ['^originDialogContentPrompt', 'originDialogContentInviteusers'],
            scope: {
                gametitlestr: '@gametitlestr',
                filterPlaceholderStr: '@filterplaceholderstr',
                titleStr: '@titlestr',
                friendsofflinestr: '@friendsofflinestr',
                noFriendsStr: '@nofriendsstr'
            },
            controller: OriginDialogContentInviteusersCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/inviteusers.html'),
            link: originDialogContentInviteusersLink
        };

    }

    /**
    * @ngdoc directive
    * @name origin-components.directives:originDialogContentInviteusers
    * @restrict E
    * @element ANY
    * @scope
    * @param {string} gametitlestr The game title from catalog
    * @param {LocalizedString} filterplaceholderstr the filter friends heading
    * @param {LocalizedString} titlestr The title for the section that allows users to select friends to invite to a game
    * @param {LocalizedString} friendsofflinestr the text for the state where a user is currently offline
    * @param {LocalizedString} nofriendsstr The text to show when a user hasn't connected with any friends on Origin yet
    * @param {LocalizedString} invite-user-dialog-title-text Invite friends to join your game confirmation modal - this information is used by the invite friends factory
    * @param {LocalizedString} invite-user-dialog-invite-text Invite modal CTA - this information is used by the invite friends factory
    * @param {LocalizedString} invite-user-dialog-cancel-text Cancel modal CTA - this information is used by the invite friends factory
    * @param {LocalizedString} search-dialog-cancel-button-text canel search - this information is used by the friends search factory
    * @param {LocalizedString} search-dialog-description - search for friends description - this information is used by the friends search factory
    * @param {LocalizedString} search-dialog-input-placeholder-text search for friends subheading - this information is used by the friends search factory
    * @param {LocalizedString} search-dialog-search-button-text the seearch form cta - this information is used by the friends search factory
    * @param {LocalizedString} search-dialog-title the headline for the search for friends form - this information is used by the friends search factory
    * @description
    *
    * Invite friends dialog
    *
    * @example
    * <example module="origin-components">
    *     <file name="index.html">
    *         <origin-dialog-content-inviteusers
    *            gametitlestr="Battlefield"
    *            filterplaceholderstr="Filter Friends"
    *            titlestr="Select friends to invite them to your game of %gamename%"
    *            friendsofflinestr="Bummer! Looks like none of your friends are online right now."
    *            nofriendsstr="You don't have any friends on Origin yet. <a class=\"otka\">Find and add your friends</a>, and try again."
    *            invite-user-dialog-title-text="Invite friends to join your game"
    *            invite-user-dialog-invite-text="Invite"
    *            invite-user-dialog-cancel-text="Cancel"
    *            search-dialog-cancel-button-text="Cancel",
    *            search-dialog-description="Search for friends by their Public ID, real name or email address."
    *            search-dialog-input-placeholder-text="Search for friends"
    *            search-dialog-search-button-text="Search"
    *            search-dialog-title="Search for friends"
    *         ></origin-dialog-content-inviteusers>
    *     </file>
    * </example>
    */
    angular.module('origin-components')
        .controller('OriginDialogContentInviteusersCtrl', OriginDialogContentInviteusersCtrl)
        .directive('originDialogContentInviteusers', originDialogContentInviteusers);
}());
