(function($){
"use strict";

var i, allConversations, insertConversationById, raiseConversationById,
    addConversationById, initialConversationIds, initialConversationQuery, 
    $iframeForConversationId, $tabForConversationId, closeConversationById,
    addConversationNotificationById, setConversationTitle, conversationTitles,
    conversationNotifications, notifyTotalNotifications, updateConversationName,
    createTabForSingleUserConversation, createTabForGroupConversation,
    totalNotificationCount, isFocused = false, closeActiveConversation;

// Map of conversation ID => conversation object
allConversations = {};

// Map of conversation ID => title
conversationTitles = {};

// Map of unseen conversation notifications
conversationNotifications = {};

$iframeForConversationId = function(id)
{
    return $('iframe.conversation[name="' + id + '"]');
};

$tabForConversationId = function(id)
{
    return $('.otk-tabs *[name="' + id + '"]');
};

notifyTotalNotifications = function(notifyingConvId)
{
    var id,
    totalNotificationCount = 0,
    $selectedTab = $tabs.otkTabs("selectedTab"),
    selectedTabId = -1;

    if ($selectedTab && isFocused)
    {
        selectedTabId = $selectedTab.attr('name');
    }

    for(id in conversationNotifications)
    {
        if (conversationNotifications.hasOwnProperty(id) && (id !== selectedTabId))
        {
            totalNotificationCount += conversationNotifications[id];
        }
    }
    if (window.nativeChatWindow)
    {
        window.nativeChatWindow.notify(totalNotificationCount, notifyingConvId);
    }
};

createTabForSingleUserConversation = function(id, conversation)
{
    var $newTab, remoteUser, remoteUserChanged, disconnectAll, $presenceIndicator;
    
    remoteUser = conversation.participants[0];
    
    // Create a tab with the main tab label set to the user's nickname
    $newTab = $tabs.otkTabs("appendTab", remoteUser.nickname, "");

    // Associate the conversation ID with this tab so we can cross reference it later
    $newTab.attr('name', id);

    // Create a presence indicator for the tab
    $presenceIndicator = $('<div class="otk-presence-indicator inset"></div>');
    $tabs.otkTabs("setTabIcon2", $newTab, $presenceIndicator);

    remoteUserChanged = function()
    {
        $presenceIndicator.attr("data-presence", Origin.utilities.presenceStringForUser(remoteUser));
        $tabs.otkTabs("setTabTitle", $newTab, remoteUser.nickname);
    };

    // Set up our initial presence, etc
    remoteUserChanged();

    // Subscribe to the user's presence changes and other info
    remoteUser.presenceChanged.connect(remoteUserChanged);
    remoteUser.nameChanged.connect(remoteUserChanged);

    disconnectAll = function()
    {
        // Kill our signals
        remoteUser.presenceChanged.disconnect(remoteUserChanged);
        remoteUser.nameChanged.disconnect(remoteUserChanged);
    }

    $newTab.data("disconnectAll", disconnectAll);
    
    return $newTab;
};

createTabForGroupConversation = function(id, conversation)
{
    var $newTab;
    
    // Create a tab with the main tab label set to the user's nickname
    $newTab = $tabs.otkTabs("appendTab", conversation.groupName, "");

    // Associate the conversation ID with this tab so we can cross reference it later
    $newTab.attr('name', id);

    return $newTab;
};

addConversationById = function(id, initial)
{
    var conversation, conversationUrl, $iframe, $tab, groupRingAnimationTimer;

    // Observe the conversation and keep track of it
    conversation = originChat.getConversationById(id);
    allConversations[id] = conversation;

    // Create a tab for it
    if (conversation.type === "ONE_TO_ONE")
    {
        // This is a 1:1 conversation
        $tab = createTabForSingleUserConversation(id, conversation);
        conversationUrl = 'conversation.html?id=' + id;
    }
    else
    {
        // This is a group conversation
        $tab = createTabForGroupConversation(id, conversation);
        conversationUrl = 'group-conversation.html?id=' + id;
    }

    // Listen for voip events so we can update the tab icon for it
    conversation.eventAdded.connect(function(conversationEvent)
    {
        var i = 0, ringAnimation;

        clearInterval(groupRingAnimationTimer);

        if ((conversationEvent.type === 'VOICE_REMOTE_CALLING')
            || (conversationEvent.type === 'VOICE_LOCAL_CALLING'))
        {
            // Show ringer icon
            if (conversation.voiceCallState === "VOICECALL_INCOMING")
            {
                $tabs.otkTabs("setTabIcon1", $tab, $("<img src='chat/images/tabs/calling0.png'></img>"));
            }
            else
            {
                $tabs.otkTabs("setTabIcon1", $tab, $("<img src='chat/images/tabs/callingout0.png'></img>"));
            }

            // Animate it
            i=1;
            ringAnimation = function()
            {
                if (conversation.voiceCallState === "VOICECALL_INCOMING")
                {
                    var image = 'chat/images/tabs/calling' + i + '.png';
                    $tabs.otkTabs("setTabIcon1", $tab, $("<img src=" + image + "></img>"));
                    if (++i > 2) { i = 0; }
                    window.setTimeout(ringAnimation, 300);
                }
                else if (conversation.voiceCallState === "VOICECALL_OUTGOING")
                {
                    var image = 'chat/images/tabs/callingout' + i + '.png';
                    $tabs.otkTabs("setTabIcon1", $tab, $("<img src=" + image + "></img>"));
                    if (++i > 2) { i = 0; }
                    window.setTimeout(ringAnimation, 300);
                }
            };
            window.setTimeout(ringAnimation, 500);
        }
        else if (conversationEvent.type === 'VOICE_GROUP_CALL_CONNECTING')
        {
            // Show ringer icon
            $tabs.otkTabs("setTabIcon1", $tab, $("<img src='chat/images/tabs/callingout0.png'></img>"));

            // Group chat voip connecting state
            groupRingAnimationTimer = window.setInterval(function () {
                var image = 'chat/images/tabs/callingout' + i + '.png';
                $tabs.otkTabs("setTabIcon1", $tab, $("<img src=" + image + "></img>"));
                if (++i > 2) { i = 0; }
            }, 300);
        }
        else if (conversationEvent.type === 'VOICE_STARTED')
        {
            // Add voip icon
            $tabs.otkTabs("setTabIcon1", $tab, $("<img src='chat/images/tabs/in-voice.png'></img>"));
        }
        else if (conversationEvent.type === 'VOICE_START_TALKING')
        {
            if (conversationEvent.from.id == originSocial.currentUser.id)
            {
                $tabs.otkTabs("setTabIcon1", $tab, $("<img src='chat/images/tabs/in-voice.png'></img>"));
            }
        }
        else if (conversationEvent.type === 'VOICE_STOP_TALKING')
        {
            if (conversationEvent.from.id == originSocial.currentUser.id)
            {
                $tabs.otkTabs("setTabIcon1", $tab, $("<img src='chat/images/tabs/in-voice.png'></img>"));
            }
        }
        else if (conversationEvent.type === 'VOICE_ENDED')
        {
            // Remove voip icon
            $tabs.otkTabs("clearTabIcon1", $tab);
        }
    }.bind(this));

    // Create an iframe for it
    $iframe = $('<iframe class="conversation">'); 
    $iframe.attr('name', id);
    $iframe.attr('src', conversationUrl);

    $iframe.appendTo($('#conversations'));
};

insertConversationById = function(id)
{
    if (!allConversations.hasOwnProperty(id))
    {
        // Create a conversation entry first
        addConversationById(id, false);
    }
};

raiseConversationById = function(id)
{
    var $iframe, $tab;

    insertConversationById(id);

    $iframe = $iframeForConversationId(id);
    $tab = $tabForConversationId(id);

    // Select our tab
    $tabs.otkTabs("selectTab", $tab);

    // Hide all other iframes
    $('iframe.conversation').removeClass('active');
    // Show our iframe
    $iframe.addClass('active');

    // And focus it
    $iframe.focus();

    // Let it know it was raised
    // Have to do this on a timer or the postMessage fails since the iframe hasn't loaded yet
    // TODO - Implement a better fix for 9.4!
    // From Ryan:
    // QtWebKit 2.2 has an issue where an element from an unfocused iframe
    // can steal focus from the focused iframe
    window.setTimeout(function()
    {
        if ($iframe[0].contentWindow && $iframe[0].contentWindow.postMessage)
        {
            $iframe[0].contentWindow.postMessage({'type': 'conversationRaised'}, '*');
        }
    }, 100);

    // Remove all notifications
    delete conversationNotifications[id];
    notifyTotalNotifications(null);
    
    // Update our title 
    document.title = conversationTitles[id] || '';
};

setConversationTitle = function(id, title)
{
    conversationTitles[id] = title;

    if ($iframeForConversationId(id).hasClass('active'))
    {
        document.title = title;
    }
};

updateConversationName = function(id, name)
{
    var $tab = $tabForConversationId(id);
    if( $tab.length )
    {
        $tabs.otkTabs("setTabTitle", $tab, name);       
        
        // The following forces a resize of the tab width
        $tab.find(".title").width("0");
        $tab.find(".title").width("");
    }
}

closeConversationById = function(id)
{
    var $iframe, newId, $tab, conversation;

    conversation = allConversations[id];
    conversation.leaveVoice();

    delete conversationNotifications[id];

    // Forget the conversation
    delete allConversations[id];

    // Remove the UI
    $iframe = $iframeForConversationId(id);

    $iframe.remove();

    // Close the tab for this conversation
    $tab = $tabForConversationId(id);
    if( $tab.length )
    {
        // Disconnect this tab's signals
        if ($tab.data("disconnectAll") !== undefined)
        {
            $tab.data("disconnectAll")();
        }

        // Close the tab
        $tabs.otkTabs("closeTab", $tab);
    }

    // If there are no tabs left, close ourselves
    if ($tabs.otkTabs("tabCount") === 0)
    {
        // Close on a short timer to avoid race condition that causes a crash after closing the last tab
        setTimeout(function() { 
            window.close();
        }, 1);                
    }

    notifyTotalNotifications(null);
};
   
closeActiveConversation = function()
{
    var $activeFrame = $('iframe.conversation.active');

    if ($activeFrame.length !== 0)
    {
        closeConversationById($activeFrame.attr('name'));
        return false;
    }
};

addConversationNotificationById = function(id)
{
    var currentCount = 0, $tab;

    if (isNaN(conversationNotifications[id]))
    {
        conversationNotifications[id] = 1;
    }
    else
    {
        conversationNotifications[id] += 1;
    }

    // Notify the appropriate tab
    $tab = $tabForConversationId(id);
    $tabs.otkTabs("notifyTab", $tab);

    // Let our native window notify the user something happened
    notifyTotalNotifications(id);
};

// Handle our layout (responding to resize)
Origin.views.initChatLayout();

// Create and add the tabs carousel component
var $carousel = $.otkTabsCarouselCreate('<section>', {});

// Create a tab component and add it to the carousel component
var $tabs = $.otkTabsCreate('<div>',
  {
    squareFirstTab: false,
    closeable: true,
    draggable: true,
    short: false
  });

// Add an otk-tabs component to the carousel
$carousel.otkTabsCarousel("setTabs", $tabs);
$('nav#tab-area').append($carousel);
$carousel.find(".overflow").attr('title', tr("ebisu_client_additional_tabs"));

// See if they specified any initialConversations
initialConversationQuery = $.getQueryString("initialConversations");
if (initialConversationQuery !== "")
{
    initialConversationIds = initialConversationQuery.split(',');

    for(i = 0; i < initialConversationIds.length; i++)
    {
        // Add all the conversations but don't create their frame
        addConversationById(initialConversationIds[i], true);
    }

    // raise the first conversation
    raiseConversationById(initialConversationIds[0]);
}

// Listen for messages to expose conversations and update our notification
// bubble
$(window).on('message', function(ev)
{
    ev = ev.originalEvent;
    switch(ev.data.type)
    {
    case 'insertConversation':
        insertConversationById(ev.data.conversationId);
        break;
    case 'raiseConversation':
        raiseConversationById(ev.data.conversationId);
        break;
    case 'closeConversation':
        closeConversationById(ev.data.conversationId);
        break;
    case 'conversationNotification':
        addConversationNotificationById(ev.data.conversationId);
        break;
    case 'setConversationTitle':
        setConversationTitle(ev.data.conversationId, ev.data.title);
        break;
    case 'updateConversationName':
        updateConversationName(ev.data.conversationId, ev.data.name);
        break;
    case 'conversationFocused':
        // Hack - tell the tabs carousel to close its overflow menu
        // Since it's in a different iframe it doesn't know about clicks in the
        // conversation frame
        $carousel.otkTabsCarousel("hideOverflowMenu");
        break;
    default:
        console.warn('Received unknown message of type ' + ev.data.type);
    }
});

// Listen for tab selection changes
$tabs.on('currentTabChanged', function(e, $tab)
{
    // Change the current conversation
    raiseConversationById($tab.attr('name'));
});

// Listen for tab close events
$tabs.on('tabCloseClicked', function(e, $tab)
{
    // Close the conversation associated with this tab
    closeConversationById($tab.attr('name'));
});

if (window.nativeChatWindow)
{
    // We have a native window (ie not in a browser) hook up its signals
    window.nativeChatWindow.insertConversation.connect(insertConversationById);
    window.nativeChatWindow.raiseConversation.connect(raiseConversationById);
    window.nativeChatWindow.closeActiveConversation.connect(closeActiveConversation);

    window.nativeChatWindow.closeConversation.connect(function (id)
    {
        closeConversationById(id);
    });
    
    window.nativeChatWindow.nativeFocus.connect(function ()
    {
        var $selectedTab, id;
        isFocused = true;
        $selectedTab = $tabs.otkTabs("selectedTab");
        if ($selectedTab)
        {
            id = $selectedTab.attr('name');
            conversationNotifications[id] = 0;
        }
        notifyTotalNotifications(null);
    });

    window.nativeChatWindow.nativeBlur.connect(function()
    {
        var $selectedTab, id;
        isFocused = false;

        $selectedTab = $tabs.otkTabs("selectedTab");
        if ($selectedTab)
        {
            id = $selectedTab.attr('name');
            conversationNotifications[id] = 0;
        }
        notifyTotalNotifications(null);

        // Tell the tabs carousel to hide the overflow menu if the chat window loses focus
        $carousel.otkTabsCarousel("hideOverflowMenu");
    });

    window.nativeChatWindow.advanceActiveTab.connect(function()
    {
        $carousel.otkTabsCarousel("advanceTabFocus");
    });

    window.nativeChatWindow.rewindActiveTab.connect(function()
    {
        $carousel.otkTabsCarousel("rewindTabFocus");
    });    

    $tabs.on('tabClosed', function(e, $tab)
    {
        var conversationId = $tab.attr('name');
        window.nativeChatWindow.onTabClosed(conversationId);
    });

}

}(jQuery));
