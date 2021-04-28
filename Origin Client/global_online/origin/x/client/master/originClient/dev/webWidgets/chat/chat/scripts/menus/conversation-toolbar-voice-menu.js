(function($){
"use strict";

var ConversationToolbarVoiceMenu, ConversationToolbarVoiceMenuManager;

ConversationToolbarVoiceMenu = function()
{
    // Create a new native context menu
    this.platformMenu = nativeMenu.createMenu();

    this.actions = {};
    
    this.addAction('startVoiceChat', tr('ebisu_client_start_voice_chat'), function()
    {
        this.conversation.joinVoice();
    });
    
    this.addAction('endVoiceChat', tr('ebisu_client_end_voice_chat'), function()
    {
        this.conversation.leaveVoice();
    });
    
    this.addAction('mute', tr('ebisu_client_mute_your_microphone'), function()
    {
        this.conversation.muteSelfInVoice();
    });
    
    this.addAction('unmute', tr('Ebisu_client_unmute_your_microphone'), function()
    {
        this.conversation.unmuteSelfInVoice();
    });
    
    this.addAction('voiceSettings', tr('ebisu_client_voice_chat_settings'), function()
    {
        clientNavigation.showVoiceSettingsPage();
    });
    
    return this;
};

ConversationToolbarVoiceMenu.prototype.addAction = function(id, label, callback)
{
    var action = this.platformMenu.addAction(label);

    action.triggered.connect($.proxy(callback, this));

    this.actions[id] = action;
};

ConversationToolbarVoiceMenu.prototype.update = function(conversation)
{
    var pushToTalk = clientSettings.readBoolSetting("EnablePushToTalk");

    this.conversation = conversation;
    
    this.actions.mute.visible = false;
    this.actions.unmute.visible = false;
    
    if ((conversation.voiceCallState.indexOf("VOICECALL_INCOMING") !== -1)
        || (conversation.voiceCallState.indexOf("VOICECALL_DISCONNECTED") !== -1))
    {
        this.actions.startVoiceChat.visible = true;
        this.actions.endVoiceChat.visible = false;
    }
    else if ((conversation.voiceCallState.indexOf("VOICECALL_OUTGOING") !== -1)
        || (conversation.voiceCallState.indexOf("VOICECALL_CONNECTED") !== -1))
    {
        this.actions.startVoiceChat.visible = false;
        this.actions.endVoiceChat.visible = true;
    }
    
    if (conversation.voiceCallState.indexOf("VOICECALL_CONNECTED") !== -1)
    {
        if (conversation.voiceCallState.indexOf("VOICECALL_MUTED") !== -1)
        {
            this.actions.mute.visible = false;
            this.actions.unmute.visible = true;
        }
        else if (!pushToTalk)
        {
            this.actions.mute.visible = true;
            this.actions.unmute.visible = false;
        }
    }
};

//
// Singleton modelled after hovercard manager
// 
ConversationToolbarVoiceMenuManager = function()
{
    var createConversationToolbarVoiceMenu, that = this;

    this.ConversationToolbarVoiceMenu = null;
    this.hideCallback = null;

    createConversationToolbarVoiceMenu = function()
    {
        var menu = new ConversationToolbarVoiceMenu();
        menu.platformMenu.addObjectName("ConversationToolbarVoiceMenu");
        menu.platformMenu.aboutToHide.connect($.proxy(function()
        {
            if (that.hideCallback)
            {
                that.hideCallback();
            }
        }, that));

        return menu;
    };

    this.popup = function(conversation, position)
    {
        // Mac OIG depends on us creating a new context menu each time
        if (this.ConversationToolbarVoiceMenu !== null)
        {
            // hide any existing context menus so we only display one at a time
            this.ConversationToolbarVoiceMenu.platformMenu.hide();
            this.ConversationToolbarVoiceMenu = null;
        }

        // Lazily build our context menu
        this.ConversationToolbarVoiceMenu = createConversationToolbarVoiceMenu();
                    
        this.ConversationToolbarVoiceMenu.update(conversation);

        this.ConversationToolbarVoiceMenu.platformMenu.popup(position);

        return true;
    };
};

if (!window.Origin) { window.Origin = {}; }

Origin.ConversationToolbarVoiceMenuManager = new ConversationToolbarVoiceMenuManager();

}(jQuery));
