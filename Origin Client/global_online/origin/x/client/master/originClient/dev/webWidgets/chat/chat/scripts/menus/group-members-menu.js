(function($){
"use strict";

var GroupMembersContextMenu, GroupMembersContextMenuManager;

GroupMembersContextMenu = function()
{
    // Create a new native context menu
    this.platformMenu = nativeMenu.createMenu();

    this.actions = {};
    
    this.addAction('chat', tr('ebisu_client_action_chat'), function()
    {
        this.user.startConversation();
    });
    
    this.addAction('voiceChat', tr('ebisu_client_start_voice_chat'), function()
    {
        this.user.startVoiceConversation();
    });
    
    this.addAction('profile', tr('ebisu_client_action_profile'), function()
    {
        this.user.showProfile("GROUP_MEMBERSHIP_LIST");
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
        
    this.adminSeparator = this.addSeparator();
    
    this.addAction('promoteToAdmin', tr('ebisu_client_promote_to_admin'), function()
    {
        window.groupMembersWindow.promoteUserToAdmin(this.user, this.groupGuid);
    });
    
    this.addAction('demoteToMember', tr('ebisu_client_demote_admin_to_member'), function()
    {
        window.groupMembersWindow.demoteAdminToMember(this.user, this.groupGuid);
    });
        
    this.addAction('transferOwnership', tr('ebisu_client_transfer_ownership'), function()
    {
        window.groupMembersWindow.transferGroupOwner(this.user, this.groupGuid);
    });
    
    this.addAction('removeFromGroup', tr('ebisu_client_remove_from_chat_group'), function()
    {
        clientNavigation.showRemoveGroupUser(this.groupGuid, this.user);
    });
    
    this.blockSeparator = this.addSeparator();

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
    
    return this;
};

GroupMembersContextMenu.prototype.addAction = function(id, label, callback)
{
    var action = this.platformMenu.addAction(label);

    action.triggered.connect($.proxy(callback, this));

    this.actions[id] = action;
};

GroupMembersContextMenu.prototype.addSeparator = function()
{
    return this.platformMenu.addSeparator();
};

GroupMembersContextMenu.prototype.update = function(user, groupGuid, role, currentUserRole)
{    
    var blocked = user.blocked, 
        incomingRequest = false, 
        outgoingRequest = false, 
        fullySubscribed = true,
        userAvailable = false,
        isMe;

    this.user = user;
    this.groupGuid = groupGuid;

    isMe = (user == originSocial.currentUser);

    if (isMe)
    {
        this.actions.chat.visible = false;
        this.actions.voiceChat.visible = false;
        this.actions.profile.visible = true; // this is the only option available
        this.actions.subscribe.visible = false;
        this.actions.acceptSubscription.visible = false;
        this.actions.rejectSubscription.visible = false;
        this.actions.promoteToAdmin.visible = false;
        this.actions.demoteToMember.visible = false;
        this.actions.transferOwnership.visible = false;
        this.actions.removeFromGroup.visible = false;
        this.actions.block.visible = false;
        this.actions.unblock.visible = false;
        this.actions.remove.visible = false;
        this.actions.revoke.visible = false;
        this.adminSeparator.visible = false;
        this.blockSeparator.visible = false;
    }
    else
    {
        switch (currentUserRole)
        {
            case 'superuser':
                this.actions.promoteToAdmin.visible = (role === "member");
                this.actions.demoteToMember.visible = (role === "admin");
                this.actions.transferOwnership.visible = true;
                this.actions.removeFromGroup.visible = true;
                this.adminSeparator.visible = true;
                break;
            case 'admin':
                this.actions.promoteToAdmin.visible = (role === "member");
                this.actions.demoteToMember.visible = false;
                this.actions.transferOwnership.visible = false;
                this.actions.removeFromGroup.visible = (role === "member");
                this.adminSeparator.visible = true;
                break;
            case 'member':
                this.actions.promoteToAdmin.visible = false;
                this.actions.demoteToMember.visible = false;
                this.actions.transferOwnership.visible = false;
                this.actions.removeFromGroup.visible = false;
                this.adminSeparator.visible = false;
                break;
            default:
                console.warn('Unknown user role: ' + currentUserRole);
        }

        userAvailable = user.availability !== 'UNAVAILABLE';
        // Deal with requests versus full subscribed contacts    
        if (user.subscriptionState.direction !== 'BOTH')
        {
            // Currently not friends for some reason
            fullySubscribed = false;
        
            outgoingRequest = user.subscriptionState.pendingContactApproval;
            incomingRequest = user.subscriptionState.pendingCurrentUserApproval;
        }
        
        this.actions.chat.visible = fullySubscribed;
        this.actions.voiceChat.visible = fullySubscribed && (user.capabilities.indexOf("VOIP") !== -1) && userAvailable;
    
        this.actions.subscribe.visible = !fullySubscribed && !outgoingRequest && !incomingRequest;
    
        this.actions.remove.visible = fullySubscribed;
        this.actions.revoke.visible = outgoingRequest;
    
        this.actions.acceptSubscription.visible = incomingRequest;
        this.actions.rejectSubscription.visible = incomingRequest;
    
        this.blockSeparator.visible = true;
        this.actions.block.visible = !blocked;
        this.actions.unblock.visible = blocked;
    }
};

//
// Singleton modelled after hovercard manager
// 
GroupMembersContextMenuManager = function()
{
    var createGroupMembersMenu, that = this;

    this.groupMembersMenu = null;
    this.hideCallback = null;

    createGroupMembersMenu = function()
    {
        var menu = new GroupMembersContextMenu();
        menu.platformMenu.addObjectName("GroupMembersContextMenu");
        menu.platformMenu.aboutToHide.connect($.proxy(function()
        {
            if (that.hideCallback)
            {
                that.hideCallback();
            }
        }, that));

        return menu;
    };

    this.popup = function(user, groupGuid, role, currentUserRole, position)
    {
        // Mac OIG depends on us creating a new context menu each time
        if (Origin.utilities.currentPlatform==="MAC-ORIGIN" && this.GroupMembersMenu !== null)
        {
            // hide any existing context menus so we only display one at a time
            this.GroupMembersMenu.platformMenu.hide();
            
            // Lazily build our context menu
            this.GroupMembersMenu = createGroupMembersMenu();
        } else if (!this.GroupMembersMenu) {
            // Lazily build our context menu
            this.GroupMembersMenu = createGroupMembersMenu();
        }
                    
        this.GroupMembersMenu.update(user, groupGuid, role, currentUserRole);

        this.GroupMembersMenu.platformMenu.popup(position);

        return true;
    };    
    return this;
};

if (!window.Origin) { window.Origin = {}; }

Origin.GroupMembersContextMenuManager = new GroupMembersContextMenuManager();

}(jQuery));
