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
    function OriginSocialHubRosterPendingitemCtrl($scope, $window, $timeout, RosterDataFactory, UserDataFactory, UtilFactory, DialogFactory, AppCommFactory,
                                                  AuthFactory, NavigationFactory, EventsHelperFactory, SocialHubFactory, ComponentsLogFactory) {

        var CONTEXT_NAMESPACE = 'origin-social-hub-roster-pendingitem';

        $scope.acceptLoc = UtilFactory.getLocalizedStr($scope.acceptStr, CONTEXT_NAMESPACE, 'acceptstr');
        $scope.declineLoc = UtilFactory.getLocalizedStr($scope.declineStr, CONTEXT_NAMESPACE, 'declinestr');
        $scope.cancelLoc = UtilFactory.getLocalizedStr($scope.cancelStr, CONTEXT_NAMESPACE, 'cancelstr');
        $scope.isContextmenuVisible = false;
        $scope.contextmenuCallbacks = {};

        $scope.cancelandblocktitleLoc = UtilFactory.getLocalizedStr($scope.cancelandblocktitle, CONTEXT_NAMESPACE, 'cancelandblocktitle');
        $scope.cancelandblockdescriptionLoc = UtilFactory.getLocalizedStr($scope.cancelandblockdescription, CONTEXT_NAMESPACE, 'cancelandblockdescription');
        $scope.cancelandblockdescription2Loc = UtilFactory.getLocalizedStr($scope.cancelandblockdescription2, CONTEXT_NAMESPACE, 'cancelandblockdescription_2');
        $scope.changeminddescriptionLoc = UtilFactory.getLocalizedStr($scope.changeminddescription, CONTEXT_NAMESPACE, 'changeminddescription');
        $scope.cancelandblockacceptLoc = UtilFactory.getLocalizedStr($scope.cancelandblockaccept, CONTEXT_NAMESPACE, 'cancelandblockaccept');
        $scope.cancelandblockdeclineLoc = UtilFactory.getLocalizedStr($scope.cancelandblockdecline, CONTEXT_NAMESPACE, 'cancelandblockdecline');
        $scope.ignoreandblocktitleLoc = UtilFactory.getLocalizedStr($scope.ignoreandblocktitle, CONTEXT_NAMESPACE, 'ignoreandblocktitle');
        $scope.ignoreandblockdescriptionLoc = UtilFactory.getLocalizedStr($scope.ignoreandblockdescription, CONTEXT_NAMESPACE, 'ignoreandblockdescription');
        $scope.ignoreandblockdescription2Loc = UtilFactory.getLocalizedStr($scope.ignoreandblockdescription2, CONTEXT_NAMESPACE, 'ignoreandblockdescription_2');
        $scope.changeminddescriptionLoc = UtilFactory.getLocalizedStr($scope.changeminddescription, CONTEXT_NAMESPACE, 'changeminddescription');
        $scope.ignoreandblockacceptLoc = UtilFactory.getLocalizedStr($scope.ignoreandblockaccept, CONTEXT_NAMESPACE, 'ignoreandblockaccept');
        $scope.ignoreandblockdeclineLoc = UtilFactory.getLocalizedStr($scope.ignoreandblockdecline, CONTEXT_NAMESPACE, 'ignoreandblockdecline');

        $scope.errorfriendnotaddedtitleLoc = UtilFactory.getLocalizedStr($scope.errorfriendnotaddedtitle, CONTEXT_NAMESPACE, 'errorfriendnotaddedtitle');
        $scope.errorfriendnotaddeddescriptionLoc = UtilFactory.getLocalizedStr($scope.errorfriendnotaddeddescription, CONTEXT_NAMESPACE, 'errorfriendnotaddeddescription');
        $scope.errorfriendnotaddedacceptLoc = UtilFactory.getLocalizedStr($scope.errorfriendnotaddedaccept, CONTEXT_NAMESPACE, 'errorfriendnotaddedaccept');

        var $element, $wrapper, handles = [];
        var self = this;

        this.setElements = function(element, wrapper) {
            $element = element;
            $wrapper = wrapper;
        };

        function requestAvatar(nucleusId) {
            UserDataFactory.getAvatar(nucleusId, 'AVATAR_SZ_SMALL')
                .then(function(response) {
                    $scope.friend.avatarImgSrc = response;
                    $scope.$digest();
                }, function() {

                }).catch(function(error) {
                    ComponentsLogFactory.error ('OriginSocialHubRosterFriendCtrl: UserDataFactory.getAvatar failed', error);
                });
        }
        this.onCancelRequestClick = function (element) {
            $timeout(function () {
                $wrapper.slideUp(300, function () {
                    RosterDataFactory.cancelPendingFriendRequest($scope.friend.nucleusId);
                    $(element).remove();
                });
            }, 0);
        };

        this.onDeclineButtonClick = function (element) {
            $wrapper.slideUp(300, function () {
                RosterDataFactory.friendRequestReject($scope.friend.nucleusId);
                $(element).remove();
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
        $scope.contextmenuCallbacks.acceptRequest = function () {
            $scope.isContextmenuVisible = false;
            $element.find('.origin-social-hub-roster-pendingitem-accept-icon').trigger('click');
        };

        $scope.contextmenuCallbacks.ignoreRequest = function () {
            $scope.isContextmenuVisible = false;
            $element.find('.origin-social-hub-roster-pendingitem-decline-icon').trigger('click');
        };

        $scope.contextmenuCallbacks.cancelRequest = function () {
            $scope.isContextmenuVisible = false;
            $element.find('.origin-social-hub-roster-pendingitem-cancel-icon').trigger('click');
        };

        $scope.contextmenuCallbacks.viewProfile = function () {
            NavigationFactory.goUserProfile($scope.friend.nucleusId);
            $scope.isContextmenuVisible = false;
        };

        this.handleErrorDialog = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleErrorDialog);

            if (result.accepted) {
                SocialHubFactory.unminimizeWindow();
                SocialHubFactory.focusWindow();
            }
        };

        this.handleBlock = function(result) {
            AppCommFactory.events.off('dialog:hide', self.handleBlock);

            if (result.accepted) {
                RosterDataFactory.blockUser($scope.friend.nucleusId);
            }
        };

        $scope.contextmenuCallbacks.cancelAndBlockRequest = function () {
            $scope.isContextmenuVisible = false;

            var description = '<b>' + $scope.cancelandblockdescriptionLoc + '</b>' + '<br><br>' +
                $scope.cancelandblockdescription2Loc + '<br><br>' +
                $scope.changeminddescriptionLoc;
            description = description.replace(/\'/g, "&#39;").replace(/%username%/g, $scope.friend.originId);

            DialogFactory.openPrompt({
                id: 'social-hub-roster-cancelRequestAndBlock-id',
                name: 'social-hub-roster-cancelRequestAndBlock',
                title: $scope.cancelandblocktitleLoc,
                description: description,
                acceptText: $scope.cancelandblockacceptLoc,
                acceptEnabled: true,
                rejectText: $scope.cancelandblockdeclineLoc,
                defaultBtn: 'yes'
            });

            AppCommFactory.events.on('dialog:hide', self.handleBlock);
        };

        $scope.contextmenuCallbacks.blockPendingRequestor = function () {
            $scope.isContextmenuVisible = false;

            var description = '<b>' + $scope.ignoreandblockdescriptionLoc + '</b>' + '<br><br>' +
                $scope.ignoreandblockdescription2Loc + '<br><br>' +
                $scope.changeminddescriptionLoc;
            description = description.replace(/\'/g, "&#39;").replace(/%username%/g, $scope.friend.originId);

            DialogFactory.openPrompt({
                id: 'social-hub-roster-ignoreRequestAndBlock-id',
                name: 'social-hub-roster-ignoreRequestAndBlock',
                title: $scope.ignoreandblocktitleLoc,
                description: description,
                acceptText: $scope.ignoreandblockacceptLoc,
                acceptEnabled: true,
                rejectText: $scope.ignoreandblockdeclineLoc,
                defaultBtn: 'yes'
            });

            AppCommFactory.events.on('dialog:hide', self.handleBlock);

        };

        function onClientOnlineStateChanged(onlineObj) {
            if (!!onlineObj && !onlineObj.isOnline) {
                DialogFactory.close('social-hub-roster-cancelRequestAndBlock-id');
                DialogFactory.close('social-hub-roster-ignoreRequestAndBlock-id');
            }
        }

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
     * @param {LocalizedString} cancelandblocktitle
     * @param {LocalizedString} cancelandblockdescription
     * @param {LocalizedString} cancelandblockdescription2
     * @param {LocalizedString} changeminddescription
     * @param {LocalizedString} cancelandblockaccept
     * @param {LocalizedString} cancelandblockdecline
     * @param {LocalizedString} ignoreandblocktitle
     * @param {LocalizedString} ignoreandblockdescription
     * @param {LocalizedString} ignoreandblockdescription2
     * @param {LocalizedString} ignoreandblockaccept
     * @param {LocalizedString} ignoreandblockdecline
     * @param {LocalizedString} errorfriendnotaddedtitle
     * @param {LocalizedString} errorfriendnotaddedaccept
     * @param {LocalizedString} errorfriendnotaddeddescription
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
     *         ></origin-social-hub-roster-pendingitem>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubRosterPendingitemCtrl', OriginSocialHubRosterPendingitemCtrl)
        .directive('originSocialHubRosterPendingitem', function(ComponentsConfigFactory, AppCommFactory, RosterDataFactory, DialogFactory, $timeout) {

            return {
                restrict: 'E',
                controller: 'OriginSocialHubRosterPendingitemCtrl',
                require: ['originSocialHubRosterPendingitem', '^originSocialHubKeyeventtargetrosteritem', '^originSocialHubRosterSection'],
                scope: {
                    'friend': '=',
                    'acceptStr': '@acceptstr',
                    'declineStr': '@declinestr',
                    'cancelStr': '@cancelstr',
                    'cancelandblocktitle': '=',
                    'cancelandblockdescription': '=',
                    'cancelandblockdescription2': '=',
                    'changeminddescription': '=',
                    'cancelandblockaccept': '=',
                    'cancelandblockdecline': '=',
                    'ignoreandblocktitle': '=',
                    'ignoreandblockdescription': '=',
                    'ignoreandblockdescription2': '=',
                    'ignoreandblockaccept': '=',
                    'ignoreandblockdecline': '=',
                    'errorfriendnotaddeddescription': '=',
                    'errorfriendnotaddedaccept': '=',
                    'errorfriendnotaddedtitle': '='
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/roster/views/pendingitem.html'),
                link: function (scope, element, attrs, ctrl) {
                    attrs = attrs;

                    var thisController   = ctrl[0],
                        keyeventRosteritemCtrl = ctrl[1],
                        sectionController = ctrl[2];

                    var $element = $(element).find('.origin-social-hub-roster-pendingitem'),
                        $wrapper = $element.closest('.origin-social-hub-roster-pendingitem-wrap');

                    thisController.setElements($element, $wrapper);

                    if (panelHeight === 0) {
                        thisController.setPanelHeight();
                    }

                    // Set up placeholder avatar
                    if (typeof scope.friend.avatarImgSrc === 'undefined') {
                        scope.friend.avatarImgSrc = ComponentsConfigFactory.getImagePath('social//avatar_placeholder.png');
                    }

                    /*
                    * Context menu functions
                    */
                    AppCommFactory.events.on('social:updateContextmenuState', function () {
                        scope.isContextmenuVisible = keyeventRosteritemCtrl.getContextmenuState();
                    });

                    AppCommFactory.events.on('social:triggerEnterEvent', function (data) {
                        var targetType = data.targetType;

                        if (targetType === 'contextmenuitem') {
                            if($(element).find(data.thisItem).length > 0) {
                                $(data.thisItem).triggerHandler('click');
                            }
                        }
                    });

                    // Accept button clicked
                    $(element).on('click', '.origin-social-hub-roster-pendingitem-accept-icon', function() {
                        var $thisElement = $(this);

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

                        }, function() {

                            // Accept friend request failed. Display error dialog
                            DialogFactory.openPrompt({
                                id: 'social-hub-roster-errorFriendNotAdded-id',
                                name: 'social-hub-roster-errorFriendNotAdded',
                                title: scope.errorfriendnotaddedtitleLoc.replace(/\'/g, "&#39;"),
                                description: scope.errorfriendnotaddeddescriptionLoc,
                                acceptText: scope.errorfriendnotaddedacceptLoc,
                                acceptEnabled: true,
                                rejectText: scope.cancelLoc,
                                defaultBtn: 'yes'
                            });

                            AppCommFactory.events.on('dialog:hide', thisController.handleErrorDialog);

                        });

                    });

                    // Decline button clicked
                    $(element).on('click', '.origin-social-hub-roster-pendingitem-decline-icon', function() {
                        thisController.onDeclineButtonClick(element);
                    });

                    // Cancel button clicked
                    $(element).on('click', '.origin-social-hub-roster-pendingitem-cancel-icon', function() {
                        thisController.onCancelRequestClick(element);
                    });

                }

            };

        });
}());
