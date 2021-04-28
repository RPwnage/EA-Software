(function($){
"use strict";

var ChatChannelContextMenu, ChatChannelContextMenuManager;

ChatChannelContextMenu = function()
{
    // Create a new native context menu
    this.platformMenu = nativeMenu.createMenu();

    this.actions = {};
    
    this.addAction('joinRoom', tr('ebisu_client_group_join_room'), function()
    {
        if (this.chatChannel.locked)
        {
            clientNavigation.showEnterRoomPassword(this.chatChannel.groupGuid, this.chatChannel.channelId);
        }
        else
        {
            this.chatChannel.joinChannel();
        }
    });
    
    this.addAction('createRoom', tr('ebisu_client_create_room'), function()
    {
        clientNavigation.showCreateRoom("", this.chatGroup.groupGuid);
    });
    
    this.deleteSeparator = this.addSeparator();
    
    this.addAction('deleteRoom', tr('ebisu_client_delete_chat_room'), function()
    {
        clientNavigation.showDeleteRoom(this.chatGroup.groupGuid, this.chatChannel.channelId, this.chatChannel.name);
    });
};

ChatChannelContextMenu.prototype.addAction = function(id, label, callback)
{
    var action = this.platformMenu.addAction(label);

    action.triggered.connect($.proxy(callback, this));

    this.actions[id] = action;
};

ChatChannelContextMenu.prototype.addSeparator = function()
{
    return this.platformMenu.addSeparator();
};

ChatChannelContextMenu.prototype.update = function(chatChannel, chatGroup)
{
    var admin, $groups, $channel;
    this.chatChannel = chatChannel;
    this.chatGroup = chatGroup;
    
    admin = (chatGroup.role === 'superuser');
    
    $groups = $('#group-content ol.chat-groups');
    
    $channel = $groups.find('li[data-channel-id=' + chatChannel.id + ']');
    if ($channel.siblings().length === 0)
    {
        // We can't delete the only channel in a group
        this.actions.deleteRoom.enabled = false;
    }

    // If we are already in this room disable join option
    if ($channel.hasClass('member'))
    {
        this.actions.joinRoom.enabled = false;
    }
    
    this.actions.deleteRoom.visible = admin;
    this.actions.createRoom.visible = admin;
    this.deleteSeparator.visible = admin;
};

//
// Singleton modelled after hovercard manager
// 
ChatChannelContextMenuManager = function()
{
    var createChatChannelMenu, that = this;

    this.chatChannelMenu = null;
    this.hideCallback = null;

    createChatChannelMenu = function()
    {
        var menu = new ChatChannelContextMenu();
        menu.platformMenu.addObjectName("chatChannelContextMenu");
        menu.platformMenu.aboutToHide.connect($.proxy(function()
        {
            if (that.hideCallback)
            {
                that.hideCallback();
            }
        }, that));

        return menu;
    };

    this.popup = function(chatChannel, chatGroup, position)
    {
        // Kill the presence dropdown if we have one
        if (Origin.views.closePresenceDropdown)
        {
            Origin.views.closePresenceDropdown();
        }

        // Mac OIG depends on us creating a new context menu each time
        if (this.chatChannelMenu !== null)
        {
            // hide any existing context menus so we only display one at a time
            this.chatChannelMenu.platformMenu.hide();
            this.chatChannelMenu = null;
        }
        // Lazily build our context menu
        this.chatChannelMenu = createChatChannelMenu();
        
        this.chatChannelMenu.update(chatChannel, chatGroup);
            
        this.chatChannelMenu.platformMenu.popup(position);

        return true;
    };
};

if (!window.Origin) { window.Origin = {}; }

Origin.ChatChannelContextMenuManager = new ChatChannelContextMenuManager();

}(jQuery));
