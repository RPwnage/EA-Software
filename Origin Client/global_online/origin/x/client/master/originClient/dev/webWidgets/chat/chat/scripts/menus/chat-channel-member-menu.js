(function($){
"use strict";

var ChatChannelMemberContextMenu, ChatChannelMemberContextMenuManager;

ChatChannelMemberContextMenu = function()
{
    // Create a new native context menu
    this.platformMenu = nativeMenu.createMenu();

    this.actions = {};
    
    this.addAction('sendMessage', tr('ebisu_client_send_message'), function()
    {
        this.user.startConversation();
    });
    
    this.addAction('voiceChat', tr('ebisu_client_start_one_on_one_voice_chat'), function()
    {
        this.user.startVoiceConversation();
    });
    
    this.addAction('profile', tr('ebisu_client_action_profile'), function()
    {
        this.user.showProfile("CHANNEL_MENU");
    });
    
    this.addAction('subscribe', tr('ebisu_client_soref_send_request'), function()
    {
        this.user.requestSubscription();
    });
    
    this.addAction('acceptSubscription', tr('ebisu_client_action_accept_request'), function()
    {
        this.user.acceptSubscriptionRequest();
    });
    
    this.addAction('rejectSubscription', tr('ebisu_client_action_ignore_request'), function()
    {
        this.user.rejectSubscriptionRequest();
    });
    
    this.muteSeparator = this.addSeparator();
    
    this.addAction('mute', tr('ebisu_client_mute_your_microphone'), function()
    {
        this.conversation.muteSelfInVoice();
    });
    
    this.addAction('unmute', tr('Ebisu_client_unmute_your_microphone'), function()
    {
        this.conversation.unmuteSelfInVoice();
    });
    
    this.addAction('localMute', tr('ebisu_client_mute_user_only_for_yourself'), function()
    {
        this.conversation.muteUserInVoice(this.user);
    });
    
    this.addAction('localUnmute', tr('ebisu_client_unmute_user_only_for_yourself'), function()
    {
        this.conversation.unmuteUserInVoice(this.user);
    });
    
    this.addAction('adminMute', tr('ebisu_client_mute_user_for_everyone'), function()
    {
        this.conversation.muteUserByAdmin(this.user);
    });
    
    this.joinSeparator = this.addSeparator();
    
    this.addAction('joinGame', tr('ebisu_client_action_join'), function()
    {
        this.user.joinGame();
    });
    
    this.addAction('watchBroadcast', tr('ebisu_client_watch_broadcast'), function()
    {
        telemetryClient.sendBroadcastJoined('UserMenu');
        clientNavigation.launchExternalBrowser(this.user.playingGame.broadcastUrl);
    });
    
    this.addAction('inviteToGame', tr('ebisu_client_action_invite_to_game'), function()
    {
        this.user.inviteToGame();
    });
    
    this.moderateSeparator = this.addSeparator();
    
    this.addAction('promote', tr('ebisu_client_promote_to_admin'), function()
    {
        var user = this.user;
        var groupName = this.chatGroup.name;
        var successCallback = function() {
            user.promoteToAdminSuccess.disconnect(successCallback);
            clientNavigation.showPromoteToAdminSuccessDialog(user, groupName);
        };
        this.user.promoteToAdminSuccess.connect(successCallback);
        this.user.promoteUserToAdmin(this.chatChannel.groupGuid);
    });
    
    this.addAction('transfer', tr('ebisu_client_transfer_ownership'), function()
    {
        this.user.transferGroupOwnership(this.chatChannel.groupGuid);
    });
    
    this.addAction('demote', tr('ebisu_client_demote_admin_to_member'), function()
    {
        var user = this.user;
        var groupName = this.chatGroup.name;
        var successCallback = function() {
            user.demoteToMemberSuccess.disconnect(successCallback);
            clientNavigation.showDemoteToMemberSuccessDialog(user, groupName);
        };
        this.user.demoteToMemberSuccess.connect(successCallback);
        this.user.demoteAdminToMember(this.chatChannel.groupGuid);
    });
    
    this.addAction('kickFromRoom', tr('ebisu_client_kick_from_room'), function()
    {
        clientNavigation.showRemoveRoomUser(this.chatChannel.groupGuid, this.chatChannel.channelId, this.user);
    });
    
    this.addAction('kickFromGroup', tr('ebisu_client_kick_from_chat_group'), function()
    {
        clientNavigation.showRemoveGroupUser(this.chatChannel.groupGuid, this.user);
    });
    
    this.blockSeparator = this.addSeparator();
    
    this.addAction('leaveRoom', tr('ebisu_client_leave_chat_room'), function()
    {
        this.conversation.requestLeaveConversation();
    });

    this.addAction('block', tr('ebisu_client_unfriend_and_block'), function ()
    {
        this.user.block();
    });
    
    this.addAction('unblock', tr('ebisu_client_unblock'), function()
    {
        this.user.unblock();
    });
    
    this.addAction('remove', tr('ebisu_client_action_delete_friend'), function()
    {
        originSocial.roster.removeContact(this.user);
    });
    
    this.addAction('revoke', tr('ebisu_client_revoke_friend_request'), function()
    {
        this.user.cancelSubscriptionRequest();
    });
    
};

ChatChannelMemberContextMenu.prototype.addAction = function(id, label, callback)
{
    var action = this.platformMenu.addAction(label);

    action.triggered.connect($.proxy(callback, this));

    this.actions[id] = action;
};

ChatChannelMemberContextMenu.prototype.addSeparator = function()
{
    return this.platformMenu.addSeparator();
};

ChatChannelMemberContextMenu.prototype.update = function(user, chatChannel, chatGroup, conversation)
{
    var blocked, incomingRequest, outgoingRequest, fullySubscribed, owner, admin, remoteUserRole, remoteUserMember, remoteUserAdmin, remoteUserOwner, participantRole,
        remoteUserVoiceState, remoteUserInVoice, remoteUserLocalMuted, remoteUserAdminMuted, isMe, userJoinable, userBroadcasting, pushToTalk;
    
    pushToTalk = clientSettings.readBoolSetting("EnablePushToTalk");

    this.user = user;
    this.conversation = conversation;
    this.chatChannel = chatChannel;
    this.chatGroup = chatGroup;
    
    isMe = (user == originSocial.currentUser);
    
    if (isMe)
    {
        this.actions.sendMessage.visible = false;
        this.actions.voiceChat.visible = false;
        this.actions.subscribe.visible = false;
        this.actions.acceptSubscription.visible = false;
        this.actions.rejectSubscription.visible = false;
        this.muteSeparator.visible = false;
        this.actions.localMute.visible = false;
        this.actions.localUnmute.visible = false;
        this.actions.adminMute.visible = false;
        this.joinSeparator.visible = false;
        this.actions.joinGame.visible = false;
        this.actions.watchBroadcast.visible = false;
        this.actions.inviteToGame.visible = false;
        this.moderateSeparator.visible = false;
        this.actions.promote.visible = false;
        this.actions.transfer.visible = false;
        this.actions.demote.visible = false;
        this.actions.kickFromRoom.visible = false;
        this.actions.kickFromGroup.visible = false;
        this.actions.block.visible = false;
        this.actions.unblock.visible = false;
        this.actions.remove.visible = false;
        this.actions.revoke.visible = false;
        this.actions.leaveRoom.visible = false; //remove channels was: true;
        
        this.actions.mute.visible = false;
        this.actions.unmute.visible = false;
        
        if (conversation && conversation.isVoiceCallInProgress)
        {
            if (conversation.voiceCallState.indexOf("VOICECALL_MUTED") !== -1)
            {
                this.actions.unmute.visible = true;
            }
            else if (!pushToTalk)
            {
                this.actions.mute.visible = true;
            }
        }
    }
    else
    {
        // Deal with requests versus full subscribed contacts
        incomingRequest = false;
        outgoingRequest = false;
        fullySubscribed = true;
        
        userJoinable = function(user)
        {
            return !!(user.playingGame && user.playingGame.joinable);
        };
        
        userBroadcasting = function(user)
        {
            return !!(user.playingGame && user.playingGame.broadcastUrl);
        };
            
        if (user.subscriptionState.direction !== 'BOTH')
        {
            // Currenty not friends for some reason
            fullySubscribed = false;
            
            outgoingRequest = user.subscriptionState.pendingContactApproval;
            incomingRequest = user.subscriptionState.pendingCurrentUserApproval;
        }
        
        blocked = user.blocked;
        
        owner = (chatGroup.role === 'superuser');
        admin = (chatGroup.role === 'admin');
        
        remoteUserRole = chatGroup.getRemoteUserRole(user);
        
        if (remoteUserRole.length > 0) 
        { 
            // if we know the member role, then use it
            remoteUserMember = (remoteUserRole === 'member');
            remoteUserAdmin = (remoteUserRole === 'admin');
            remoteUserOwner = (remoteUserRole === 'owner');
        } else {
            // try to get the participant role
            participantRole = chatChannel.getParticipantRole(user);
            remoteUserMember = (participantRole === 'participant');
            remoteUserAdmin = (participantRole === 'moderator');            
        }
            
        this.actions.sendMessage.visible = fullySubscribed;
        this.actions.voiceChat.visible = fullySubscribed && (user.capabilities.indexOf("VOIP") !== -1);
        
        this.actions.subscribe.visible = !fullySubscribed && !outgoingRequest && !incomingRequest;
        
        this.actions.remove.visible = fullySubscribed;
        this.actions.revoke.visible = outgoingRequest;
        
        this.actions.acceptSubscription.visible = incomingRequest;
        this.actions.rejectSubscription.visible = incomingRequest;
        
        this.actions.block.visible = !blocked;
        this.actions.unblock.visible = blocked;
        
        this.moderateSeparator.visible = admin || owner;
        this.actions.promote.visible = (admin || owner) && remoteUserMember;
        this.actions.transfer.visible = owner;
        this.actions.demote.visible = owner && remoteUserAdmin;
            
        if (remoteUserMember)
        {
            this.actions.kickFromGroup.visible = admin || owner;
        }
        else if (remoteUserAdmin)
        {
            this.actions.kickFromGroup.visible = owner;
        }
        else
        {
            this.actions.kickFromGroup.visible = false;
        }        
        
        this.actions.mute.visible = false;
        this.actions.unmute.visible = false;
        this.actions.leaveRoom.visible = false;
        
        this.actions.joinGame.enabled = userJoinable(user);        
        this.actions.watchBroadcast.enabled = userBroadcasting(user);
        this.actions.inviteToGame.enabled = userJoinable(originSocial.currentUser);
        
        if (conversation)
        {
            remoteUserVoiceState = conversation.remoteUserVoiceCallState(user);
            remoteUserInVoice = (remoteUserVoiceState.indexOf("REMOTE_VOICE_ACTIVE") !== -1);
            remoteUserLocalMuted = (remoteUserVoiceState.indexOf("REMOTE_VOICE_LOCAL_MUTE") !== -1);
            remoteUserAdminMuted = (remoteUserVoiceState.indexOf("REMOTE_VOICE_REMOTE_MUTE") !== -1);
            
            this.actions.localMute.visible = conversation.isVoiceCallInProgress && !remoteUserLocalMuted && !remoteUserAdminMuted && remoteUserInVoice;
            this.actions.localUnmute.visible = conversation.isVoiceCallInProgress && remoteUserLocalMuted && remoteUserInVoice;

            if (remoteUserMember)
            {
                this.actions.kickFromRoom.visible = false; // remove channels  admin || owner;
                this.actions.adminMute.visible = conversation.isVoiceCallInProgress && (admin || owner) && !remoteUserLocalMuted && !remoteUserAdminMuted && remoteUserInVoice;
            }
            else if (remoteUserAdmin)
            {
                this.actions.kickFromRoom.visible = false; // remove channels owner;
                this.actions.adminMute.visible = conversation.isVoiceCallInProgress && owner && !remoteUserLocalMuted && !remoteUserAdminMuted && remoteUserInVoice;
            }
            else
            {
                this.actions.kickFromRoom.visible = false;
                this.actions.adminMute.visible = false;
            }
            
            this.muteSeparator.visible = conversation.isVoiceCallInProgress && remoteUserInVoice;
        }
        else
        {        
            this.actions.localMute.visible = false;
            this.actions.localUnmute.visible = false;
            
            this.actions.kickFromRoom.visible = false;
            
            this.actions.adminMute.visible = false;
            
            this.muteSeparator.visible = false;
        }
    }
};

//
// Singleton modelled after hovercard manager
// 
ChatChannelMemberContextMenuManager = function()
{
    var createChatChannelMemberMenu, that = this;

    this.chatChannelMemberMenu = null;
    this.hideCallback = null;

    createChatChannelMemberMenu = function()
    {
        var menu = new ChatChannelMemberContextMenu();
        menu.platformMenu.addObjectName("ChatChannelMemberContextMenu");
        menu.platformMenu.aboutToHide.connect($.proxy(function()
        {
            if (that.hideCallback)
            {
                that.hideCallback();
            }
        }, that));

        return menu;
    };

    this.popup = function(user, chatChannel, chatGroup, conversation, position)
    {
        // Kill the presence dropdown if we have one
        if (Origin.views.closePresenceDropdown)
        {
            Origin.views.closePresenceDropdown();
        }

        // Mac OIG depends on us creating a new context menu each time
        if (this.chatChannelMemberMenu !== null)
        {
            // hide any existing context menus so we only display one at a time
            this.chatChannelMemberMenu.platformMenu.hide();
            this.chatChannelMemberMenu = null;
        }
        // Lazily build our context menu
        this.chatChannelMemberMenu = createChatChannelMemberMenu();
            
        this.chatChannelMemberMenu.update(user, chatChannel, chatGroup, conversation);
            
        this.chatChannelMemberMenu.platformMenu.popup(position);
        
        return true;
    };
};

if (!window.Origin) { window.Origin = {}; }

Origin.ChatChannelMemberContextMenuManager = new ChatChannelMemberContextMenuManager();

}(jQuery));
