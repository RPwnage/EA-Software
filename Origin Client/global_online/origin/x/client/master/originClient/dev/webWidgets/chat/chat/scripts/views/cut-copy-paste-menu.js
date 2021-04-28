(function ($) {
"use strict";

var CutCopyPasteContextMenu, CutCopyPasteContextMenuManager;

// This is heavily based on game-contextmenu from My Games
CutCopyPasteContextMenu = function()
{
    // Create a new native context menu
    this.platformMenu = nativeMenu.createMenu();

    this.actions = {};

    this.addAction('cut', tr('ebisu_client_menu_cut'), function ()
    {
        document.execCommand('cut');
    });

    this.addAction('copy', tr('ebisu_client_menu_copy'), function ()
    {
        document.execCommand('copy');
    });

    this.addAction('paste', tr('ebisu_client_menu_paste'), function ()
    {
        document.execCommand('paste');
    });
};

CutCopyPasteContextMenu.prototype.addAction = function (id, label, callback)
{
    var action = this.platformMenu.addAction(label);

    action.triggered.connect($.proxy(callback, this));

    this.actions[id] = action;
};

CutCopyPasteContextMenu.prototype.updateActions = function ()
{
    this.actions.cut.visible = true;
    this.actions.cut.enabled = document.queryCommandEnabled('cut');

    this.actions.copy.visible = true;
    this.actions.copy.enabled = document.queryCommandEnabled('copy');

    this.actions.paste.visible = true;
    this.actions.paste.enabled = document.queryCommandEnabled('paste');
};

//
// Singleton modeled after hovercard manager
// 
CutCopyPasteContextMenuManager = function()
{
    var createCutCopyPasteMenu, that = this;

    this.cutCopyPasteMenu = null;
    this.hideCallback = null;

    createCutCopyPasteMenu = function()
    {
        var menu = new CutCopyPasteContextMenu();
        menu.platformMenu.addObjectName("cutCopyPasteContextMenu");
        menu.platformMenu.aboutToHide.connect($.proxy(function()
        {
            if (that.hideCallback)
            {
                that.hideCallback();
            }

        }, that));

        return menu;
    };

    this.popup = function (position)
    {
        // Mac OIG depends on us creating a new context menu each time
        if (this.cutCopyPasteMenu !== null)
        {
            // hide any existing context menus so we only display one at a time
            this.cutCopyPasteMenu.platformMenu.hide();
            this.cutCopyPasteMenu = null;
        }
        // Lazily build our context menu
        this.cutCopyPasteMenu = createCutCopyPasteMenu();

        this.cutCopyPasteMenu.updateActions();

        this.cutCopyPasteMenu.platformMenu.popup(position);

        return true;
    };
};

if (!window.Origin) { window.Origin = {}; }

Origin.CutCopyPasteContextMenuManager = new CutCopyPasteContextMenuManager();

}(Zepto));
