/**
 * Created by tirhodes on 2015-03-06.
 * @file area.js
 */
(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialHubChatroomsAreaCtrl($scope, SocialDataFactory, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-social-hub-chatrooms-area';

        $scope.searchChatRoomsLoc = UtilFactory.getLocalizedStr($scope.searchChatroomsStr, CONTEXT_NAMESPACE, 'searchchatroomsstr');
        $scope.chatRoomInvitationsLoc = UtilFactory.getLocalizedStr($scope.chatRoomInvitationsStr, CONTEXT_NAMESPACE, 'chatroominvitationsstr');
        $scope.chatRoomsLoc = UtilFactory.getLocalizedStr($scope.chatRoomsStr, CONTEXT_NAMESPACE, 'chatroomsstr');

        $scope.listState = 'LOADING';
        $scope.nameFilter = '';

        this.setNameFilter = function (filterText) {
            $scope.nameFilter = filterText;
            $scope.$digest();
        };

        SocialDataFactory.getGroups().then(function (groups) {
            if (Object.keys(groups).length) {
                $scope.listState = 'LOADED';
            }
            else {
                $scope.listState = 'NOROOMS';
            }
            $scope.$digest();
        }, function () {
            $scope.listState = 'ERROR';
            $scope.$digest();
        });

        function onGroupListResults() {
            $scope.listState = 'LOADED';
            $scope.$digest();
        }

        function onDestroy() {
            SocialDataFactory.events.off('socialGroupListResults', onGroupListResults);
        }

        SocialDataFactory.events.on('socialGroupListResults', onGroupListResults);
        $scope.$on('$destroy', onDestroy);
    }

    /**
    * @ngdoc directive
    * @name origin-components.directives:originSocialHubChatroomsArea
    * @restrict E
    * @element ANY
    * @scope
    * @param {LocalizedString} searchchatroomsstr "Search Chat Rooms"
    * @param {LocalizedString} chatroomsinvitationsstr "CHAT ROOM INVITATIONS"
    * @param {LocalizedString} chatroomsstr "CHAT ROOMS"
    * @description
    *
    * origin social hub -> chatrooms area
    *
    * @example
    * <example module="origin-components">
    *     <file name="index.html">
    *         <origin-social-hub-chatrooms-area
    *            searchchatroomsstr="Search Chat Rooms"
    *            chatroomsinvitationsstr="CHAT ROOM INVITATIONS"
    *            chatroomsstr="CHAT ROOMS"
    *         ></origin-social-hub-chatrooms-area>
    *     </file>
    * </example>
    *
    */

    angular.module('origin-components')
        .controller('OriginSocialHubChatroomsAreaCtrl', OriginSocialHubChatroomsAreaCtrl)
        .directive('originSocialHubChatroomsArea', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                scope: {
                    searchChatroomsStr: '@searchchatroomsstr',
                    chatRoomInvitationsStr: '@chatroominvitationsstr',
                    chatRoomsStr: '@chatroomsstr'
                },
                controller: 'OriginSocialHubChatroomsAreaCtrl',
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/chatrooms/views/area.html'),
                link: function (scope, element, attrs, ctrl) {

                    // Filter
                    $(element).on('keyup', '.origin-social-hub-chatrooms-area-rooms-filter-group-field > input', function () {
                        var text = $(element).find('.origin-social-hub-chatrooms-area-rooms-filter-group-field > input').val();
                        ctrl.setNameFilter(text);
                    });

                    // Listen for clicks on the "error loading chatrooms" cta button
                    $(element).on('buttonClicked', '.origin-social-hub-chatrooms-area-section-error origin-social-hub-cta', function () {
                        window.alert('FEATURE NOT IMPLEMENTED - Error Loading Chatrooms CTA Button Clicked');
                    });

                    // Listen for clicks on the "you have no chatrooms" cta button
                    $(element).on('buttonClicked', '.origin-social-hub-chatrooms-area-section-norooms origin-social-hub-cta', function () {
                        window.alert('FEATURE NOT IMPLEMENTED - No Chatrooms CTA Button Clicked');
                    });

                }

            };

        });
} ());

