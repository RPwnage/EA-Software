(function($){
"use strict";

function titleForConversationTab(conversation)
{
    return tr("ebisu_client_group_chat_name_number").replace("%1", conversation.multiUserChatSequenceNumber);
}

function updateParticipantSummary($tab, conversation)
{
    $tab.find('div.count').text(conversation.participants.length + 1);
}

function updateParticipant($tab, participant)
{
    var tabSelector, $user;
    
    tabSelector = 'li.tab[data-user-id="' + participant.id + '"]';
    $user = $tab.children('ul.participants').children(tabSelector);

    Origin.views.updateRemoteUser($user, participant);
}

function updateParticipantAvatar($tab, participant)
{
    var tabSelector, $user;
    
    tabSelector = 'li.tab[data-user-id="' + participant.id + '"]';
    $user = $tab.children('ul.participants').children(tabSelector);

    $user.children('img.avatar').attr('src', participant.avatarUrl);
}

function addGroupChatParticipant($tab, conversation, participant)
{
    var $participants, $user; 
        
    $participants = $tab.children('ul.participants');

    $user = Origin.views.$createRemoteUser(participant, 'menu');
    $user.addClass('tab participant');
    $user.appendTo($participants);
    
    updateParticipantSummary($tab, conversation);
}

function removeGroupChatParticipant($tab, conversation, user)
{
    var userSelector = 'li.tab[data-user-id="' + user.id + '"]';
    $tab.children('ul.participants').children(userSelector).remove();

    updateParticipantSummary($tab, conversation);
}

function $createGroupChatTab(conversation, buttonClass)
{
    var $tab, $participants, $header, $ourTab, headline;

    // Build our content skeleton
    $tab = $(
            '<li class="tab multi-user draggable">' + 
                '<header class="title">' +
                    '<div class="pulse-overlay"></div>' +
                    '<button class="expand"></button>' +
                    '<div class="icon"></div>' +
                    '<div class="title-text"></div>' +
                    '<div class="count"></div>' +
                '</header>' +
                '<ul class="participants"></ul>' +
                '<footer class="endcap"></footer>' +
            '</li>');

    // Add on our parameters in an HTML-safe way
    $tab.attr('data-conversation-id', conversation.id);
    
    headline = titleForConversationTab(conversation);

    $header = $tab.children('header.title');
    $header.children('div.title-text').text(headline).attr("title", headline);
    $header.append($('<button>').addClass(buttonClass));
    
    // Add our initial participants
    // Don't use addGroupChatParticipant because it queries the ul.participants
    // and updates the summary every time
    $participants = $tab.children('ul.participants');

    // Add ourselves
    $ourTab = Origin.views.$createRemoteUser(originSocial.currentUser, 'menu').addClass('tab participant self');
    $ourTab.appendTo($participants);


    // Add everyone else
    conversation.participants.forEach(function(participant)
    {
        var $tab = Origin.views.$createRemoteUser(participant, 'menu');
        $tab.addClass('tab participant');
        $tab.appendTo($participants);
    });
        
    updateParticipantSummary($tab, conversation);

    return $tab;
}

function $createInactiveGroupChatTab(conversation, text)
{
    var $tab;

    // Build our content skeleton
    $tab = $(
            '<li class="tab multi-user placeholder draggable">' + 
                '<header class="title">' +
                    '<div class="pulse-overlay"></div>' +
                    '<div class="icon"></div>' +
                    '<div class="title-text"></div>' +
                    '<button class="close">' +
                '</header>' +
            '</li>');

    // Add on our parameters in an HTML-safe way
    $tab.attr('data-conversation-id', conversation.id);

    if (text === undefined)
    {
        text = titleForConversationTab(conversation);
    }

    $tab.find('div.title-text').text(text);

    return $tab;

}

window.Origin.views.updateParticipant = updateParticipant;
window.Origin.views.updateParticipantAvatar = updateParticipantAvatar;
window.Origin.views.removeGroupChatParticipant = removeGroupChatParticipant;
window.Origin.views.addGroupChatParticipant = addGroupChatParticipant;
window.Origin.views.$createGroupChatTab = $createGroupChatTab;
window.Origin.views.$createInactiveGroupChatTab = $createInactiveGroupChatTab;

}(Zepto));
