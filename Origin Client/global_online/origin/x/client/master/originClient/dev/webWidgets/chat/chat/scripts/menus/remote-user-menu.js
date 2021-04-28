(function($){
"use strict";

var RemoteUserContextMenu, RemoteUserContextMenuManager;

// This is heavily based on game-contextmenu from My Games
RemoteUserContextMenu = function()
{
    // Create a new native context menu
    this.platformMenu = nativeMenu.createMenu();

    this.actions = {};

    this.addAction('sendMessage', tr('ebisu_client_send_message'), function()
    {
        this.rosterContacts[0].startConversation();
    });

    this.addAction('voiceChat', tr('ebisu_client_start_one_on_one_voice_chat'), function()
    {
        this.rosterContacts[0].startVoiceConversation();
    });

    this.addMenuAsAction('chatGroupsMenu', tr("ebisu_client_invite_into_chat_room")).setMinWidth(0);

    this.addAction('profile', tr('ebisu_client_action_profile'), function()
    {
        this.rosterContacts[0].showProfile("FRIENDS_LIST");
    });

    this.windowSeparator = this.addSeparator();

    this.addAction('joinGame', tr('ebisu_client_action_join'), function()
    {
        this.rosterContacts[0].joinGame();
    });

    this.addAction('watchBroadcast', tr('ebisu_client_watch_broadcast'), function()
    {
        telemetryClient.sendBroadcastJoined('UserMenu');
        clientNavigation.launchExternalBrowser(this.rosterContacts[0].playingGame.broadcastUrl);
    });

    this.addAction('inviteToGame', tr('ebisu_client_action_invite_to_game'), function()
    {
        this.rosterContacts[0].inviteToGame();
    });

    this.addAction('acceptSubscription', tr('ebisu_client_action_accept_request'), function()
    {
        this.rosterContacts[0].acceptSubscriptionRequest();
    });

    this.addAction('rejectSubscription', tr('ebisu_client_action_ignore_request'), function()
    {
        this.rosterContacts[0].rejectSubscriptionRequest();
    });

    this.subscriptionSeparator = this.addSeparator();

    this.addAction('block', tr('ebisu_client_unfriend_and_block'), function ()
    {
        this.rosterContacts[0].block();
    });

    this.addAction('unblock', tr('ebisu_client_unblock'), function()
    {
        this.rosterContacts[0].unblock();
    });

    this.addAction('remove', tr('ebisu_client_action_delete_friend'), function()
    {
        originSocial.roster.removeContact(this.rosterContacts[0]);
    });

    this.addAction('revoke', tr('ebisu_client_revoke_friend_request'), function()
    {
        this.rosterContacts[0].cancelSubscriptionRequest();
    });

    this.closeSeparator = this.addSeparator();

    this.addAction('close', tr('ebisu_client_close'), function()
    {
        this.closeCallback();
    });
};

RemoteUserContextMenu.prototype.addAction = function(id, label, callback)
{
    var action = this.platformMenu.addAction(label);

    action.triggered.connect($.proxy(callback, this));

    this.actions[id] = action;
};

RemoteUserContextMenu.prototype.addMenuAsAction = function(id, title)
{
    this[id] = this.platformMenu.addMenu(title);
    return this[id];
};

RemoteUserContextMenu.prototype.addSeparator = function()
{
    return this.platformMenu.addSeparator();
};

RemoteUserContextMenu.prototype.updateForRemoteUsers = function(contacts, chattable, closeCallback)
{
    var incomingRequest, outgoingRequest, fullySubscribed, blocked, closable,
    userJoinable, userBroadcasting, groupSelected, chatGroups, userAvailable,
    groupNameAlphaSort, conversationGroupNameSort, canInviteToRoom = false, 
    sortedConversations = [];

    userJoinable = function(user)
    {
        return !!(user.playingGame && user.playingGame.joinable);
    };

    userBroadcasting = function(user)
    {
        return !!(user.playingGame && user.playingGame.broadcastUrl);
    };

    userAvailable = contacts.some(function(contact)
    {
        return contact.availability !== 'UNAVAILABLE';
    });

    conversationGroupNameSort = function(convA, convB)
    {
        return convA.groupName.localeCompare(convB.groupName);
    }
   
    groupSelected = contacts.length > 1;

    this.rosterContacts = contacts;
    this.closeCallback = closeCallback;

    closable = closeCallback !== undefined;

    // Deal with requests versus full subscribed contacts
    incomingRequest = false;
    outgoingRequest = false;
    fullySubscribed = true;

    // can invite to rooms?
    if (!!contacts[0]) {
        canInviteToRoom = ( (contacts[0].playingGame) || ((contacts[0].availability === 'ONLINE') || (contacts[0].availability === 'AWAY')) );
    }
    
    if (!!contacts[0].subscriptionState && contacts[0].subscriptionState.direction !== 'BOTH')
    {
        // This is some sort of request
        fullySubscribed = false;

        outgoingRequest = contacts[0].subscriptionState.pendingContactApproval;
        incomingRequest = contacts[0].subscriptionState.pendingCurrentUserApproval;
    }
    
    // Create an empty object to check for submenu actions later
    this.chatGroupsMenu.actions = [];
    sortedConversations = originChat.conversations.splice(0);
    sortedConversations.sort(conversationGroupNameSort);

    if (sortedConversations.length)
    {
        this.chatGroupsMenu.clear();

        sortedConversations.forEach(function(conversation)
        {
            if (!!conversation.groupName && conversation.groupName.length > 0) 
            {
                var action = this.chatGroupsMenu.addAction(conversation.groupName);
                action.enabled = true;
                
                // add this action to our object so we can check against it later
                // Use Guid for a unique identifier
                this.chatGroupsMenu.actions[conversation.groupGuid] = action;

                action.triggered.connect(function()
                {
                    // could add check here to see if the contacts are already in the room
                    conversation.inviteUsersToRoom(contacts);
                });
            }
        }, this);
        
    }

    blocked = contacts[0].blocked;

    this.actions.profile.visible = !groupSelected;
    this.windowSeparator.visible = !groupSelected;
    
    this.actions.sendMessage.visible = !groupSelected && chattable && fullySubscribed;
    
    this.actions.voiceChat.visible = !groupSelected && chattable && fullySubscribed && (!!contacts[0].capabilities && contacts[0].capabilities.indexOf("VOIP") !== -1);
    this.actions.voiceChat.enabled = userAvailable && originSocial.currentUser.visibility === 'VISIBLE';
    
    this.chatGroupsMenu.menuAction.visible = (canInviteToRoom && fullySubscribed && (Object.keys(this.chatGroupsMenu.actions).length > 0));
    
    this.actions.remove.visible = !groupSelected && fullySubscribed;
    this.actions.revoke.visible = !groupSelected && outgoingRequest;

    this.actions.joinGame.visible = !groupSelected;
    this.actions.joinGame.enabled = userJoinable(contacts[0]);

    this.actions.watchBroadcast.visible = !groupSelected;
    this.actions.watchBroadcast.enabled = userBroadcasting(contacts[0]);

    this.actions.inviteToGame.visible = !groupSelected;
    this.actions.inviteToGame.enabled = userJoinable(originSocial.currentUser);

    this.actions.acceptSubscription.visible = !groupSelected && incomingRequest;
    this.actions.rejectSubscription.visible = !groupSelected && incomingRequest;

    this.actions.block.visible = !groupSelected && !blocked;
    this.actions.unblock.visible = !groupSelected && blocked;

    this.subscriptionSeparator.visible = !groupSelected;

    this.actions.close.visible = !groupSelected && closable;
    this.closeSeparator.visible = !groupSelected && closable;
};

//
// Singleton modelled after hovercard manager
// 
RemoteUserContextMenuManager = function()
{
    var createRemoteUserMenu, that = this;

    this.rosterContactMenu = null;
    this.showingSingleRemoteUserId = null;

    this.hideCallback = null;

    createRemoteUserMenu = function()
    {
        var menu = new RemoteUserContextMenu();
        menu.platformMenu.addObjectName("friendsContextMenu");
        menu.platformMenu.aboutToHide.connect($.proxy(function()
        {
            if (that.hideCallback)
            {
                that.hideCallback();
            }

            that.showingSingleRemoteUserId = null;
        }, that));

        return menu;
    };

    this.popup = function(contacts, chattable, position, closeCallback)
    {
        if (contacts.length > 1) return; // Nothing to show on multi-select

        // Kill the presence dropdown if we have one
        if (Origin.views.closePresenceDropdown)
        {
            Origin.views.closePresenceDropdown();
        }

        // Mac OIG depends on us creating a new context menu each time
        if (this.rosterContactMenu !== null)
        {
            // hide any existing context menus so we only display one at a time
            this.rosterContactMenu.platformMenu.hide();
            this.rosterContactMenu = null;
        }
        // Lazily build our context menu
        this.rosterContactMenu = createRemoteUserMenu();


        // Update our state
        if (contacts.length === 1)
        {
            this.showingSingleRemoteUserId = contacts[0].id;
        }
        else
        {
            this.showingSingleRemoteUserId = null;
        }

        this.rosterContactMenu.updateForRemoteUsers(contacts, chattable, closeCallback);

        this.rosterContactMenu.platformMenu.popup(position);

        return true;
    };
};

RemoteUserContextMenuManager.prototype.updateRemoteUserIfVisible = function(contact)
{
    if (this.showingSingleRemoteUserId === contact.id)
    {
        this.rosterContactMenu.updateForRemoteUsers([contact]);
    }
};

if (!window.Origin) { window.Origin = {}; }

Origin.RemoteUserContextMenuManager = new RemoteUserContextMenuManager();

} (jQuery));
