(function($){
"use strict";

var GroupContextMenu, GroupContextMenuManager;

GroupContextMenu = function()
{
    // Create a new native context menu
    this.platformMenu = nativeMenu.createMenu();

    this.actions = {};
    
    this.addAction('createGroup', tr('ebisu_client_create_chat_room'), function()
    {
        clientNavigation.showCreateGroupDialog();
    });
};


GroupContextMenu.prototype.update = function()
{
    this.actions.createGroup.enabled = (originSocial.currentUser.visibility !== 'INVISIBLE');
};

GroupContextMenu.prototype.addAction = function(id, label, callback)
{
    var action = this.platformMenu.addAction(label);

    action.triggered.connect($.proxy(callback, this));

    this.actions[id] = action;
};

GroupContextMenu.prototype.addSeparator = function()
{
    return this.platformMenu.addSeparator();
};

//
// Singleton modelled after hovercard manager
// 
GroupContextMenuManager = function()
{
    var createGroupMenu, that = this;

    this.groupMenu = null;
    this.hideCallback = null;

    createGroupMenu = function()
    {
        var menu = new GroupContextMenu();
        menu.platformMenu.addObjectName("GroupContextMenu");
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
        if (this.groupMenu !== null)
        {
            // hide any existing context menus so we only display one at a time
            this.groupMenu.platformMenu.hide();
            this.groupMenu = null;
        }
        // Lazily build our context menu
        this.groupMenu = createGroupMenu();

        this.groupMenu.update();
        
        this.groupMenu.platformMenu.popup(position);

        return true;
    };
};

if (!window.Origin) { window.Origin = {}; }

Origin.GroupContextMenuManager = new GroupContextMenuManager();

}(jQuery));
