/**
 * Created by tirhodes on 2015-02-23.
 * @file pendingitem.js
 */
(function () {

    'use strict';
    var panelHeight = 0;

    /**
    * The controller
    */
    function OriginSocialHubRosterPendingitemCtrl($scope, $window, $timeout, KeyEventsHelper, RosterDataFactory, UserDataFactory, UtilFactory, DialogFactory, AppCommFactory, AuthFactory, NavigationFactory, EventsHelperFactory, SocialHubFactory, ComponentsLogFactory, ObserverFactory) {

        var CONTEXT_NAMESPACE = 'origin-social-hub-roster-pendingitem';

        $scope.acceptLoc = UtilFactory.getLocalizedStr($scope.acceptStr, CONTEXT_NAMESPACE, 'acceptstr');
        $scope.declineLoc = UtilFactory.getLocalizedStr($scope.declineStr, CONTEXT_NAMESPACE, 'declinestr');
        $scope.cancelLoc = UtilFactory.getLocalizedStr($scope.cancelStr, CONTEXT_NAMESPACE, 'cancelstr');
        $scope.isContextmenuVisible = false;
        $scope.contextmenuCallbacks = {};

        $scope.cancelandblocktitleLoc = UtilFactory.getLocalizedStr($scope.cancelandblocktitle, CONTEXT_NAMESPACE, 'cancelandblocktitle');
        $scope.cancelandblockdescriptionLoc = UtilFactory.getLocalizedStr($scope.cancelandblockdescription, CONTEXT_NAMESPACE, 'cancelandblockdescription');
        $scope.cancelandblockdescription2Loc = UtilFactory.getLocalizedStr($scope.cancelandblockdescription2, CONTEXT_NAMESPACE, 'cancelandblockdescription2');
        $scope.changeminddescriptionLoc = UtilFactory.getLocalizedStr($scope.changeminddescription, CONTEXT_NAMESPACE, 'changeminddescription');
        $scope.cancelandblockacceptLoc = UtilFactory.getLocalizedStr($scope.cancelandblockaccept, CONTEXT_NAMESPACE, 'cancelandblockaccept');
        $scope.cancelandblockdeclineLoc = UtilFactory.getLocalizedStr($scope.cancelandblockdecline, CONTEXT_NAMESPACE, 'cancelandblockdecline');
        $scope.ignoreandblocktitleLoc = UtilFactory.getLocalizedStr($scope.ignoreandblocktitle, CONTEXT_NAMESPACE, 'ignoreandblocktitle');
        $scope.ignoreandblockdescriptionLoc = UtilFactory.getLocalizedStr($scope.ignoreandblockdescription, CONTEXT_NAMESPACE, 'ignoreandblockdescription');
        $scope.ignoreandblockdescription2Loc = UtilFactory.getLocalizedStr($scope.ignoreandblockdescription2, CONTEXT_NAMESPACE, 'ignoreandblockdescription2');
        $scope.ignoreandblockacceptLoc = UtilFactory.getLocalizedStr($scope.ignoreandblockaccept, CONTEXT_NAMESPACE, 'ignoreandblockaccept');
        $scope.ignoreandblockdeclineLoc = UtilFactory.getLocalizedStr($scope.ignoreandblockdecline, CONTEXT_NAMESPACE, 'ignoreandblockdecline');

        $scope.errorfriendnotaddedtitleLoc = UtilFactory.getLocalizedStr($scope.errorfriendnotaddedtitle, CONTEXT_NAMESPACE, 'errorfriendnotaddedtitle');
        $scope.errorfriendnotaddeddescriptionLoc = UtilFactory.getLocalizedStr($scope.errorfriendnotaddeddescription, CONTEXT_NAMESPACE, 'errorfriendnotaddeddescription');
        $scope.errorfriendnotaddedacceptLoc = UtilFactory.getLocalizedStr($scope.errorfriendnotaddedaccept, CONTEXT_NAMESPACE, 'errorfriendnotaddedaccept');

        $scope.acceptfriendrequest = UtilFactory.getLocalizedStr($scope.acceptfriendrequest, CONTEXT_NAMESPACE, 'acceptfriendrequest');
        $scope.ignorefriendrequest = UtilFactory.getLocalizedStr($scope.ignorefriendrequest, CONTEXT_NAMESPACE, 'ignorefriendrequest');
        $scope.cancelfriendrequest = UtilFactory.getLocalizedStr($scope.cancelfriendrequest, CONTEXT_NAMESPACE, 'cancelfriendrequest');
        $scope.viewprofile = UtilFactory.getLocalizedStr($scope.viewprofile, CONTEXT_NAMESPACE, 'viewprofile');
        $scope.cancelfriendrequestandblock = UtilFactory.getLocalizedStr($scope.cancelfriendrequestandblock, CONTEXT_NAMESPACE, 'cancelfriendrequestandblock');
        $scope.ignorefriendrequestandblock = UtilFactory.getLocalizedStr($scope.ignorefriendrequestandblock, CONTEXT_NAMESPACE, 'ignorefriendrequestandblock');

        var $element, $wrapper, handles = [];
        var self = this;

        this.setElements = function(element, wrapper) {
            $element = element;
            $wrapper = wrapper;
        };

        // If we get a blocked event here we need to remove the pending invite
        // and tell the RosterDataFactory about this change
        function onUserSubsctiptionChanged(newVal) {
            if (newVal.relationship === 'BLOCKED') {
                var result = {'accepted' : true};
                if (newVal.requestSent) {
                    self.onCancelRequestClick(result);
                    RosterDataFactory.deleteFromSectionRoster('OUTGOING', $scope.friend.nucleusId);
                } else {
                    self.onDeclineButtonClick(result);
                    RosterDataFactory.deleteFromSectionRoster('INCOMING', $scope.friend.nucleusId);
                }
            }
        }

        // Connect our observer to the proper callback
        function setUpObservers(user) {
            ObserverFactory.create(user.subscription)
                .defaultTo({state: 'NONE', requestSent: false, relationship: 'NON_FRIEND'})
                .attachToScope($scope, onUserSubsctiptionChanged);
        }

        // Double check this user's blocked status and set up an observer
        // This is needed because we can not garentee to get roster updates for blocked users
        RosterDataFactory.isBlocked($scope.friend.nucleusId).then(function(value) {
            if (value) {
                $scope.friend.subscription.data.relationship = 'BLOCKED';
            }
            setUpObservers($scope.friend);
        });

        function requestAvatar(nucleusId) {
            UserDataFactory.getAvatar(nucleusId, Origin.defines.avatarSizes.SMALL)
                .then(function(response) {
                    $scope.friend.avatarImgSrc = response;
                    $scope.$digest();
                }, function() {

                }).catch(function(error) {
                    ComponentsLogFactory.error ('OriginSocialHubRosterFriendCtrl: UserDataFactory.getAvatar failed', error);
                });
        }
        this.onCancelRequestClick = function(element) {
            $timeout(function () {
                $wrapper.slideUp(300, function () {
                    RosterDataFactory.cancelPendingFriendRequest($scope.friend.nucleusId);
                    angular.element(element).remove();
                });
            }, 0);
            RosterDataFactory.updateRosterViewport();
        };

        this.onDeclineButtonClick = function(element) {
            $wrapper.slideUp(300, function() {
                RosterDataFactory.friendRequestReject($scope.friend.nucleusId);
                angular.element(element).remove();
            });
            RosterDataFactory.updateRosterViewport();
        };

        function handleNameUpdate(friend) {
            $scope.friend.originId = friend.EAID;
            $scope.friend.firstName = friend.firstName;
            $scope.friend.lastName = friend.lastName;
            $scope.$digest();
        }

        this.setPanelHeight = function() {
            panelHeight = $element.outerHeight();
            if (panelHeight > 0) {
                RosterDataFactory.setFriendPanelHeight(panelHeight);
            }
        };

        /* Context menu callbacks */
        $scope.contextmenuCallbacks.acceptRequest = function($event) {
            $scope.isContextmenuVisible = false;
            $element.find('.origin-social-hub-roster-pendingitem-accept-icon').trigger('click');
            KeyEventsHelper.cancelEvent($event);
        };

        $scope.contextmenuCallbacks.ignoreRequest = function($event) {
            $scope.isContextmenuVisible = false;
            $element.find('.origin-social-hub-roster-pendingitem-decline-icon').trigger('click');
            KeyEventsHelper.cancelEvent($event);

        };

        $scope.contextmenuCallbacks.cancelRequest = function($event) {
            $scope.isContextmenuVisible = false;
            $element.find('.origin-social-hub-roster-pendingitem-cancel-icon').trigger('click');
            KeyEventsHelper.cancelEvent($event);

        };

        $scope.contextmenuCallbacks.viewProfile = function($event) {
            if (Origin.client.oig.IGOIsActive()) {
                // Open OIG profile window
                NavigationFactory.openIGOSPA('PROFILE', $scope.friend.nucleusId);
            }
            else {
                NavigationFactory.goUserProfile($scope.friend.nucleusId);
            }
            $scope.isContextmenuVisible = false;
            KeyEventsHelper.cancelEvent($event);
        };

        this.handleErrorDialog = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleErrorDialog);

            if (result.accepted) {
                SocialHubFactory.unminimizeWindow();
                SocialHubFactory.focusWindow();
            }
        };

        this.handleIgnoreAndBlock = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleIgnoreAndBlock);

            if (result.accepted) {
                RosterDataFactory.ignoreAndBlockUser($scope.friend.nucleusId);
            }
        };

        this.handleCancelAndBlock = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleCancelAndBlock);

            if (result.accepted) {
                RosterDataFactory.cancelAndBlockUser($scope.friend.nucleusId);
            }
        };

        $scope.contextmenuCallbacks.cancelAndBlockRequest = function($event) {
            $scope.isContextmenuVisible = false;

            var description = '<b>' + $scope.cancelandblockdescriptionLoc + '</b>' + '<br><br>' +
                $scope.cancelandblockdescription2Loc + '<br><br>' +
                $scope.changeminddescriptionLoc;
            description = description.replace(/%username%/g, $scope.friend.originId);

            KeyEventsHelper.cancelEvent($event);

            DialogFactory.openPrompt({
                id: 'social-hub-roster-cancelRequestAndBlock-id',
                name: 'social-hub-roster-cancelRequestAndBlock',
                title: $scope.cancelandblocktitleLoc,
                description: description,
                acceptText: $scope.cancelandblockacceptLoc,
                acceptEnabled: true,
                rejectText: $scope.cancelandblockdeclineLoc,
                defaultBtn: 'yes',
                callback: self.handleCancelAndBlock
            });
        };

        $scope.contextmenuCallbacks.blockPendingRequestor = function($event) {
            $scope.isContextmenuVisible = false;

            var description = '<b>' + $scope.ignoreandblockdescriptionLoc + '</b>' + '<br><br>' +
                $scope.ignoreandblockdescription2Loc + '<br><br>' +
                $scope.changeminddescriptionLoc;
            description = description.replace(/%username%/g, $scope.friend.originId);

            KeyEventsHelper.cancelEvent($event);

            DialogFactory.openPrompt({
                id: 'social-hub-roster-ignoreRequestAndBlock-id',
                name: 'social-hub-roster-ignoreRequestAndBlock',
                title: $scope.ignoreandblocktitleLoc,
                description: description,
                acceptText: $scope.ignoreandblockacceptLoc,
                acceptEnabled: true,
                rejectText: $scope.ignoreandblockdeclineLoc,
                defaultBtn: 'yes',
                callback: self.handleIgnoreAndBlock
            });
        };

        function onClientOnlineStateChanged(onlineObj) {
            if (!!onlineObj && !onlineObj.isOnline) {
                DialogFactory.close('social-hub-roster-cancelRequestAndBlock-id');
                DialogFactory.close('social-hub-roster-ignoreRequestAndBlock-id');
            }
        }

        $scope.openContextmenu = function() {
            $scope.isContextmenuVisible = true;
            $scope.$digest();
        };

        $scope.closeContextmenu = function() {
            $scope.isContextmenuVisible = false;
            $scope.$digest();
        };

        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);

        handles[handles.length] = UserDataFactory.events.on('socialFriendsFullNameUpdated:' + $scope.friend.nucleusId, handleNameUpdate);
        UserDataFactory.getUserRealName($scope.friend.nucleusId); // request the originId

        requestAvatar($scope.friend.nucleusId);


        function onDestroy() {
            EventsHelperFactory.detachAll(handles);
        }

        $scope.$on('$destroy', onDestroy);

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubRosterPendingitem
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} friend Javascript object representing the friend for this pending item
     * @param {LocalizedString} acceptstr "Accept" tooltip
     * @param {LocalizedString} declinestr "Decline" tooltip
     * @param {LocalizedString} cancelstr "Cancel" tooltip
     * @param {LocalizedString} cancelandblocktitle desc
     * @param {LocalizedString} cancelandblockdescription desc
     * @param {LocalizedString} cancelandblockdescription2 desc
     * @param {LocalizedString} changeminddescription desc
     * @param {LocalizedString} cancelandblockaccept desc
     * @param {LocalizedString} cancelandblockdecline desc
     * @param {LocalizedString} ignoreandblocktitle desc
     * @param {LocalizedString} ignoreandblockdescription desc
     * @param {LocalizedString} ignoreandblockdescription2 desc
     * @param {LocalizedString} ignoreandblockaccept desc
     * @param {LocalizedString} ignoreandblockdecline desc
     * @param {LocalizedString} errorfriendnotaddedtitle desc
     * @param {LocalizedString} errorfriendnotaddedaccept desc
     * @param {LocalizedString} errorfriendnotaddeddescription desc
     * @param {LocalizedString} acceptfriendrequest Accept Friend Request
     * @param {LocalizedString} ignorefriendrequest Ignore Friend Request
     * @param {LocalizedString} cancelfriendrequest Cancel Friend Request
     * @param {LocalizedString} viewprofile View Profile
     * @param {LocalizedString} cancelfriendrequestandblock Cancel Friend Request and Block
     * @param {LocalizedString} ignorefriendrequestandblock Ignore Friend Request and Block
     * @description
     *
     * origin social hub -> roster area -> pending item
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-hub-roster-pendingitem
     *            friend="friend"
     *            acceptstr="Accept"
     *            declinestr="Decline"
     *            cancelstr="Cancel"
     *                 cancelandblocktitle="Text"
     *                 cancelandblockdescription ="Text"
     *                 cancelandblockdescription2 ="Text"
     *                 cancelandblockaccept="Text"
     *                 cancelandblockdecline="Text"
     *                 ignoreandblocktitle="Text"
     *                 ignoreandblockdescription="Text"
     *                 ignoreandblockdescription2="Text"
     *                 changeminddescription="Text"
     *                 ignoreandblockaccept="Text"
     *                 ignoreandblockdecline="Text"
     *                 errorfriendnotaddedtitle="Text"
     *                 errorfriendnotaddedaccept="Text"
     *                 errorfriendnotaddeddescription="Text"
     *                 acceptfriendrequest="Text"
     *                 ignorefriendrequest="Text"
     *                 cancelfriendrequest="Text"
     *                 viewprofile="Text"
     *                 cancelfriendrequestandblock="Text"
     *                 ignorefriendrequestandblock="Text"
     *         ></origin-social-hub-roster-pendingitem>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubRosterPendingitemCtrl', OriginSocialHubRosterPendingitemCtrl)
        .directive('originSocialHubRosterPendingitem', function(ComponentsConfigFactory, RosterDataFactory, DialogFactory, $timeout, KeyEventsHelper, ActiveElementHelper) {

            return {
                restrict: 'E',
                controller: 'OriginSocialHubRosterPendingitemCtrl',
                require: ['originSocialHubRosterPendingitem', '^originSocialHubRosterSection'],
                scope: {
                    'friend': '=',
                    'acceptStr': '@acceptstr',
                    'declineStr': '@declinestr',
                    'cancelStr': '@cancelstr',
                    'cancelandblocktitle': '@',
                    'cancelandblockdescription': '@',
                    'cancelandblockdescription2': '@',
                    'changeminddescription': '@',
                    'cancelandblockaccept': '@',
                    'cancelandblockdecline': '@',
                    'ignoreandblocktitle': '@',
                    'ignoreandblockdescription': '@',
                    'ignoreandblockdescription2': '@',
                    'ignoreandblockaccept': '@',
                    'ignoreandblockdecline': '@',
                    'errorfriendnotaddeddescription': '@',
                    'errorfriendnotaddedaccept': '@',
                    'errorfriendnotaddedtitle': '@',
                    'acceptfriendrequest': '@',
                    'ignorefriendrequest': '@',
                    'cancelfriendrequest': '@',
                    'viewprofile': '@',
                    'cancelfriendrequestandblock': '@',
                    'ignorefriendrequestandblock': '@'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/roster/views/pendingitem.html'),
                link: function (scope, element, attrs, ctrl) {
                    attrs = attrs;

                    var thisController   = ctrl[0],
                        sectionController = ctrl[1];

                    var $element = angular.element(element).find('.origin-social-hub-roster-pendingitem'),
                        $wrapper = $element.closest('.origin-social-hub-roster-pendingitem-wrap');

                    thisController.setElements($element, $wrapper);

                    if (panelHeight === 0) {
                        thisController.setPanelHeight();
                    }

                    // Set up placeholder avatar
                    if (typeof scope.friend.avatarImgSrc === 'undefined') {
                        scope.friend.avatarImgSrc = ComponentsConfigFactory.getImagePath('social//avatar_placeholder.png');
                    }
                    function onAcceptButton() {
                        var $thisElement = angular.element(this);
                        RosterDataFactory.friendRequestAccept(scope.friend.nucleusId).then(function() {
                            var $icon = $thisElement.closest('.origin-social-hub-roster-pendingitem-icon');
                            var $pendingItem = $thisElement.closest('.origin-social-hub-roster-pendingitem');
                            var $pendingItemWrapper = $thisElement.closest('.origin-social-hub-roster-pendingitem-wrap');

                            // Change background
                            $pendingItem.addClass('origin-social-hub-roster-pendingitem-accepted');

                            // Icon
                            $icon.addClass('origin-social-hub-roster-pendingitem-accept-icon-accepted');

                            // Remove other items
                            $pendingItem.children().not('.origin-social-hub-roster-pendingitem-accept-icon-wrap').fadeOut();

                            $timeout(function () {
                                $pendingItemWrapper.slideUp(300, function () {
                                    sectionController.deleteFromRoster(scope.friend.nucleusId);
                                });
                            }, 500);
                            var next = ActiveElementHelper.getNext($element);
                            angular.element(next).focus();
                        }, function() {
                            // Accept friend request failed. Display error dialog
                            DialogFactory.openPrompt({
                                id: 'social-hub-roster-errorFriendNotAdded-id',
                                name: 'social-hub-roster-errorFriendNotAdded',
                                title: scope.errorfriendnotaddedtitleLoc,
                                description: scope.errorfriendnotaddeddescriptionLoc,
                                acceptText: scope.errorfriendnotaddedacceptLoc,
                                acceptEnabled: true,
                                rejectText: scope.cancelLoc,
                                defaultBtn: 'yes',
                                callback: thisController.handleErrorDialog
                            });
                        });
                    }
                    function onDeclineButton() {
                        var next = ActiveElementHelper.getNext($element);
                        thisController.onDeclineButtonClick(element);
                        angular.element(next).focus();
                    }
                    function onCancelButton() {
                        var next = ActiveElementHelper.getNext($element);
                        thisController.onCancelRequestClick(element);
                        angular.element(next).focus();
                    }
                    function handleRightKeyup(e) {
                        e.stopPropagation();
                        scope.openContextmenu();
                    }
                    function handleLeftKeyup(e) {
                        e.stopPropagation();
                        scope.closeContextmenu();
                        $element.focus();
                    }
                    function handleEnterKeyup(e) {
                        e.stopPropagation();
                        onAcceptButton();
                    }
                    function handleEscapeKeyup(e) {
                        if(scope.friend.subReqSent) {
                            onCancelButton();
                        } else {
                            onDeclineButton();
                        }
                        e.stopPropagation();
                    }
                    function handleRightClick(e) {
                        e.preventDefault();
                        scope.openContextmenu();
                        $element.focus();
                    }
                    function handleFocusOut() {
                        //run this on the next tick to give the active element a chance to set
                        setTimeout(function() {
                            if (!$element.has(document.activeElement).length) {
                                scope.closeContextmenu();
                            }

                        }, 0);

                    }
                    function handleKeyup(event) {
                       return KeyEventsHelper.mapEvents(event, {
                           'right': handleRightKeyup,
                           'left': handleLeftKeyup,
                           'enter': handleEnterKeyup,
                           'escape': handleEscapeKeyup
                       });
                    }
                    function handleLeftClick(e) {
                        // There's an icon to icon-sider
                        if(angular.element(e.target).hasClass('origin-social-hub-roster-contextmenu-otkicon-downarrow') && !scope.isContextmenuVisible) {
                            scope.openContextmenu();
                        } else {
                            scope.closeContextmenu();
                            $element.focus();
                        }
                    }

                    $element.on('click', handleLeftClick);
                    $element.on('focusout', handleFocusOut);
                    $element.on('keyup', handleKeyup);
                    $element.bind('contextmenu', handleRightClick);
                    $element.on('click', '.origin-social-hub-roster-pendingitem-accept-icon', onAcceptButton);
                    $element.on('click', '.origin-social-hub-roster-pendingitem-decline-icon', onDeclineButton);
                    $element.on('click', '.origin-social-hub-roster-pendingitem-cancel-icon', onCancelButton);

                    function onDestroy() {
                        $element.off('click', handleLeftClick);
                        $element.off('focusout', handleFocusOut);
                        $element.unbind('contextmenu', handleRightClick);
                        $element.off('keyup', handleKeyup);
                        $element.off('click', '.origin-social-hub-roster-pendingitem-accept-icon', onAcceptButton);
                        $element.off('click', '.origin-social-hub-roster-pendingitem-decline-icon', onDeclineButton);
                        $element.off('click', '.origin-social-hub-roster-pendingitem-cancel-icon', onCancelButton);
                    }
                    scope.$on('$destroy', onDestroy);
                    window.addEventListener('unload', onDestroy);
                }

            };

        });
}());
