/**
 * Created by tirhodes on 2015-02-23.
 * @file pendingitem.js
 */
(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialHubRosterPendingitemCtrl($scope, $window, $timeout, RosterDataFactory, UserDataFactory, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-social-hub-roster-pendingitem';

        $scope.acceptLoc = UtilFactory.getLocalizedStr($scope.acceptStr, CONTEXT_NAMESPACE, 'acceptstr');
        $scope.declineLoc = UtilFactory.getLocalizedStr($scope.declineStr, CONTEXT_NAMESPACE, 'declinestr');
        $scope.cancelLoc = UtilFactory.getLocalizedStr($scope.cancelStr, CONTEXT_NAMESPACE, 'cancelstr');
        $scope.rosterContextmenuVisible = false;
        $scope.contextmenuCallbacks = {};

        var $element, $wrapper;

        this.setElements = function(element, wrapper) {
            $element = element;
            $wrapper = wrapper;
        };

        function requestAvatar(nucleusId) {
            UserDataFactory.getAvatar(nucleusId, 'AVATAR_SIZE_MEDIUM')
                .then(function(response) {
                    $scope.friend.avatarImgSrc = response;
                    $scope.$digest();
                }, function() {

                }).catch(function(error) {
                    Origin.log.exception(error, 'OriginSocialHubRosterFriendCtrl: UserDataFactory.getAvatar failed');
                });
        }

        this.onAcceptButtonClick = function (element) {
            $timeout(function () {
                $wrapper.slideUp(300, function () {
                    RosterDataFactory.friendRequestAccept($scope.friend.nucleusId);
                    $(element).remove();
                });
            }, 500);
        };

        this.onCancelRequestClick = function (element) {
            $timeout(function () {
                $wrapper.slideUp(300, function () {
                    RosterDataFactory.cancelPendingFriendRequest($scope.friend.nucleusId);
                    $(element).remove();
                });
            }, 500);
        };

        this.onDeclineButtonClick = function (element) {
            $wrapper.slideUp(300, function () {
                RosterDataFactory.friendRequestReject($scope.friend.nucleusId);
                $(element).remove();
            });
        };

		function handleNameUpdate(friend) {
			$scope.friend.originId = friend.EAID;
			$scope.friend.firstName = friend.firstName;
			$scope.friend.lastName = friend.lastName;
			$scope.$digest();
		}

        /* Context menu callbacks */
        $scope.contextmenuCallbacks.acceptRequest = function () {
            $scope.rosterContextmenuVisible = false;
            $element.find('.origin-social-hub-roster-pendingitem-accept-icon').trigger('click');
        };

        $scope.contextmenuCallbacks.ignoreRequest = function () {
            $scope.rosterContextmenuVisible = false;
            $element.find('.origin-social-hub-roster-pendingitem-decline-icon').trigger('click');
        };

        $scope.contextmenuCallbacks.cancelRequest = function () {
            $scope.rosterContextmenuVisible = false;
            $element.find('.origin-social-hub-roster-pendingitem-cancel-icon').trigger('click');
        };

        $scope.contextmenuCallbacks.viewProfile = function () {
            $window.alert('View profile coming soon');
            $scope.rosterContextmenuVisible = false;
        };

        $scope.contextmenuCallbacks.cancelAndBlockRequest = function () {
            $window.alert('Cancel, yes. Block, no.');
            $scope.rosterContextmenuVisible = false;
        };

        $scope.contextmenuCallbacks.blockPendingRequestor = function () {
            $window.alert('You have requested to block a pending requestor. Too bad -- youre stuck with them!');
            $scope.rosterContextmenuVisible = false;
        };

        UserDataFactory.events.on('socialFriendsFullNameUpdated:' + $scope.friend.nucleusId, handleNameUpdate);
        UserDataFactory.getUserRealName($scope.friend.nucleusId); // request the originId

        requestAvatar($scope.friend.nucleusId);


        function onDestroy() {
            UserDataFactory.events.off('socialFriendsFullNameUpdated:' + $scope.friend.nucleusId, handleNameUpdate);
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
     *         ></origin-social-hub-roster-pendingitem>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubRosterPendingitemCtrl', OriginSocialHubRosterPendingitemCtrl)
        .directive('originSocialHubRosterPendingitem', function(ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialHubRosterPendingitemCtrl',
                scope: {
                    'friend': '=',
                    'acceptStr': '@acceptstr',
                    'declineStr': '@declinestr',
                    'cancelStr': '@cancelstr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/roster/views/pendingitem.html'),
                link: function (scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    var $element = $(element).find('.origin-social-hub-roster-pendingitem'),
                        $wrapper = $element.closest('.origin-social-hub-roster-pendingitem-wrap');

                    ctrl.setElements($element, $wrapper);

                    function removeOtkClass () {
                        $('.otktooltip-isvisible').removeClass('otktooltip-isvisible');
                    }

					// Set up placeholder avatar
					if (typeof scope.friend.avatarImgSrc === 'undefined') {
						scope.friend.avatarImgSrc = ComponentsConfigFactory.getImagePath('social//avatar_placeholder.png');
					}

                    /*
                    * Context menu functions
                    */
                    function isSelectedItem () {
                        return $(element).closest(document.activeElement).length > 0;
                    }

                    // Catches event from keyeventtarget .js
                    scope.$on('newSocialSelection', function (e, data) {

                        if (data.triggerSource === 'leftClick' && !scope.rosterContextmenuVisible) {
                            scope.rosterContextmenuVisible = isSelectedItem();
                        } else if (data.triggerSource === 'rightClick') {
                            scope.rosterContextmenuVisible = isSelectedItem();
                        } else {
                            scope.rosterContextmenuVisible = false;
                        }

                        scope.$digest();
                    });

                    // Hides context menu from clicking away
                    $(document).on('click', function (event) {
                        var $target = $(event.target);

                        if(!$target.hasClass('origin-social-hub-roster-contextmenu-otkicon-downarrow')) {
                            scope.rosterContextmenuVisible = false;
                            scope.$digest();
                         }
                    });

                    // Accept button clicked
                    $(element).on('click', '.origin-social-hub-roster-pendingitem-accept-icon', function() {

                        var $icon = $(this).closest('.origin-social-hub-roster-pendingitem-icon');

                        // Change background
                        $element.addClass('origin-social-hub-roster-pendingitem-accepted');

                        // Icon
                        $icon.addClass('origin-social-hub-roster-pendingitem-accept-icon-accepted');

                        // Remove other items
                        $element.children().not('.origin-social-hub-roster-pendingitem-accept-icon-wrap').fadeOut();
                        removeOtkClass();

                        ctrl.onAcceptButtonClick(element);
                    });

                    // Decline button clicked
                    $(element).on('click', '.origin-social-hub-roster-pendingitem-decline-icon', function() {
                        removeOtkClass();
                        ctrl.onDeclineButtonClick(element);
                    });

                    // Cancel button clicked
                    $(element).on('click', '.origin-social-hub-roster-pendingitem-cancel-icon', function() {
                        var $icon = $(this).closest('.origin-social-hub-roster-pendingitem-icon');

                        // Change background
                        $element.addClass('origin-social-hub-roster-pendingitem-cancelled');

                        // Icon
                        $icon.addClass('origin-social-hub-roster-pendingitem-cancel-icon-cancelled');

                        // Remove other items
                        $element.children().not('.origin-social-hub-roster-pendingitem-cancel-icon-wrap').fadeOut();
						removeOtkClass();
                        ctrl.onCancelRequestClick(element);
                    });

                    // Handle tooltips
                    $(element).on('mouseover', '[data-has-tooltip]', function() {
                        var $tooltip = $(this).find('.otktooltip');

                        var elementWidth = $(this).width();
                        var elementHeight = $(this).height();
                        var tooltipWidth = $tooltip.width();

                        $tooltip.css({'left': (elementWidth/2)-(tooltipWidth/2) + 'px', 'top': -(elementHeight + 4) + 'px'});

                        $tooltip.addClass('otktooltip-isvisible');
                    });

                    $(element).on('mouseout', '[data-has-tooltip]', function() {
                        removeOtkClass();
                    });

                }

            };

        });
}());

