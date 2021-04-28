(function($){
"use strict";

function toggleChatChannelClosed($channel)
{
    if ($channel.hasClass('locked'))
    {
        return;
    }
    if ($channel.hasClass('member'))
    {
        $channel.children('div.otk-expand-collapse-indicator').toggleClass('expanded');
        $channel.find('ol.channel-users').toggleClass('expanded');
        $channel.find('header > hgroup').toggleClass('expanded');

        if ($channel.children('div.otk-expand-collapse-indicator').hasClass('expanded'))
        {
            var channel = chatChannelForElement($channel);
            channel.getChannelPresence();
        }
    }
}

function chatChannelDoubleClick($channel)
{
    if ($channel.hasClass('locked') || originSocial.currentUser.visibility === 'INVISIBLE')
    {
        return;
    }
    if (!$channel.hasClass('member'))
    {
        $channel.addClass('member');
    }
}

function updateChannel($channel, channel)
{
    var $name, $status, $users;

    $name = $channel.find('h3.chat-name');
    $name.text(channel.name); 
    
    $users = $channel.find('ol.channel-users');
    $users.empty();

/* This section is commented out due to a design change for 9.5
   Please do not remove this code as there are plans to add this back 
   for the next release
     if (channel.channelMembers)
    {
        channel.channelMembers = channel.channelMembers.map(function(user)
        {
            return {key: Origin.utilities.sortKeyForUser(user,false), user: user};
        }).sort(function(a, b)
        {
           
            if (a.key > b.key)
            {
                return 1;
            }
    
            if (a.key < b.key)
            {
                return -1;
            }

            return 0;
        }).forEach(function(obj)
        {
            //obj PROPS: .user and .key
            var $user = $(
                    '<li class="otk-contact-mini channel-user">' +
                        '<div class="otk-presence-indicator"></div>' +
                        '<span class="nickname"></span>' +
                        '<button class="otk-hover-context-menu-button user"></button>' +                  
                    '</li>');
            
            $user.attr('data-user-id', obj.user.id);
            $user.children('div.otk-presence-indicator').attr('data-presence', Origin.utilities.presenceStringForUser(obj.user));
            $user.find('span.nickname').text(obj.user.nickname);
            $users.append($user);
        });

    } */
    
    $channel.toggleClass('member', channel.inChannel);
    $channel.toggleClass('locked', channel.locked);
    
    if (channel.locked)
    {
        $channel.children('img.locked-channel').attr('title', tr('ebisu_client_password_protected'));
    }
    else if (!$channel.hasClass('member'))
    {
        $channel.children('div.otk-expand-collapse-indicator').removeClass('expanded');
        $channel.find('ol.channel-users').removeClass('expanded');
        $channel.find('header > hgroup').removeClass('expanded');
    }
    else
    {
        //$channel.children('div.otk-expand-collapse-indicator').show();
        
        $status = $channel.find('h6.chat-status');
        $status.text(channel.status); 
    }

}

function idForChatChannelElement(channelElement)
{
    return $(channelElement).attr('data-channel-id');
}

function chatChannelForElement(channelElement)
{
    var id = idForChatChannelElement(channelElement);

    if (id)
    {
        return originSocial.getChannelById(id);
    }

    return null;
}

function $createChannel(channel)
{
/*
   The commented out code was due to a design change for 9.5
   Please do not remove this code as there are plans to add this back 
   for the next release
   */
    // Build our content skeleton
    var $channel = $(
            '<li>' +
                /* '<div class="otk-expand-collapse-indicator" data-trigger="triggerChatGroup"></div>' + */
                '<img class="locked-channel" id="locked-channel-img" src="chat/images/common/locked.png">' +
                '<header>' +
                    '<hgroup data-trigger="triggerChatGroup">' +
                        '<h3 class="chat-name"/>' +
                        '<h6 class="chat-status"/>' +
                    '</hgroup>' +
                    '<button class="otk-hover-context-menu-button channel"></button>' +                  
                '</header>' +
/*                 '<ol class="channel-users"></ol>' +
 */            '</li>'),
    $toggleTriggers = $channel.find('[data-trigger="triggerChatGroup"]');  
/*     $toggleTriggers.on('click', function()
    {
        toggleChatChannelClosed($channel);
    }); */
    
    $toggleTriggers.on('dblclick', function()
    {
        chatChannelDoubleClick($channel);
    });

    $channel.attr('data-channel-id', channel.id);
    
    // Update our content
    updateChannel($channel, channel);

    return $channel;
}

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.$createChannel = $createChannel;
window.Origin.views.updateChannel = updateChannel;
window.Origin.views.idForChatChannelElement = idForChatChannelElement;
window.Origin.views.chatChannelForElement = chatChannelForElement;

}(jQuery));
