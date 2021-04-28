(function () {

    'use strict';

    var CONTEXT_NAMESPACE = 'origin-profile-header',
        AVATAR_UPDATE_THROTTLE_MS = 500;

    /**
    * The controller
    */
    function OriginProfileHeaderCtrl($scope, UserDataFactory, RosterDataFactory, UtilFactory, AppCommFactory, DialogFactory, AuthFactory, ComponentsConfigFactory, NavigationFactory, SocialHubFactory, SubscriptionFactory, ComponentsLogFactory, ReportUserFactory, ObserverFactory, ComponentsConfigHelper) {
        $scope.onlineLoc = UtilFactory.getLocalizedStr($scope.onlineStr, CONTEXT_NAMESPACE, 'onlinestr');
        $scope.awayLoc = UtilFactory.getLocalizedStr($scope.awayStr, CONTEXT_NAMESPACE, 'awaystr');
        $scope.offlineLoc = UtilFactory.getLocalizedStr($scope.offlineStr, CONTEXT_NAMESPACE, 'offlinestr');
        $scope.invisibleLoc = UtilFactory.getLocalizedStr($scope.invisibleStr, CONTEXT_NAMESPACE, 'invisiblestr');
        $scope.playingLoc = UtilFactory.getLocalizedStr($scope.playingStr, CONTEXT_NAMESPACE, 'playingstr');
        $scope.broadcastingLoc = UtilFactory.getLocalizedStr($scope.broadcastingStr, CONTEXT_NAMESPACE, 'broadcastingstr');
        $scope.editoneadotcomLoc = UtilFactory.getLocalizedStr($scope.editoneadotcomStr, CONTEXT_NAMESPACE, 'editoneadotcomstr');
        $scope.sendfriendrequestLoc = UtilFactory.getLocalizedStr($scope.sendfriendrequestStr, CONTEXT_NAMESPACE, 'sendfriendrequeststr');
        $scope.friendrequestsentLoc = UtilFactory.getLocalizedStr($scope.friendrequestsentStr, CONTEXT_NAMESPACE, 'friendrequestsentstr');
        $scope.acceptFriendRequestLoc = UtilFactory.getLocalizedStr($scope.acceptfriendrequestStr, CONTEXT_NAMESPACE, 'acceptfriendrequeststr');
        $scope.ignoreFriendRequestLoc = UtilFactory.getLocalizedStr($scope.ignorefriendrequestStr, CONTEXT_NAMESPACE, 'ignorefriendrequeststr');
        $scope.unfriendLoc = UtilFactory.getLocalizedStr($scope.unfriendStr, CONTEXT_NAMESPACE, 'unfriendstr');
        $scope.blockandunfriendLoc = UtilFactory.getLocalizedStr($scope.blockandunfriendStr, CONTEXT_NAMESPACE, 'blockandunfriendstr');
        $scope.blockuserLoc = UtilFactory.getLocalizedStr($scope.blockuserStr, CONTEXT_NAMESPACE, 'blockuserstr');
        $scope.unblockuserLoc = UtilFactory.getLocalizedStr($scope.unblockuserStr, CONTEXT_NAMESPACE, 'unblockuserstr');
        $scope.reportuserLoc = UtilFactory.getLocalizedStr($scope.reportuserStr, CONTEXT_NAMESPACE, 'reportuserstr');
        $scope.unfriendusertitleLoc = UtilFactory.getLocalizedStr($scope.unfriendusertitleStr, CONTEXT_NAMESPACE, 'unfriendusertitlestr');
        $scope.unfrienduserdescriptionLoc = UtilFactory.getLocalizedStr($scope.unfrienduserdescriptionStr, CONTEXT_NAMESPACE, 'unfrienduserdescriptionstr');
        $scope.unfrienduseracceptLoc = UtilFactory.getLocalizedStr($scope.unfrienduseracceptStr, CONTEXT_NAMESPACE, 'unfrienduseracceptstr');
        $scope.unfrienduserrejectLoc = UtilFactory.getLocalizedStr($scope.unfrienduserrejectStr, CONTEXT_NAMESPACE, 'unfrienduserrejectstr');
        $scope.errorfriendnotaddedtitleLoc = UtilFactory.getLocalizedStr($scope.errorfriendnotaddedtitle, CONTEXT_NAMESPACE, 'errorfriendnotaddedtitle');
        $scope.errorfriendnotaddeddescriptionLoc = UtilFactory.getLocalizedStr($scope.errorfriendnotaddeddescription, CONTEXT_NAMESPACE, 'errorfriendnotaddeddescription');
        $scope.errorfriendnotaddedacceptLoc = UtilFactory.getLocalizedStr($scope.errorfriendnotaddedaccept, CONTEXT_NAMESPACE, 'errorfriendnotaddedaccept');
        $scope.errorfriendnotaddedcancelLoc = UtilFactory.getLocalizedStr($scope.errorfriendnotaddedcancel, CONTEXT_NAMESPACE, 'errorfriendnotaddedcancel');
        $scope.cancelandblockuserLoc = UtilFactory.getLocalizedStr($scope.cancelandblockuser, CONTEXT_NAMESPACE, 'cancelandblockuser');
        $scope.ignoreandblockuserLoc = UtilFactory.getLocalizedStr($scope.ignoreandblockuser, CONTEXT_NAMESPACE, 'ignoreandblockuser');

        $scope.cancelandblocktitleLoc = UtilFactory.getLocalizedStr($scope.cancelandblocktitle, CONTEXT_NAMESPACE, 'cancelandblocktitle');
        $scope.cancelandblockdescriptionLoc = UtilFactory.getLocalizedStr($scope.cancelandblockdescription, CONTEXT_NAMESPACE, 'cancelandblockdescription');
        $scope.cancelandblockdescription2Loc = UtilFactory.getLocalizedStr($scope.cancelandblockdescriptiontwoStr, CONTEXT_NAMESPACE, 'cancelandblockdescriptiontwostr');
        $scope.changeminddescriptionLoc = UtilFactory.getLocalizedStr($scope.changeminddescription, CONTEXT_NAMESPACE, 'changeminddescription');
        $scope.cancelandblockacceptLoc = UtilFactory.getLocalizedStr($scope.cancelandblockaccept, CONTEXT_NAMESPACE, 'cancelandblockaccept');
        $scope.cancelandblockdeclineLoc = UtilFactory.getLocalizedStr($scope.cancelandblockdecline, CONTEXT_NAMESPACE, 'cancelandblockdecline');
        $scope.ignoreandblocktitleLoc = UtilFactory.getLocalizedStr($scope.ignoreandblocktitle, CONTEXT_NAMESPACE, 'ignoreandblocktitle');
        $scope.ignoreandblockdescriptionLoc = UtilFactory.getLocalizedStr($scope.ignoreandblockdescription, CONTEXT_NAMESPACE, 'ignoreandblockdescription');
        $scope.ignoreandblockdescription2Loc = UtilFactory.getLocalizedStr($scope.ignoreandblockdescriptiontwoStr, CONTEXT_NAMESPACE, 'ignoreandblockdescriptiontwostr');
        $scope.ignoreandblockacceptLoc = UtilFactory.getLocalizedStr($scope.ignoreandblockaccept, CONTEXT_NAMESPACE, 'ignoreandblockaccept');
        $scope.ignoreandblockdeclineLoc = UtilFactory.getLocalizedStr($scope.ignoreandblockdecline, CONTEXT_NAMESPACE, 'ignoreandblockdecline');

        $scope.hasSubscription = false;

        $scope.avatarImgSrc = ComponentsConfigFactory.getImagePath('profile\\avatar-placeholder.png');

        $scope.optionsContextmenuVisible = false;
        $scope.contextmenuCallbacks = {};

        function setUpObservers(user) {
            ObserverFactory.create(user.subscription)
                .defaultTo({state: 'NONE', requestSent: false, relationship: 'NON_FRIEND'})
                .attachToScope($scope, 'subscription');
            ObserverFactory.create(user._presence)
            .defaultTo({jid: $scope.nucleusId, presenceType: '', show: 'UNAVAILABLE', gameActivity: {}})
            .attachToScope($scope, 'presence');
        }


        RosterDataFactory.getRosterUser($scope.nucleusId).then(function(user) {
            if (!!user) {
                RosterDataFactory.isBlocked(user.nucleusId).then(function(value) {
                    if (value) {
                        user.subscription.data.relationship = 'BLOCKED';
                    }
                    return user;
                }).then(setUpObservers);
            }
        });

        /**
         * Update images when new avatar is received
         * @param {string} avatarUrl
         */
        function updateAvatarImages(avatarUrl) {
            $scope.avatarImgSrc = avatarUrl;
            $scope.backgroundImage = avatarUrl;
            $scope.$digest();
        }

        /**
         * Redirect to error page if call failed
         */
        function handleAvatarUpdateFailure() {
            NavigationFactory.goToState('app.error_notfound');
        }

        /**
         * Log error on exceptions
         */
        function handleAvatarUpdateException(error) {
            ComponentsLogFactory.error('OriginProfileHeaderCtrl: UserDataFactory.getAvatar failed', error);
        }

        /**
         * Update user avatar from UserDataFactory
         */
        function getUserAvatar() {
            UserDataFactory.getAvatar($scope.nucleusId, Origin.defines.avatarSizes.LARGE)
                .then(updateAvatarImages, handleAvatarUpdateFailure)
                .catch(handleAvatarUpdateException);
        }

        /**
         * Update user avatar from API
         */
        function getUserAvatarFromServer() {
            UserDataFactory.getAvatarFromServer($scope.nucleusId, Origin.defines.avatarSizes.LARGE)
                .then(updateAvatarImages, handleAvatarUpdateFailure)
                .catch(handleAvatarUpdateException);
        }
        var updateUserAvatarThrottled = _.throttle(getUserAvatarFromServer, AVATAR_UPDATE_THROTTLE_MS); // Throttled version so we don't query too frequently


        function getUserInfo() {

            // If the nucleusId was not specified view current user's profile
            if ($scope.nucleusId === '') {
                $scope.nucleusId = Origin.user.userPid();
            }

            if (Number($scope.nucleusId) === Number(Origin.user.userPid())) {
                $scope.hasSubscription = SubscriptionFactory.userHasSubscription();
            }

            // Get username, first name and last name
		    Origin.atom.atomUserInfoByUserIds([$scope.nucleusId])
                .then(function (response) {
                    $scope.user = response[0];
                    $scope.$digest();
                }).catch(function (error) {
                    ComponentsLogFactory.error('OriginProfileHeaderCtrl: Origin.atom.atomUserInfoByUserIds failed', error);
                });

            // Get the avatar image
            getUserAvatar();

			// Force reload the blocking list, in case this has changed asynchronously
			RosterDataFactory.loadBlockList();
        }

        if (AuthFactory.isAppLoggedIn()) {
            getUserInfo();
        }

        $scope.removeFriendAndBlockDlgStrings = {};
        $scope.removeFriendAndBlockDlgStrings.title = UtilFactory.getLocalizedStr($scope.unfriendandblocktitle, CONTEXT_NAMESPACE, 'unfriendandblocktitle');
        $scope.removeFriendAndBlockDlgStrings.description = UtilFactory.getLocalizedStr($scope.unfriendandblockdescription, CONTEXT_NAMESPACE, 'unfriendandblockdescription');
        $scope.removeFriendAndBlockDlgStrings.description2 = UtilFactory.getLocalizedStr($scope.unfriendandblockdescriptiontwoStr, CONTEXT_NAMESPACE, 'unfriendandblockdescriptiontwostr');
        $scope.removeFriendAndBlockDlgStrings.description3 = UtilFactory.getLocalizedStr($scope.unfriendandblockdescriptionthreeStr, CONTEXT_NAMESPACE, 'unfriendandblockdescriptionthreestr');
        $scope.removeFriendAndBlockDlgStrings.acceptText = UtilFactory.getLocalizedStr($scope.unfriendandblockaccept, CONTEXT_NAMESPACE, 'unfriendandblockaccept');
        $scope.removeFriendAndBlockDlgStrings.rejectText = UtilFactory.getLocalizedStr($scope.unfriendandblockreject, CONTEXT_NAMESPACE, 'unfriendandblockreject');

        $scope.blockDlgStrings = {};
        $scope.blockDlgStrings.title = UtilFactory.getLocalizedStr($scope.blocktitle, CONTEXT_NAMESPACE, 'blocktitle');
        $scope.blockDlgStrings.description = UtilFactory.getLocalizedStr($scope.blockdescription, CONTEXT_NAMESPACE, 'blockdescriptionstr');
        $scope.blockDlgStrings.description2 = UtilFactory.getLocalizedStr($scope.blockdescriptiontwoStr, CONTEXT_NAMESPACE, 'blockdescriptiontwostr');
        $scope.blockDlgStrings.description3 = UtilFactory.getLocalizedStr($scope.blockdescriptionthreeStr, CONTEXT_NAMESPACE, 'blockdescriptionthreestr');
        $scope.blockDlgStrings.acceptText = UtilFactory.getLocalizedStr($scope.blockaccept, CONTEXT_NAMESPACE, 'blockaccept');
        $scope.blockDlgStrings.rejectText = UtilFactory.getLocalizedStr($scope.blockreject, CONTEXT_NAMESPACE, 'blockreject');

        this.handleBlockUser = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleBlockUser);
            if (result.accepted) {
                RosterDataFactory.blockUser($scope.nucleusId);
            }
        };

        this.handleCancelAndBlockUser = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleCancelAndBlockUser);
            if (result.accepted) {
                RosterDataFactory.cancelAndBlockUser($scope.nucleusId);
            }
        };

        this.handleIgnoreAndBlockUser = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleIgnoreAndBlockUser);
            if (result.accepted) {
                RosterDataFactory.ignoreAndBlockUser($scope.nucleusId);
            }
        };

        this.handleUnfriendAndBlockUser = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleUnfriendAndBlockUser);
            if (result.accepted) {
                RosterDataFactory.removeFriendAndBlock($scope.nucleusId);
            }
        };

        function onClientAuthChanged(authChangeObject) {
            // reset nucleus ids and images when the user logs out
            if (!authChangeObject.isLoggedIn) {
                $scope.nucleusId = '';
                $scope.avatarImgSrc = ComponentsConfigFactory.getImagePath('profile\\avatar-placeholder.png');
                $scope.backgroundImage = undefined;
            } else {
                getUserInfo();

                $scope.$digest();
            }
        }

        // listen for auth changes
        AuthFactory.events.on('myauth:change', onClientAuthChanged);
        AuthFactory.events.on('myauth:ready', onClientAuthChanged);
        Origin.events.on(Origin.events.DIRTYBITS_AVATAR, updateUserAvatarThrottled);

        function onResize() {
            // If the window size changes close the context menu so the user can't resize it off the screen
            $scope.optionsContextmenuVisible = false;
            $scope.$digest();
        }

        function onDestroy() {
            AuthFactory.events.off('myauth:change', onClientAuthChanged);
            AuthFactory.events.off('myauth:ready', onClientAuthChanged);
            Origin.events.off(Origin.events.DIRTYBITS_AVATAR, updateUserAvatarThrottled);
            $(window).off('resize', onResize);
        }

        $scope.$on('$destroy', onDestroy);


        // Context Menu Functions
        this.toggleOptionsContextmenu = function () {
            $scope.optionsContextmenuVisible = !$scope.optionsContextmenuVisible;

            if ($scope.optionsContextmenuVisible) {
                // position the menu under the button
                var smallScreen = $(window).width() < 600;
                var left, top, right;
                var $header = $scope.$element.find('.origin-profile-header');
                var paddingRight = parseInt($header.css('padding-right'), 10);
                var paddingTop = parseInt($header.css('padding-top'));
                var $menuButton = $scope.$element.find('.origin-profile-header-controls-options-button');
                var $offsetParent = $menuButton.offsetParent();
                var $menu = $scope.$element.find('#origin-profile-header-controls-options-menu');
                var buttonPosition = $menuButton.position();
                var leftPositioned = $menuButton.css('float') === 'left'
                ;

                // when positioned to left of a button on small screens
                // ie: pending friend requests
                if (leftPositioned) {
                    left = buttonPosition.left + paddingRight + 12;
                    top = (buttonPosition.top + $menuButton.height()) + paddingTop + 40;
                    $menu.css({ 'left': left + 'px', 'top': top + 'px', 'right': 'auto' });
                }
                else {
                    if(smallScreen) {
                        right = ($offsetParent.width() - buttonPosition.left) + paddingRight;
                    } else {
                        right = ($offsetParent.width() - buttonPosition.left) + paddingRight + 70;
                    }
                    top = (buttonPosition.top + $menuButton.height()) + paddingTop + 8;
                    $menu.css({ 'right': right + 'px', 'top': top + 'px', 'left': 'auto' });
                }
            }

            $scope.$digest();
        };

        this.closeOptionsContextmenu = function (digest) {
            $scope.optionsContextmenuVisible = false;
            if(digest) { $scope.$digest(); }
        };

        $(window).resize(onResize);

        var self = this;
        this.handleRemoveFriend = function (result) {
            AppCommFactory.events.off('dialog:hide', self.handleRemoveFriend);
            if (result.accepted) {
                RosterDataFactory.removeFriend($scope.nucleusId);
                RosterDataFactory.updateRosterViewport();
            }
        };

        $scope.contextmenuCallbacks.ignoreFriendRequest = function () {
            self.closeOptionsContextmenu(false);

            RosterDataFactory.friendRequestReject($scope.nucleusId);
            RosterDataFactory.updateRosterViewport();
        };

        $scope.contextmenuCallbacks.unfriendUser = function() {

            self.closeOptionsContextmenu(false);
            DialogFactory.openPrompt({
                id: 'profile-header-removeFriend',
                title: $scope.unfriendusertitleLoc,
                description: $scope.unfrienduserdescriptionLoc.replace('%username%', $scope.user.EAID),
                acceptText: $scope.unfrienduseracceptLoc,
                acceptEnabled: true,
                rejectText: $scope.unfrienduserrejectLoc,
                defaultBtn: 'yes',
                callback: self.handleRemoveFriend
            });
        };

        $scope.contextmenuCallbacks.blockUser = function () {

            self.closeOptionsContextmenu(false);

            var description = '<strong>' + $scope.blockDlgStrings.description + '</strong>' + '<br><br>' +
                $scope.blockDlgStrings.description2 + '<br><br>' +
                $scope.blockDlgStrings.description3;
            description = description.replace(/%username%/g, $scope.user.EAID);

            DialogFactory.openPrompt({
                id: 'social-profile-header-blockUser',
                title: $scope.blockDlgStrings.title,
                description: description,
                acceptText: $scope.blockDlgStrings.acceptText,
                acceptEnabled: true,
                rejectText: $scope.blockDlgStrings.rejectText,
                defaultBtn: 'yes',
                callback: self.handleBlockUser
            });
        };

        $scope.contextmenuCallbacks.cancelAndBlockUser = function () {
            self.closeOptionsContextmenu(false);

            var description = '<strong>' + $scope.cancelandblockdescriptionLoc + '</strong>' + '<br><br>' +
                $scope.cancelandblockdescription2Loc + '<br><br>' +
                $scope.changeminddescriptionLoc;
            description = description.replace(/%username%/g, $scope.user.EAID);

            DialogFactory.openPrompt({
                id: 'profile-header-cancelRequestAndBlock-id',
                name: 'profile-header-cancelRequestAndBlock',
                title: $scope.cancelandblocktitleLoc,
                description: description,
                acceptText: $scope.cancelandblockacceptLoc,
                acceptEnabled: true,
                rejectText: $scope.cancelandblockdeclineLoc,
                defaultBtn: 'yes',
                callback: self.handleCancelAndBlockUser
            });
        };

        $scope.contextmenuCallbacks.ignoreAndBlockUser = function () {
            self.closeOptionsContextmenu(false);

            var description = '<strong>' + $scope.ignoreandblockdescriptionLoc + '</strong>' + '<br><br>' +
                $scope.ignoreandblockdescription2Loc + '<br><br>' +
                $scope.changeminddescriptionLoc;
            description = description.replace(/%username%/g, $scope.user.EAID);

            DialogFactory.openPrompt({
                id: 'profile-header-ignoreRequestAndBlock-id',
                name: 'profile-header-ignoreRequestAndBlock',
                title: $scope.ignoreandblocktitleLoc,
                description: description,
                acceptText: $scope.ignoreandblockacceptLoc,
                acceptEnabled: true,
                rejectText: $scope.ignoreandblockdeclineLoc,
                defaultBtn: 'yes',
                callback: self.handleIgnoreAndBlockUser
            });
        };

        $scope.contextmenuCallbacks.blockAndUnfriendUser = function () {

            self.closeOptionsContextmenu(false);

            var description = '<strong>' + $scope.removeFriendAndBlockDlgStrings.description + '</strong>' + '<br><br>' +
                $scope.removeFriendAndBlockDlgStrings.description2 + '<br><br>' +
                $scope.removeFriendAndBlockDlgStrings.description3;
            description = description.replace(/%username%/g, $scope.user.EAID);

            DialogFactory.openPrompt({
                id: 'social-hub-roster-removeFriendAndBlock',
                title: $scope.removeFriendAndBlockDlgStrings.title,
                description: description,
                acceptText: $scope.removeFriendAndBlockDlgStrings.acceptText,
                acceptEnabled: true,
                rejectText: $scope.removeFriendAndBlockDlgStrings.rejectText,
                defaultBtn: 'yes',
                callback: self.handleUnfriendAndBlockUser
            });
        };

        $scope.contextmenuCallbacks.unblockUser = function () {
            self.closeOptionsContextmenu(false);
            RosterDataFactory.unblockUser($scope.nucleusId);
        };


        $scope.contextmenuCallbacks.reportUser = function () {
            self.closeOptionsContextmenu(false);
            var currentUserPresence = RosterDataFactory.getCurrentUserPresence();

            ReportUserFactory.reportUser({
                    userid: $scope.user.userId,
                    username: $scope.user.EAID,
                    mastertitle: _.get(currentUserPresence,['gameActivity','masterTitle'])
                });
        };

        this.editSelfProfile = function () {
            var url = ComponentsConfigHelper.getUrl('eaAccounts');
            if (!Origin.client.isEmbeddedBrowser()){
                url = url.replace("{locale}", Origin.locale.locale());
                url = url.replace("{access_token}", "");
            }
            NavigationFactory.asyncOpenUrlWithEADPSSO(url);
        };

        this.sendFriendRequest = function () {
            RosterDataFactory.sendFriendRequest($scope.nucleusId);
        };

        this.handleErrorDialog = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleErrorDialog);
            if (result.accepted) {
                SocialHubFactory.unminimizeWindow();
                SocialHubFactory.focusWindow();
            }
        };

        this.acceptFriendRequest = function () {
            RosterDataFactory.friendRequestAccept($scope.nucleusId).then(function() {
                RosterDataFactory.deleteFromSectionRoster('INCOMING', $scope.nucleusId);

            }, function() {

                // Accept friend request failed. Display error dialog
                DialogFactory.openPrompt({
                    id: 'social-hub-roster-errorFriendNotAdded-id',
                    name: 'social-hub-roster-errorFriendNotAdded',
                    title: $scope.errorfriendnotaddedtitleLoc,
                    description: $scope.errorfriendnotaddeddescriptionLoc,
                    acceptText: $scope.errorfriendnotaddedacceptLoc,
                    acceptEnabled: true,
                    rejectText: $scope.errorfriendnotaddedcancelLoc,
                    defaultBtn: 'yes',
                    callback: self.handleErrorDialog
                });
            });
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfileHeader
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} nucleusid nucleusId of the user
     * @param {LocalizedString} onlinestr "ONLINE"
     * @param {LocalizedString} awaystr "AWAY"
     * @param {LocalizedString} offlinestr "OFFLINE"
     * @param {LocalizedString} invisiblestr "INVISIBLE"
     * @param {LocalizedString} playingstr "PLAYING:"
     * @param {LocalizedString} broadcastingstr "BROADCASTING:"
     * @param {LocalizedString} editoneadotcomstr "Edit on EA.com"
     * @param {LocalizedString} sendfriendrequeststr "Send Friend Request"
     * @param {LocalizedString} friendrequestsentstr "Friend Request Sent"
     * @param {LocalizedString} acceptfriendrequeststr "Accept Friend Request"
     * @param {LocalizedString} ignorefriendrequeststr "Ignore Friend Request"
     * @param {LocalizedString} unfriendstr "Unfriend"
     * @param {LocalizedString} blockandunfriendstr "Block & Unfriend"
     * @param {LocalizedString} blockuserstr "Block User"
     * @param {LocalizedString} unblockuserstr "Unblock User"
     * @param {LocalizedString} reportuserstr "Report User"
     * @param {LocalizedString} unfriendusertitlestr "Unfriend User"
     * @param {LocalizedString} unfrienduserdescriptionstr "Are you sure you want to unfriend %username%?"
     * @param {LocalizedString} unfrienduseracceptstr "Unfriend"
     * @param {LocalizedString} unfrienduserrejectstr "Cancel"
     * @param {LocalizedString} blocktitle title of the Block dialog
     * @param {LocalizedString} blockdescriptionstr Description of the Block dialog
     * @param {LocalizedString} blockdescriptiontwostr Description of the Block dialog
     * @param {LocalizedString} blockdescriptionthreestr Description of the Block dialog
     * @param {LocalizedString} blockaccept Accept prompt of the Block dialog
     * @param {LocalizedString} blockreject Reject prompt of the Block dialog
     * @param {LocalizedString} unfriendandblocktitle title of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockdescription Description of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockdescriptiontwostr Description of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockdescriptionthreestr Description of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockaccept Accept prompt of the Unfriend And Block dialog
     * @param {LocalizedString} unfriendandblockreject Reject prompt of the Unfriend And Block dialog
     * @param {LocalizedString} errorfriendnotaddedtitle Title of Too Many Friends error dialog
     * @param {LocalizedString} errorfriendnotaddedaccept Accept prompt of Too Many Friends error dialog
     * @param {LocalizedString} errorfriendnotaddeddescription Description of Too Many Friends error dialog
     * @param {LocalizedString} errorfriendnotaddedcancel Reject string Too Many Friends error dialog
     * @param {LocalizedString} cancelandblockuser "Cancel Friend Request and Block"
     * @param {LocalizedString} ignoreandblockuser "Ignore Friend Request and Block"
     * @param {LocalizedString} cancelandblocktitle desc
     * @param {LocalizedString} cancelandblockdescription desc
     * @param {LocalizedString} cancelandblockdescriptiontwostr desc
     * @param {LocalizedString} changeminddescription desc
     * @param {LocalizedString} cancelandblockaccept desc
     * @param {LocalizedString} cancelandblockdecline desc
     * @param {LocalizedString} ignoreandblocktitle desc
     * @param {LocalizedString} ignoreandblockdescription desc
     * @param {LocalizedString} ignoreandblockdescriptiontwostr desc
     * @param {LocalizedString} ignoreandblockaccept desc
     * @param {LocalizedString} ignoreandblockdecline desc
     *
     *
     * @param {LocalizedString} report-user-dialog-title-text report user dialog title text. Set up once as default. E.G., "Report User"
     * @param {LocalizedString} report-user-dialog-submit-text report user dialog submit text. Set up once as default. E.G., "Submit"
     * @param {LocalizedString} report-user-dialog-cancel-text report user dialog cancel text. Set up once as default. E.G., "Cancel"
     * @param {LocalizedString} report-user-dialog-close-text report user dialog close text. Set up once as default. E.G., "Cancel"
     * @param {LocalizedString} report-user-dialog-thank-you-text report user dialog thank you text. Set up once as default. E.G., "Thank you. Your report has been submitted to Customer Support."
     * @param {LocalizedString} report-user-dialog-confidential-text report user dialog confidential text. Set up once as default. E.G., "All Reports are confidential"
     *
     * @description
     *
     * Profile Header
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-header nucleusid="123456789"
     *            onlinestr="ONLINE"
     *            awaystr="AWAY"
     *            offlinestr="OFFLINE"
     *            invisiblestr="INVISIBLE"
     *            playingstr="PLAYING:"
     *            broadcastingstr="BROADCASTING:"
     *            editoneadotcomstr="Edit on EA.com"
     *            sendfriendrequeststr="Send Friend Request"
     *            friendrequestsentstr="Friend Request Sent"
     *            acceptfriendrequeststr="Accept Friend Request"
     *            ignorefriendrequeststr="Ignore Friend Request"
     *            unfriendstr="Unfriend"
     *            blockandunfriendstr="Block & Unfriend"
     *            blockuserstr="Block User"
     *            unblockuserstr="Unblock User"
     *            reportuserstr="Report User"
     *            unfriendusertitlestr="Unfriend User"
     *            unfrienduserdescriptionstr="Are you sure you want to unfriend %username%?"
     *            unfrienduseracceptstr="Unfriend"
     *            unfrienduserrejectstr="Cancel"
     *               blocktitle="Block User"
     *               blockdescription="Are you sure?"
     *               blockdescriptiontwostr="Once you block them, %username% won't be able to chat with you or view your full profile."
     *               blockdescriptionthreestr="If you change your mind later, you can always remove %username% from your block list by selecting EA Account and Billing from the Origin menu."
     *               blockaccept="Block"
     *               blockreject="Cancel"
     *              unfriendandblockdescription="Are you sure you want to unfriend and block %username%?"
     *              unfriendandblockdescriptiontwostr="Once you unfriend and block them, %username% won't be able to chat with you or view your full profile."
     *              unfriendandblockdescriptionthreestr="If you change your mind later, you can always remove %username% from your block list by selecting EA Account and Billing from the Origin menu."
     *              unfriendandblocktitle="Unfriend And Block"
     *              unfriendandblockaccept="Yes"
     *              unfriendandblockreject="No"
     *               errorfriendnotaddedtitle="Text"
     *               errorfriendnotaddedaccept="Text"
     *               errorfriendnotaddeddescription="Text"
     *               errorfriendnotaddedcancel="Text"
     *               cancelandblockuser= "Cancel Friend Request and Block"
     *               ignoreandblockuser= "Ignore Friend Request and Block"
     *                 cancelandblocktitle="Text"
     *                 cancelandblockdescription ="Text"
     *                 cancelandblockdescriptiontwostr ="Once you do this, %username% won't be able to chat with you or view your full profile."
     *                 cancelandblockaccept="Text"
     *                 cancelandblockdecline="Text"
     *                 ignoreandblocktitle="Text"
     *                 ignoreandblockdescription="Text"
     *                 ignoreandblockdescriptiontwostr="Once you do this, %username% won't be able to chat with you or view your full profile."
     *                 changeminddescription="Text"
     *                 ignoreandblockaccept="Text"
     *                 ignoreandblockdecline="Text"
     *            reportusertitlestr="Report User"
     *            reportusersubmitstr="Submit"
     *            reportusercancelstr="Cancel"
     *            reportuserclosestr="Close"
     *            reportuserthankyoustr="Thank you. Your report has been submitted to Customer Support."
     *            reportuserconfidentialstr="All Reports are confidential."
     *         ></origin-profile-header>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfileHeaderCtrl', OriginProfileHeaderCtrl)
        .directive('originProfileHeader', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfileHeaderCtrl',
                scope: {
                    nucleusId: '@nucleusid',
                    onlineStr: '@onlinestr',
                    awayStr: '@awaystr',
                    offlineStr: '@offlinestr',
                    invisibleStr: '@invisiblestr',
                    playingStr: '@playingstr',
                    broadcastingStr: '@broadcastingstr',
                    editoneadotcomStr: '@editoneadotcomstr',
                    sendfriendrequestStr: '@sendfriendrequeststr',
                    friendrequestsentStr: '@friendrequestsentstr',
                    acceptfriendrequestStr: '@acceptfriendrequeststr',
                    ignorefriendrequestStr: '@ignorefriendrequeststr',
                    unfriendStr: '@unfriendstr',
                    blockandunfriendStr: '@blockandunfriendstr',
                    blockuserStr: '@blockuserstr',
                    unblockuserStr: '@unblockuserstr',
                    reportuserStr: '@reportuserstr',
                    unfriendusertitleStr: '@unfriendusertitlestr',
                    unfrienduserdescriptionStr: '@unfrienduserdescriptionstr',
                    unfrienduseracceptStr: '@unfrienduseracceptstr',
                    unfrienduserrejectStr: '@unfrienduserrejectstr',
                    blocktitle: '@',
                    blockdescription: '@blockdescriptionstr',
                    blockdescriptiontwoStr: '@blockdescriptiontwostr',
                    blockdescriptionthreeStr: '@blockdescriptionthreestr',
                    unfriendandblocktitle: '@',
                    unfriendandblockdescription: '@',
                    unfriendandblockdescriptiontwoStr: '@unfriendandblockdescriptiontwostr',
                    unfriendandblockdescriptionthreeStr: '@unfriendandblockdescriptionthreestr',
                    unfriendandblockaccept: '@',
                    unfriendandblockreject: '@',
                    errorfriendnotaddeddescription: '@',
                    errorfriendnotaddedaccept: '@',
                    errorfriendnotaddedtitle: '@',
                    errorfriendnotaddedcancel: '@',
                    cancelandblockuser: '@',
                    ignoreandblockuser: '@',
                    cancelandblocktitle: '@',
                    cancelandblockdescription: '@',
                    cancelandblockdescriptiontwoStr: '@cancelandblockdescriptiontwostr',
                    changeminddescription: '@',
                    cancelandblockaccept: '@',
                    cancelandblockdecline: '@',
                    ignoreandblocktitle: '@',
                    ignoreandblockdescription: '@',
                    ignoreandblockdescriptiontwoStr: '@ignoreandblockdescriptiontwostr',
                    ignoreandblockaccept: '@',
                    ignoreandblockdecline: '@',
                    reportUserDialogTitleText: '@',
                    reportUserDialogSubmitText: '@',
                    reportUserDialogCancelText: '@',
                    reportUserDialogCloseText: '@',
                    reportUserDialogThankYouText: '@',
                    reportUserDialogConfidentialText: '@'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/views/header.html'),
                link: function(scope, element, attrs, ctrl) {
                    attrs = attrs;

                    scope.$element = $(element);

                    function onPrimaryButtonClick() {
                        var which = $(this).attr('data-button');
                        if (which === 'editselfprofile') {
                            // Edit self profile
                            ctrl.editSelfProfile();
                        } else if (which === 'sendfriendrequest') {
                            // Send friend request
                            ctrl.sendFriendRequest();
                        } else if (which === 'acceptfriendrequest') {
                            // Accept friend request
                            ctrl.acceptFriendRequest();
                        }
                    }

                    function onOptionsButtonClick() {
                        ctrl.toggleOptionsContextmenu();
                    }

                    function onDocumentClick(e) {
                        var $target = $(e.target);
                        if (scope.optionsContextmenuVisible && !$target.hasClass('origin-profile-header-controls-options-button')) {
                            ctrl.closeOptionsContextmenu(true);
                        }
                    }
                    // Listen for clicks on the primary header button
                    $(element).on('click', '.origin-profile-header-controls-primary-button', onPrimaryButtonClick);


                    // Listen for clicks on the menu button
                    $(element).on('click', '.origin-profile-header-controls-options-button', onOptionsButtonClick);

                    // Cancel options menu by clicking elsewhere
                    $(document).on('click', onDocumentClick);

                    function onDestroy() {
                        $(element).off('click', '.origin-profile-header-controls-primary-button', onPrimaryButtonClick);
                        $(element).off('click', '.origin-profile-header-controls-options-button', onOptionsButtonClick);
                        $(document).off('click', onDocumentClick);

                    }
                    scope.$on('$destroy', onDestroy);

                }
            };

        });
}());
