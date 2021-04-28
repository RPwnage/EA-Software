(function($){
"use strict";

var onGroupEvent, onChannelEvent, onChannelUserEvent,
    showGroupMenu, showChannelMenu, showChannelUserMenu,
    inviteCount=0;

function getGroupTabNotificationCount()
{
    return inviteCount;
}

function clearGroupTabNotificationCount()
{
    inviteCount = 0;
}

function updateInvitationCount()
{
    var numInvites, $groups;

    $groups = $('#group-content ol.chat-groups');
    numInvites = $groups.find(".chat-group.group-invite").length;
    if (numInvites != inviteCount)
    {
        inviteCount = numInvites;

        // Update the count badge on the groups tab
        Origin.views.updateGroupTabCountBadge(inviteCount);
    }
}

function addGroup(group, presortKey)
{        
    var $group, $groups, $channel, $channels, inserted, newSortKey;
              
    $group = Origin.views.$createGroup(group).addClass('chat-group');
    
    $group.attr('data-sort-key', presortKey ? presortKey : Origin.utilities.sortKeyForGroup(group));

    $groups = $('#group-content ol.chat-groups');
    
    if (presortKey)
    {
        $groups.append($group);
    }
    else
    {
        // Not pre-sorted, need to sort this element
        newSortKey = $group.attr('data-sort-key');
        inserted = false;

        // Find our correct sort position
        $.each($groups.children(), function() 
        {
            var $existingGroup = $(this);

            if (newSortKey.localeCompare($existingGroup.attr('data-sort-key')) < 0)
            {
                // We found our insertion place
                $group.insertBefore($existingGroup);
                inserted = true;
                return false;
            }
        });

        if (!inserted)
        {
            // Add to the end
            $groups.append($group);
        }
    }    
    
    // Add the channels
    if (group.channels)
    {
        $channels = $group.find('ol.chat-channels');
        group.channels.forEach(function(channel)
        {
            $channel = Origin.views.$createChannel(channel).addClass('chat-channel');
            
            $channels.append($channel);
        });
    }

    updateInvitationCount();
}

function removeGroup(group)
{
    var $groups;
    $groups = $('#group-content ol.chat-groups');
    
    $groups.find('li[data-group-id=' + group.id + ']').remove();

    updateInvitationCount();
}

function changeGroup(group)
{
    var $group, $groups, newSortKey, oldSortKey;
    
    $groups = $('#group-content ol.chat-groups');
    $group = $groups.find('li[data-group-id=' + group.id + ']');
    
    newSortKey = Origin.utilities.sortKeyForGroup(group);
    oldSortKey = $group.attr('data-sort-key');
    
    if (newSortKey.localeCompare(oldSortKey) !== 0)
    {
        removeGroup(group);
        addGroup(group);
    }
    
    Origin.views.updateGroup($group, group);
    Origin.views.updateGroupChatTab(group.id, group.name);

    updateInvitationCount();
}

function addChannel(group, channel)
{
    console.log('Adding chat channel');
    var $group, $groups, $channel, $channels;
    $groups = $('#group-content ol.chat-groups');
    
    $group = $groups.find('li[data-group-id=' + group.id + ']');
    $channels = $group.find('ol.chat-channels');
    $channel = Origin.views.$createChannel(channel).addClass('chat-channel');
    
    if (channel.locked)
    {
        $channel.addClass('locked');
    }
    
    $channels.append($channel);
}

function removeChannel(group, channel)
{
    var $group, $groups, $channels;
    $groups = $('#group-content ol.chat-groups');
    
    $group = $groups.find('li[data-group-id=' + group.id + ']');
    $channels = $group.find('ol.chat-channels');
    
    $channels.find('li[data-channel-id=' + channel.id + ']').remove();
}

function changeChannel(group, channel)
{
    var $group, $groups, $channels, $channel;
    $groups = $('#group-content ol.chat-groups');
    
    $group = $groups.find('li[data-group-id=' + group.id + ']');
    $channels = $group.find('ol.chat-channels');
    
    $channel = $channels.find('li[data-channel-id=' + channel.id + ']');
    Origin.views.updateChannel($channel, channel);
}

$(document).ready(function()
{
    var $roomContent, $body;

    // Add our groups
    $body = $('body');
    $roomContent = $('#group-content');
                  
    $roomContent.append(Origin.views.$createGroups());
                
    // Figure out our sort keys
    originSocial.groupList.groups.map(function(g)
    {
        return {key: Origin.utilities.sortKeyForGroup(g), group: g};
    })
    // Sort by the keys
    .sort(function(a, b)
    {        
        return a.key.localeCompare(b.key);
    })
    // Add the groups
    .forEach(function(g)
    {
        // Pass a hint that we're already sorted
        addGroup(g.group, g.key);
    });  

    // Listen for new groups
    originSocial.groupList.addChatGroup.connect(function(group)
    {
        addGroup(group);
        
        // Might have to get rid of the add groups callout
        window.Origin.views.updateMessaging();
    });
    
    // Listen for new channels
    originSocial.groupList.addChatChannel.connect(function(group, channel)
    {
        addChannel(group, channel);
    });
    
    // Listen for groups being deleted
    originSocial.groupList.removeChatGroup.connect(function(group)
    {
        removeGroup(group);
        
        // Might have to add the add groups callout
        window.Origin.views.updateMessaging();
    });
    
    // Listen for channels being deleted
    originSocial.groupList.removeChatChannel.connect(function(group, channel)
    {
        removeChannel(group, channel);
    });
    
    // Listen for groups being changed
    originSocial.groupList.changeChatGroup.connect(function(group)
    {
        changeGroup(group);
    });
    
    // Listen for channels being changed
    originSocial.groupList.changeChatChannel.connect(function(group, channel)
    {
        changeChannel(group, channel);
    });
    
    // Helper to add events on all groups
    onGroupEvent = function(eventName, callback)
    {
        $roomContent.on(eventName, 'li.chat-group', function(evt)
        {
            var chatGroup = Origin.views.chatGroupForElement(this);
            if (chatGroup)
            {
                // Grab the event
                evt.stopImmediatePropagation();
                evt.preventDefault();

                // Call our inner callback
                callback.call(this, chatGroup, evt);
            }
        });
    };
    
    // Helper to add events on all channels
    onChannelEvent = function(eventName, callback)
    {
        $roomContent.on(eventName, 'li.chat-channel', function(evt)
        {
            var chatChannel = Origin.views.chatChannelForElement(this);
            var chatGroup = Origin.views.chatGroupForElement($(this).parents('li.chat-group'));
            
            if (chatChannel && chatGroup)
            {
                // Grab the event
                evt.stopImmediatePropagation();
                evt.preventDefault();

                // Call our inner callback
                callback.call(this, chatChannel, chatGroup, evt);
            }
        });
    };
    
    // Helper to add events on all channel users
    onChannelUserEvent = function(eventName, callback)
    {
        $roomContent.on(eventName, 'li.channel-user', function(evt)
        {
            var channelUser = Origin.views.remoteUserForElement(this);
            var chatChannel = ($(this).parents('li.chat-channel'));
            var chatGroup = Origin.views.chatGroupForElement($(this).parents('li.chat-group'));
            
            if (channelUser)
            {
                // Grab the event
                evt.stopImmediatePropagation();
                evt.preventDefault();

                // Call our inner callback
                callback.call(this, channelUser, chatChannel, chatGroup, evt);
            }
        });
    };
    
    showGroupMenu = function (group)
    {
        var $selected;
        
        $selected = Origin.views.$selectedElements();

        Origin.ChatGroupContextMenuManager.popup(group);
        $selected.addClass('showing-menu');
    };
    
    showChannelMenu = function (channel, group)
    {
        var $selected;
        
        $selected = Origin.views.$selectedElements();

        Origin.ChatChannelContextMenuManager.popup(channel, group);
        $selected.addClass('showing-menu');
    };
    
    showChannelUserMenu = function (user, channel, group)
    {        
        var $selected, chatChannel;

        $selected = Origin.views.$selectedElements();
        chatChannel = Origin.views.chatChannelForElement(channel);
        
        Origin.ChatChannelMemberContextMenuManager.popup(user, chatChannel, group);
        $selected.addClass('showing-menu');
    };
    
    $('section#group-content').on('contextmenu', function ()
    {
        if (Origin.views.groupsLoaded())
        {
            Origin.GroupContextMenuManager.popup();
        }
        return false;
    });

    onChannelEvent('contextmenu', function (channel, group)
    {
        showChannelMenu(channel, group);
        return false;
    });
    
    onChannelEvent('dblclick', function (channel)
    {
        if (channel.locked)
        {
            clientNavigation.showEnterRoomPassword(channel.groupGuid, channel.channelId);
        }
        else
        {
            channel.joinChannel();
        }
        return false;
    });

    onGroupEvent('dblclick', function (group)
    {
        if (group.inviteGroup)
        {
            group.acceptGroupInvite();
        }
        else if (group.channels.length > 0) 
        {
            group.channels[0].joinChannel();
        }
        else
        {
            group.failedToEnterRoom();
        }
        return false;
    });
    
    onGroupEvent('contextmenu', function (group)
    {
        showGroupMenu(group);
        return false;
    });
    
    onChannelUserEvent('contextmenu', function (user, chatChannel, chatGroup)
    {
        showChannelUserMenu(user, chatChannel, chatGroup);  
        
        return false;
    });
   
    // Hook up the add groups callout button
    $('#group-content').on('click', 'button.create-group:not(.disabled)', function()
    {
        clientNavigation.showCreateGroupDialog();
    });
    
    // Hook up the go online button for the offline callout
    $('#group-content button.go-online').on('click', function()
    {
        onlineStatus.requestOnlineMode();
    });

    // Handle hover context menu button clicks
    $roomContent.on('click', '.otk-hover-context-menu-button', function(evt)
    {
        var $item = $(this), group, channel, user;

        evt.stopImmediatePropagation();
        evt.preventDefault();

        if ($item.hasClass('group'))
        {
            $item = $item.closest('li.chat-group');
            group = Origin.views.chatGroupForElement($item[0]);
            Origin.ChatGroupContextMenuManager.popup(group);
        }
        else if ($item.hasClass('channel'))
        {
            $item = $item.closest('li.chat-channel');
            channel = Origin.views.chatChannelForElement($item[0]);
            group = Origin.views.chatGroupForElement($item.closest('li.chat-group'));
            Origin.ChatChannelContextMenuManager.popup(channel, group);
        }
        else if ($item.hasClass('user'))
        {
            $item = $item.closest('li.channel-user');
            user = Origin.views.remoteUserForElement($item[0]);
            channel = Origin.views.chatChannelForElement($item.closest('chat-channel'));
            group = Origin.views.chatGroupForElement($item.closest('li.chat-group'));
            Origin.ChatChannelMemberContextMenuManager.popup(user, channel, group);
        }
        
        return false;
    });

    
    // Handle the accept/ignore links for subscription requests
    $roomContent.on('click', 'li.chat-group hgroup > h6 > a', function(evt)
    {
        var group, $requestLink;

        evt.stopImmediatePropagation();
        evt.preventDefault();

        $requestLink = $(this);
        if ($requestLink.hasClass('clicked') === false)
        {
            $requestLink.addClass('clicked'); // prevent user from clicking the link more than once

            group = Origin.views.chatGroupForElement($requestLink.closest('li.chat-group'));
        
            if ($requestLink.hasClass('accept'))
            {
                group.acceptGroupInvite();
            }
            else if ($requestLink.hasClass('reject'))
            {
                group.rejectGroupInvite();
            }
        }
    });

});

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.clearGroupTabNotificationCount = clearGroupTabNotificationCount;
window.Origin.views.getGroupTabNotificationCount = getGroupTabNotificationCount;

}(jQuery));
