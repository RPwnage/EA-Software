(function($){
"use strict";

function toggleChatGroupClosed($group)
{
    $group.children('div.otk-expand-collapse-indicator').toggleClass('expanded');
    $group.children('header').children('hgroup').toggleClass('expanded');
    $group.find('ol.chat-channels').toggleClass('expanded');
    
    if ($group.children('div.otk-expand-collapse-indicator').hasClass('expanded'))
    {
        var group = chatGroupForElement($group);
        group.getGroupChannelInformation();
    }
}

function updateGroup($group, group)
{        
console.log("updateGroup: " + group.name);
    var $name, $status, $invitedBy;
    
    $name = $group.find('h3.group-name');
    $name.text(group.name);
    
    $status = $group.find('h6.group-status');
    
    if (group.inviteGroup)
    {
        $status.append($('<a href="#" class="accept">').text(tr('ebisu_client_action_accept_request')));
        $status.append($('<a href="#" class="reject">').text(tr('ebisu_client_action_ignore_request')));
        
        if (!!group.invitedBy && !!group.invitedBy.nickname && group.invitedBy.nickname.length > 0)
        {
            $invitedBy = $group.find('h6.group-invited-by');
            $invitedBy.text(tr('ebisu_client_groip_group_invite').replace('%1', group.invitedBy.nickname));
        }
    }
    else
    {        
        $status.empty();
    }
    
    $group.find('div.add-friend-to-group-button').toggle(!group.inviteGroup);
    // remove channels
    //$group.find('div.otk-expand-collapse-indicator').toggle(!group.inviteGroup); 

    $group.toggleClass("group-invite", group.inviteGroup);
    $group.toggleClass("invited-by", (!!group.invitedBy && group.invitedBy.nickname.length > 0));
}

function idForChatGroupElement(groupElement)
{
    return $(groupElement).attr('data-group-id');
}

function chatGroupForElement(groupElement)
{
    var id = idForChatGroupElement(groupElement);
    
    if (id)
    {
        return originSocial.getGroupById(id);
    }

    return null;
}

function $createGroup(group)
{
    // Build our content skeleton
    var $addFriendButton,
        $group = $(
            '<li>' + 
                '<div class="otk-expand-collapse-indicator" data-trigger="triggerChatGroup"></div>' +
                '<header class="group-header">' +
                    '<hgroup data-trigger="triggerChatGroup">' +
                        '<h3 class="group-name"></h3>' +
                        '<h6 class="group-invited-by"></h6>' +
                        '<h6 class="group-status"></h6>' +
                    '</hgroup>' +
                    '<div class="add-friend-to-group-button"><img src="chat/images/common/invite-to-group.png"></img></div>' +
                    '<button class="otk-hover-context-menu-button group"></button>' +
                '</header>' +
                '<ol class="chat-channels"></ol>' +
            '</li>');
            
    // remove channels
    //$toggleTriggers = $group.find('[data-trigger="triggerChatGroup"]');
    //$toggleTriggers.on('click', function()
    //{
    //    if (group && !group.inviteGroup)
    //    {
    //        toggleChatGroupClosed($group);
    //    }
    //});

    group.getGroupChannelInformation();
    
    $group.attr('data-group-id', group.id);

    $addFriendButton = $group.find('div.add-friend-to-group-button');
    $addFriendButton.attr('title', tr("ebisu_client_group_invite_friends"));

    $addFriendButton.on('click', function() {
        if (originSocial.roster.hasFriends) {
            clientNavigation.showInviteFriendsToGroupDialog(group.groupGuid);    
        } else {
            clientNavigation.showYouNeedFriendsDialog();
        }
    });

    $addFriendButton.on('mouseenter', function() {
        $addFriendButton.addClass('active');
    });

    $addFriendButton.on('mouseleave', function() {
        $addFriendButton.removeClass('active');
    });

    $addFriendButton.on('mousedown mouseup mouseleave', function(e) {
        $addFriendButton.toggleClass('pressed', e.type === 'mousedown');
    });

    // Update our content
    updateGroup($group, group);

    return $group;
}

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.toggleChatGroupClosed = toggleChatGroupClosed;
window.Origin.views.$createGroup = $createGroup;
window.Origin.views.updateGroup = updateGroup;
window.Origin.views.idForChatGroupElement = idForChatGroupElement;
window.Origin.views.chatGroupForElement = chatGroupForElement;

}(jQuery));