;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }

// NOTIFICATION_DISABLED = 0,
// NOTIFICATION_SOUND_ONLY = 1,
// NOTIFICATION_ALERT_ONLY = 2,
// NOTIFICATION_SOUND_ALERT = 3

var NotificationsView = function()
{
    if(Origin.views.currentPlatform() == "PC")
    {
        $("#page_title").text(tr('ebisu_client_settings'));
    }
	else
    {
        $("#page_title").text(tr('ebisu_client_preferences'));
    }
    if(originUser.socialAllowed)
    {
        // Set up friends list objects
        this.setupObject("#not-chkInviteToChatRoom", "InviteToChatRoomNotification", "Friend");
        this.setupObject("#not-chkIncomingChatRoomMessage", "IncomingChatRoomMessageNotification", "Friend");
        this.setupObject("#not-chkIncomingChat", "IncomingTextChatNotification", "Friend");
        this.setupObject("#not-chkFriendSignIn", "FriendSigningInNotification", "Friend");
        this.setupObject("#not-chkFriendSignOut", "FriendSigningOutNotification", "Friend");
        this.setupObject("#not-chkFriendStartsGame", "FriendStartsGameNotification", "Friend");
        this.setupObject("#not-chkFriendQuitsGame", "FriendQuitsGameNotification", "Friend");
        this.setupObject("#not-chkFriendStartBroadcast", "FriendStartBroadcastNotification", "Friend");
        this.setupObject("#not-chkFriendStopBroadcast", "FriendStopBroadcastNotification", "Friend");
        this.setupObject("#not-chkGameInvite", "GameInviteNotification", "Friend");

        // - Friend List Sound All -
        $("#not-chkFriendAllSoundsEnabled").on('change', function(evt)
        {
            Origin.views.notifications.toggleAllFriendListChecked(this.checked, "Sound")
        });
        $("#not-chkFriendAllSoundsEnabled").prop("checked", this.isAllItemsChecked("Sound", "Friend"));

        // - Friend List Notifications All -
        $("#not-chkFriendAllNotificationsEnabled").on('change', function(evt)
        {
            Origin.views.notifications.toggleAllFriendListChecked(this.checked, "Notification")
        });
        $("#not-chkFriendAllNotificationsEnabled").prop("checked", this.isAllItemsChecked("Notification", "Friend"));
    }
    else
    {
        $('#not-friendTable').hide();
    }

    // Set up Download objects
    this.setupObject("#not-chkDownloadFails", "DownloadErrorNotification", "Download");
    this.setupObject("#not-chkDownloadFinished", "FinishedDownloadNotification", "Download");
    this.setupObject("#not-chkInstallFinished", "FinishedInstallNotification", "Download");

    // - Download Sound All -
    $("#not-chkDownloadAllSoundsEnabled").on('change', function(evt)
    {
        Origin.views.notifications.toggleAllDownloadChecked(this.checked, "Sound")
    });
    $("#not-chkDownloadAllSoundsEnabled").prop("checked", this.isAllItemsChecked("Sound", "Download"));

    // - Download Notifications All -
    $("#not-chkDownloadAllNotificationsEnabled").on('change', function(evt)
    {
        Origin.views.notifications.toggleAllDownloadChecked(this.checked, "Notification")
    });
    $("#not-chkDownloadAllNotificationsEnabled").prop("checked", this.isAllItemsChecked("Notification", "Download"));
};


NotificationsView.prototype.toggleAllFriendListChecked = function(isAllChecked, type)
{
    // Invite to Chat Room
    var object = $("#not-chkInviteToChatRoom%1Enabled".replace("%1", type));
    if(object.prop("checked") != isAllChecked)
    {
        object.prop("checked", isAllChecked);
        clientSettings.writeSetting("InviteToChatRoomNotification", this.calculateValue("#not-chkInviteToChatRoom%1Enabled"));
    }

    // Incoming Chat Room Message
    var object = $("#not-chkIncomingChatRoomMessage%1Enabled".replace("%1", type));
    if(object.prop("checked") != isAllChecked)
    {
        object.prop("checked", isAllChecked);
        clientSettings.writeSetting("IncomingChatRoomMessageNotification", this.calculateValue("#not-chkIncomingChatRoomMessage%1Enabled"));
    }

    // Incoming Chat
    var object = $("#not-chkIncomingChat%1Enabled".replace("%1", type));
    if(object.prop("checked") != isAllChecked)
    {
        object.prop("checked", isAllChecked);
        clientSettings.writeSetting("IncomingTextChatNotification", this.calculateValue("#not-chkIncomingChat%1Enabled"));
    }

    // Friend sign in
    object = $("#not-chkFriendSignIn%1Enabled".replace("%1", type));
    if(object.prop("checked") != isAllChecked)
    {
        object.prop("checked", isAllChecked);
        clientSettings.writeSetting("FriendSigningInNotification", this.calculateValue("#not-chkFriendSignIn%1Enabled"));
    }

    // Friend sign out
    object = $("#not-chkFriendSignOut%1Enabled".replace("%1", type));
    if(object.prop("checked") != isAllChecked)
    {
        object.prop("checked", isAllChecked);
        clientSettings.writeSetting("FriendSigningOutNotification", this.calculateValue("#not-chkFriendSignOut%1Enabled"));
    }

    // Friend starts game
    object = $("#not-chkFriendStartsGame%1Enabled".replace("%1", type));
    if(object.prop("checked") != isAllChecked)
    {
        object.prop("checked", isAllChecked);
        clientSettings.writeSetting("FriendStartsGameNotification", this.calculateValue("#not-chkFriendStartsGame%1Enabled"));
    }

    // Friend Quits game
    object = $("#not-chkFriendQuitsGame%1Enabled".replace("%1", type));
    if(object.prop("checked") != isAllChecked)
    {
        object.prop("checked", isAllChecked);
        clientSettings.writeSetting("FriendQuitsGameNotification", this.calculateValue("#not-chkFriendQuitsGame%1Enabled"));
    }

    // Friend starts broadcast
    object = $("#not-chkFriendStartBroadcast%1Enabled".replace("%1", type));
    if(object.prop("checked") != isAllChecked)
    {
        object.prop("checked", isAllChecked);
        clientSettings.writeSetting("FriendStartBroadcastNotification", this.calculateValue("#not-chkFriendStartBroadcast%1Enabled"));
    }

    // Friend stops broadcast
    object = $("#not-chkFriendStopBroadcast%1Enabled".replace("%1", type));
    if(object.prop("checked") != isAllChecked)
    {
        object.prop("checked", isAllChecked);
        clientSettings.writeSetting("FriendStopBroadcastNotification", this.calculateValue("#not-chkFriendStopBroadcast%1Enabled"));
    }

    // Received game invite
    object = $("#not-chkGameInvite%1Enabled".replace("%1", type));
    if(object.prop("checked") != isAllChecked)
    {
        object.prop("checked", isAllChecked);
        clientSettings.writeSetting("GameInviteNotification", this.calculateValue("#not-chkGameInvite%1Enabled"));
    }
}


NotificationsView.prototype.toggleAllDownloadChecked = function(isAllChecked, type)
{
    // Download Error
    var object = $("#not-chkDownloadFails%1Enabled".replace("%1", type));
    if(object.prop("checked") != isAllChecked)
    {
        object.prop("checked", isAllChecked);
        clientSettings.writeSetting("DownloadErrorNotification", this.calculateValue("#not-chkDownloadFails%1Enabled"));
    }

    // Download Finished
    object = $("#not-chkDownloadFinished%1Enabled".replace("%1", type));
    if(object.prop("checked") != isAllChecked)
    {
        object.prop("checked", isAllChecked);
        clientSettings.writeSetting("FinishedDownloadNotification", this.calculateValue("#not-chkDownloadFinished%1Enabled"));
    }

    // Install Finished
    object = $("#not-chkInstallFinished%1Enabled".replace("%1", type));
    if(object.prop("checked") != isAllChecked)
    {
        object.prop("checked", isAllChecked);
        clientSettings.writeSetting("FinishedInstallNotification", this.calculateValue("#not-chkInstallFinished%1Enabled"));
    }
}

NotificationsView.prototype.isAllItemsChecked = function(type, category)
{
    var isAllChecked = false;
    if(category == "Friend")
    {
        isAllChecked = $("#not-chkInviteToChatRoom%1Enabled".replace("%1", type)).prop("checked")
                && $("#not-chkIncomingChatRoomMessage%1Enabled".replace("%1", type)).prop("checked")
                && $("#not-chkIncomingChat%1Enabled".replace("%1", type)).prop("checked")
                && $("#not-chkFriendSignIn%1Enabled".replace("%1", type)).prop("checked")
                && $("#not-chkFriendSignOut%1Enabled".replace("%1", type)).prop("checked")
                && $("#not-chkFriendStartsGame%1Enabled".replace("%1", type)).prop("checked")
                && $("#not-chkFriendQuitsGame%1Enabled".replace("%1", type)).prop("checked")
                && $("#not-chkFriendStartBroadcast%1Enabled".replace("%1", type)).prop("checked")
                && $("#not-chkFriendStopBroadcast%1Enabled".replace("%1", type)).prop("checked")
                && $("#not-chkGameInvite%1Enabled".replace("%1", type)).prop("checked");
    }
    else if(category == "Download")
    {
        isAllChecked = $("#not-chkDownloadFails%1Enabled".replace("%1", type)).prop("checked")
                && $("#not-chkDownloadFinished%1Enabled".replace("%1", type)).prop("checked")
                && $("#not-chkInstallFinished%1Enabled".replace("%1", type)).prop("checked");
    }

    return isAllChecked;
}


NotificationsView.prototype.calculateValue = function(name)
{
    var settingVal = 0;
    settingVal += $(name.replace("%1", "Sound")).prop("checked") ? 1 : 0;
    settingVal += $(name.replace("%1", "Notification")).prop("checked") ? 2 : 0;
    return settingVal;
}


NotificationsView.prototype.setupObject = function(subject, settingName, category)
{
    var settingValue = clientSettings.readSetting(settingName);

    // Setup
    var $object = $("%1SoundEnabled".replace("%1", subject));
    $object.on('change', function(evt)
    {
        var allItemsChecked = Origin.views.notifications.isAllItemsChecked("Sound", category) ? true : false;
        $("#not-chk%1AllSoundsEnabled".replace("%1", category)).prop("checked", allItemsChecked);
        clientSettings.writeSetting(settingName, Origin.views.notifications.calculateValue("%0%1Enabled".replace("%0", subject)));
    });
    $object.prop("checked", settingValue == 1 || settingValue == 3);

    $object = $("%1NotificationEnabled".replace("%1", subject));
    $object.on('change', function(evt)
    {
        var allItemsChecked = Origin.views.notifications.isAllItemsChecked("Notification", category) ? true : false;
        $("#not-chk%1AllNotificationsEnabled".replace("%1", category)).prop("checked", allItemsChecked);
        clientSettings.writeSetting(settingName, Origin.views.notifications.calculateValue("%0%1Enabled".replace("%0", subject)));
    });
    $object.prop("checked", settingValue == 2 || settingValue == 3);
}


// Expose to the world
Origin.views.notifications = new NotificationsView();

})(jQuery);
