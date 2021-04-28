/**
 * Created by tirhodes on 2015-03-06.
 * @file room.js
 */
(function () {

    'use strict';
    
    /**
    * The controller
    */
    function OriginSocialHubChatroomsRoomCtrl($scope, ConversationFactory, SocialDataFactory, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-social-hub-chatrooms-room';

        $scope.joinConversationsLoc = UtilFactory.getLocalizedStr($scope.joinConversationsStr, CONTEXT_NAMESPACE, 'joinconversationstr');
        $scope.inviteFriendsToChatroomLoc = UtilFactory.getLocalizedStr($scope.inviteFriendsToChatroomStr, CONTEXT_NAMESPACE, 'invitefriendstochatroomstr');
        $scope.editChatroomLoc = UtilFactory.getLocalizedStr($scope.editChatroomStr, CONTEXT_NAMESPACE, 'editchatroomstr');
        $scope.viewChatroomMembersLoc = UtilFactory.getLocalizedStr($scope.viewChatroomMembersStr, CONTEXT_NAMESPACE, 'viewchatroommembersstr');
        $scope.deleteChatroomLoc = UtilFactory.getLocalizedStr($scope.deleteChatroomStr, CONTEXT_NAMESPACE, 'deletechatroomstr');

        this.joinRoom = function(jid) {
            if (jid.length > 0) {
                ConversationFactory.joinConversation(jid);
            }
        };
        
        this.acceptInvite = function(jid) {
            if (jid.length > 0) {
                SocialDataFactory.acceptGroupInvite(jid);
            }
        };
        
        this.declineInvite = function(jid) {
            if (jid.length > 0) {
                SocialDataFactory.declineGroupInvite(jid);
            }
        };
        
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubChatroomsRoom
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} chatroom Javascript object representing a chat room
     * @param {number} index the index of the chat room
     * @param {LocalizedString} joinconversationstr "Join Conversation"
     * @param {LocalizedString} invitefriendstochatroomstr "Invite Friends to Chat Room..."
     * @param {LocalizedString} editchatroomstr "Edit Chat Room..."
     * @param {LocalizedString} viewchatroommembersstr "View Chat Room Members..."
     * @param {LocalizedString} deletechatroomstr "Delete Chat Room"
     * @description
     *
     * origin social hub -> chatrooms room
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-hub-chatrooms-room
     *            joinconversationstr="Join Conversation"
     *            invitefriendstochatroomstr "Invite Friends to Chat Room..."
     *            editchatroomstr="Edit Chat Room..."
     *            viewchatroommembersstr="View Chat Room Members..."
     *            deletechatroomstr="Delete Chat Room"
     *         ></origin-social-hub-chatrooms-room>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubChatroomsRoomCtrl', OriginSocialHubChatroomsRoomCtrl)
        .directive('originSocialHubChatroomsRoom', function(ComponentsConfigFactory) {
        
            return {
                restrict: 'E',
                controller: 'OriginSocialHubChatroomsRoomCtrl',
                scope: {
                    chatroom: '=',
                    index: '@',
                    joinConversationsStr: '@joinconversationstr',
                    inviteFriendsToChatroomStr: '@invitefriendstochatroomstr',
                    editChatroomStr: '@editchatroomstr',
                    viewChatroomMembersStr: '@viewchatroommembersstr',
                    deleteChatroomStr: '@deletechatroomstr'
                },
                link: function(scope, element, attrs, ctrl) {                               
                    $(element).on('dblclick', '.origin-social-hub-chatrooms-room-name', function(event) { 
                        ctrl.joinRoom($(event.target).find('#origin-social-hub-chatrooms-room-jid-input').val());
                    });
                    
                    $(element).on('click', '.origin-social-hub-roster-chatrooms-room-accept-icon', function() { 
                        ctrl.acceptInvite($(element).find('#origin-social-hub-chatrooms-group-guid-input').val());
                    });

                    $(element).on('click', '.origin-social-hub-roster-chatrooms-room-decline-icon', function() { 
                        ctrl.declineInvite($(element).find('#origin-social-hub-chatrooms-group-guid-input').val());
                    });
                    
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/chatrooms/views/room.html')                
            };
        
        });
}());

