(function($){
"use strict";

var ChatGroupContextMenu, ChatGroupContextMenuManager;

ChatGroupContextMenu = function()
{
    // Create a new native context menu
    this.platformMenu = nativeMenu.createMenu();

    this.actions = {};
    
    /* remove channels
    this.addAction('createRoom', tr('ebisu_client_create_room'), function()
    {
        clientNavigation.showCreateRoom(this.chatGroup.name, this.chatGroup.groupGuid);
    });
    */

    this.addAction('joinRoom', tr('ebisu_client_group_join_room'), function()
    {
        if (this.chatGroup.channels.length > 0) 
        {
            this.chatGroup.channels[0].joinChannel();
        }
        else
        {
            this.chatGroup.failedToEnterRoom();
        }
    });
    
    this.addAction('inviteFriends', tr('ebisu_client_invite_friends_to_chat_group_ellipsis'), function()
    {
        if (originSocial.roster.hasFriends) {            
            clientNavigation.showInviteFriendsToGroupDialog(this.chatGroup.groupGuid);
        } else {
            clientNavigation.showYouNeedFriendsDialog();
        }   
    });
    
    this.addAction('editGroup', tr('ebisu_client_group_edit'), function()
    {
        clientNavigation.showEditGroup(this.chatGroup.name, this.chatGroup.groupGuid);
    });
    
    this.addAction('viewMembers', tr('ebisu_client_group_view_members'), function()
    {
        clientNavigation.showGroupMembersDialog(this.chatGroup.name, this.chatGroup.groupGuid);
    });
    
    this.windowSeparator = this.addSeparator();
    
    this.addAction('deleteGroup', tr('ebisu_client_group_delete'), function()
    {
        clientNavigation.showDeleteGroup(this.chatGroup.name, this.chatGroup.groupGuid);
    });
    
    this.addAction('leaveGroup', tr('ebisu_client_group_leave'), function()
    {
        clientNavigation.showLeaveGroup(this.chatGroup.name, this.chatGroup.groupGuid);
    });
    
    this.addAction('accept', tr('ebisu_client_action_accept_request'), function()
    {
        this.chatGroup.acceptGroupInvite();
    });
    
    this.addAction('ignore', tr('ebisu_client_action_ignore_request'), function()
    {
        this.chatGroup.rejectGroupInvite();
    });
};

ChatGroupContextMenu.prototype.addAction = function(id, label, callback)
{
    var action = this.platformMenu.addAction(label);

    action.triggered.connect($.proxy(callback, this));

    this.actions[id] = action;
};

ChatGroupContextMenu.prototype.addSeparator = function()
{
    return this.platformMenu.addSeparator();
};

ChatGroupContextMenu.prototype.update = function(chatGroup)
{
    this.chatGroup = chatGroup;
    
    if (chatGroup.inviteGroup)
    {
        this.windowSeparator.visible = false;
        this.actions.joinRoom.visible = false;
        this.actions.inviteFriends.visible = false;
        this.actions.editGroup.visible = false;
        this.actions.viewMembers.visible = false;
        this.actions.deleteGroup.visible = false;
        this.actions.leaveGroup.visible = false;
    }
    else
    {
        this.actions.accept.visible = false;
        this.actions.ignore.visible = false;
        
        switch (chatGroup.role)
        {
            case 'superuser':
                this.actions.joinRoom.visible = true;
                this.actions.inviteFriends.visible = true;
                this.actions.editGroup.visible = true;
                this.actions.viewMembers.visible = true;
                this.actions.deleteGroup.visible = true;
                this.actions.leaveGroup.visible = false;
                break;
            case 'admin':
            case 'member':
                this.actions.joinRoom.visible = true;
                this.actions.inviteFriends.visible = true;
                this.actions.editGroup.visible = false;
                this.actions.viewMembers.visible = true;
                this.actions.deleteGroup.visible = false;
                this.actions.leaveGroup.visible = true;
                break;
            default:
                console.warn('Unknown group rights: ' + chatGroup.rights);
        }
    }
    

};

//
// Singleton modelled after hovercard manager
// 
ChatGroupContextMenuManager = function()
{
    var createChatGroupMenu, that = this;

    this.chatGroupMenu = null;
    this.hideCallback = null;

    createChatGroupMenu = function()
    {
        var menu = new ChatGroupContextMenu();
        menu.platformMenu.addObjectName("ChatGroupContextMenu");
        menu.platformMenu.aboutToHide.connect($.proxy(function()
        {
            if (that.hideCallback)
            {
                that.hideCallback();
            }
        }, that));

        return menu;
    };

    this.popup = function(chatGroup, position)
    {
        // Kill the presence dropdown if we have one
        if (Origin.views.closePresenceDropdown)
        {
            Origin.views.closePresenceDropdown();
        }

        // Mac OIG depends on us creating a new context menu each time
        if (this.chatGroupMenu !== null)
        {
            // hide any existing context menus so we only display one at a time
            this.chatGroupMenu.platformMenu.hide();
            this.chatGroupMenu = null;
        }
        // Lazily build our context menu
        this.chatGroupMenu = createChatGroupMenu();
                    
        this.chatGroupMenu.update(chatGroup);

        this.chatGroupMenu.platformMenu.popup(position);

        return true;
    };
};

if (!window.Origin) { window.Origin = {}; }

Origin.ChatGroupContextMenuManager = new ChatGroupContextMenuManager();

}(jQuery));
