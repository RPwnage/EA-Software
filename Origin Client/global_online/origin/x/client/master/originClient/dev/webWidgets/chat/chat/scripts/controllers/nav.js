(function($){
"use strict";

var $tabs,
    $groupTab = null,
    $callout = null,
    loadTimeoutTimer = null, updateAddFriendButtonState, updateCreateGroupButtonState;

function showRosterContent()
{    
    $('#roster-content').show();
    $('#group-content').hide();
    $('#status-bar button.add-friend').show();
    $('#status-bar button.create-group').hide();
    $('#nav-buttons h1.friends').addClass('selected');
    $('#nav-buttons h1.rooms').removeClass('selected');
    $('#search-groups').hide();
    $('#search-friends').show();
}

function showGroupContent()
{    
    $('#roster-content').hide();
    $('#group-content').show();
    $('#status-bar button.add-friend').hide();
    $('#status-bar button.create-group').show();
    $('#nav-buttons h1.rooms').addClass('selected');
    $('#nav-buttons h1.friends').removeClass('selected'); 
    $('#search-groups').show();
    $('#search-friends').hide();
}

function updateFriendTabCountBadge(count)
{
    var $tab;
    if ($tabs !== undefined)
    {
        $tab = $tabs.otkTabs("tabByIndex", 0);
        $tabs.otkTabs("setNotificationCount", $tab, count);
    }
}

function updateGroupTabCountBadge(count)
{
    var $tab;
    if ($tabs !== undefined)
    {
        $tab = $tabs.otkTabs("tabByIndex", 1);
        $tabs.otkTabs("setNotificationCount", $tab, count);
    }
}

function updateMessaging()
{
    var friendDisplayMode, groupDisplayMode;

    if (loadTimeoutTimer)
    {
        window.clearTimeout(loadTimeoutTimer);
        loadTimeoutTimer = null;
    }

    if (originSocial.connection.established && originSocial.roster.hasLoaded)
    {
        if (originSocial.roster.contacts.length === 0)
        {
            friendDisplayMode = 'add-friends';
        }
        else
        {
            friendDisplayMode = 'contacts';
        }
        
        if (originSocial.groupList.groups.length === 0)
        {
            if (originSocial.groupListLoadSuccess) 
            {
                groupDisplayMode = 'create-groups';
            }
            else
            {
                groupDisplayMode = 'timeout'; // This occurs only if the group list load has failed
            }
        }
        else
        {
            groupDisplayMode = 'groups';
        }

        onUpdateSettings("Callout_GroupsTabShown", clientSettings.readBoolSetting("Callout_GroupsTabShown")); // We may need to show the callouts now that we're online
    }
    else
    {
        if (onlineStatus.onlineState)
        {
            friendDisplayMode = 'loading';
            groupDisplayMode = 'loading';

            // Start a timeout
            loadTimeoutTimer = window.setTimeout(function()
            {
                loadTimeoutTimer = null;

                // Display timeout message for friends tab
                $('#roster-content')
                    .attr('data-display', 'timeout')
                    // QtWebKit 2.2 sucks
                    .addClass('forcerecalc')
                    .removeClass('forcerecalc');

                // Display timeout message for chat group tab
                $('#group-content')
                    .attr('data-display', 'timeout')
                    // QtWebKit 2.2 sucks
                    .addClass('forcerecalc')
                    .removeClass('forcerecalc');
                    
            }, 120000); // 2 minutes
        }
        else
        {
            friendDisplayMode = 'offline';
            groupDisplayMode = 'offline';
        }
    }

    $('#roster-content')
        .attr('data-display', friendDisplayMode)
        // QtWebKit 2.2 sucks
        .addClass('forcerecalc')
        .removeClass('forcerecalc');
    $('#group-content')
        .attr('data-display', groupDisplayMode)
        // QtWebKit 2.2 sucks
        .addClass('forcerecalc')
        .removeClass('forcerecalc');
}

function groupsLoaded()
{
    return $('#group-content').attr('data-display') === 'groups' ? true : false;
}

function onUpdateSettings(name, value)
{
    // Some setting has changed - see if it's the callout settings
    if ($groupTab && originSocial.connection.established && (name ==="Callout_GroupsTabShown"))
    {
        if (value === true)
        {
            value = "true";
        }
        else if (value === false)
        {
            value = "false";
        }

        if (value === "false")
        {
            if ($callout)
            {
                // One already exists - kill it
                $callout.remove();
            }

            $callout = $.otkCalloutCreate(
            {
                newTitle: tr("ebisu_client_new_title"),
                title: tr("ebisu_client_groups_tab_title"),
                xOffset: 10,
                yOffset: 17,
                content: [{text: tr("ebisu_client_this_is_where_you_find_all_groups")}],
                bindedElement: $groupTab
            });

            $callout.on('select', function(e)
            {
                // Mark it as shown
                clientSettings.writeSetting("Callout_GroupsTabShown", "true");

                // Nuke it from the DOM (the default is to just hide)
                $callout.remove();
                $callout = null;
            });

        }
        else if (value === "true")
        {
            if ($callout)
            {
                // One already exists - kill it
                $callout.remove();
                $callout = null;
            }
        }
    }
}

$(document).ready(function()
{
    var $addFriend, $createGroup;
    
    // Handle our loading indicator so we can hide any DOM building
    updateMessaging();
    originSocial.connection.changed.connect(updateMessaging);
    originSocial.roster.loaded.connect(updateMessaging);
    onlineStatus.onlineStateChanged.connect(updateMessaging);

    $tabs = $.otkTabsCreate($('#tabs'),
    {
        squareFirstTab: true,
        closeable: false,
        draggable: false,
        short: true
    });
    
    $tabs.otkTabs("appendTab", tr('ebisu_client_friends'), "");
    $groupTab = $tabs.otkTabs("appendTab", tr('ebisu_client_chat_groups'), "");
    
    $('body').on('currentTabChanged', '.otk-tabs', function(e, $tab)
    {
        var index = $tabs.otkTabs("tabIndex", $tab);
        
        if (index === 0)
        {
            showRosterContent();
        }
        else
        {
            showGroupContent();
            Origin.views.clearGroupTabNotificationCount();
        }
    });

    $addFriend = $('#status-bar button.add-friend');
    $createGroup = $('button.create-group'); // select both "create-group" buttons
    
    // Disable the "Add a Friend" button if offline
    updateAddFriendButtonState = function()
    {
        $addFriend.toggleClass("disabled", !onlineStatus.onlineState);
    };    
    updateAddFriendButtonState();
    onlineStatus.onlineStateChanged.connect(updateAddFriendButtonState);    

    $('#status-bar').on('click', 'button.add-friend:not(.disabled)', function()
    {
        clientNavigation.showFriendSearchDialog();
    });
    
    // Disable the "Create a Group" button if offline, if groups load failed, or if user is invisible
    updateCreateGroupButtonState = function()
    {
        $createGroup.toggleClass("disabled", (!onlineStatus.onlineState || !originSocial.connection.established || !originSocial.groupListLoadSuccess || originSocial.currentUser.visibility === 'INVISIBLE'));
    };    
    updateCreateGroupButtonState();
    onlineStatus.onlineStateChanged.connect(updateCreateGroupButtonState);
    originSocial.connection.changed.connect(updateCreateGroupButtonState);  
    originSocial.currentUser.visibilityChanged.connect(updateCreateGroupButtonState);    

    $('#status-bar').on('click', 'button.create-group:not(.disabled)', function()
    {
        clientNavigation.showCreateGroupDialog();
    });
    
    // Hook up instant search
    $('#search-friends').on('input', function()
    {
        Origin.views.searchContacts($(this).val().toLowerCase());
    });
    $('#search-groups').on('input', function()
    {
        Origin.views.searchGroups($(this).val().toLowerCase());
    });
    
    $('#search-groups').attr('placeholder', tr('ebisu_client_search_groups'));    
    $('#search-friends').attr('placeholder', tr('ebisu_client_search_friends'));
    
    // Set initial count value
    updateGroupTabCountBadge(Origin.views.getGroupTabNotificationCount());

    showRosterContent();

    // We need to watch for when the callout settings values change
    clientSettings.updateSettings.connect(onUpdateSettings);

    // Set our initial callout state
    $groupTab.ready(function()
    {
        onUpdateSettings("Callout_GroupsTabShown", clientSettings.readBoolSetting("Callout_GroupsTabShown"));
    });

});

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.updateMessaging = updateMessaging;
window.Origin.views.updateFriendTabCountBadge = updateFriendTabCountBadge;
window.Origin.views.updateGroupTabCountBadge = updateGroupTabCountBadge;
window.Origin.views.groupsLoaded = groupsLoaded;

}(jQuery));
