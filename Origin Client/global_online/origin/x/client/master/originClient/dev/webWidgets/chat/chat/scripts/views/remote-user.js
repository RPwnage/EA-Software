(function($){
"use strict";

// Updates everything but the user's avatar
function updateRemoteUser($user, remoteUser)
{
    var availability, realName, $name, fullName, presenceClass, $subtitle,
        subtitleText, subscriptionState, statusText, playingGame;

    // These are used multiple times
    // Cache their values to avoid repeated calls to the JavaScript bridge
    availability = remoteUser.availability;
    statusText = remoteUser.statusText;
    playingGame = remoteUser.playingGame;
    realName = remoteUser.realName;
    subscriptionState = remoteUser.subscriptionState;

    // Build our name
    $name = $user.children('div.name');
    $name.children('span.nickname').text(remoteUser.nickname); 
    $name.children('span.real-name').remove();

    if (realName)
    {
        fullName = realName.firstName + ' ' + realName.lastName;
        $name.append($('<span class="real-name">').text(fullName));
    }

    // Crunch our presence information down to a single CSS class name
    if (availability === null)
    {
        presenceClass = 'unknown';
    }
    else if (playingGame)
    {
        presenceClass = playingGame.joinable ? 'joinable' : 'ingame';
        if (remoteUser.playingGame.broadcastUrl)
        {
            presenceClass += ' broadcasting';
        }
        else if ($user.hasClass('broadcasting'))
        {
            // Manually remove this here since we will already have joinable / ingame and not remove it below
            $user.removeClass('broadcasting');
        }
    }
    else
    {
        presenceClass = availability.toLowerCase();
    }

    if (!$user.hasClass(presenceClass))
    {
        $user.removeClass('away ingame joinable online unavailable unknown broadcasting');
        $user.addClass(presenceClass);
    }

    $subtitle = $user.children('span.subtitle');
    $subtitle.removeClass('status presence request-controls blocked friend-invite');

    if (remoteUser.blocked)
    {
        $subtitle.text(tr('ebisu_client_blocked_uppercase')).addClass('blocked');
    }
    else if (subscriptionState !== undefined && subscriptionState.direction === 'NONE')
    {
        $subtitle.empty();

        if (subscriptionState.pendingContactApproval === true)
        {
            // This is an outgoing subscription request. Display controls for canceling it
            $subtitle.append($('<a href="#" class="revoke">').text(tr('ebisu_client_revoke_friend_request')));
        }
        else
        {
            // This is an incoming subscription request. Display controls for accepting or rejecting it
            $subtitle.append($('<a href="#" class="accept">').text(tr('ebisu_client_action_accept_request')));
            $subtitle.append($('<a href="#" class="reject">').text(tr('ebisu_client_action_ignore_request')));
            $subtitle.addClass("friend-invite");
        }
            
        $subtitle.addClass('request-controls');
    }
    else
    {
        subtitleText = "";

        if (statusText)
        {
            subtitleText = statusText;
            $subtitle.addClass('status');
        }
        else 
        {
            if (availability === "ONLINE")
            {
                subtitleText = tr('ebisu_client_presence_online');
            }
            else if (availability === "AWAY")
            {
                subtitleText = tr('ebisu_client_presence_away');
            }
            else if (availability === "UNAVAILABLE")
            {
                subtitleText = tr('ebisu_client_offline');
            }
            
            $subtitle.addClass('presence');
        }
    
        $subtitle.text(subtitleText);
    }
}

// Updates a roster user's avatar
function updateRemoteUserAvatar($user, remoteUser)
{
    $user.children('img.avatar').attr('src', remoteUser.avatarUrl);
}

// Creates a new user
function $createRemoteUser(remoteUser, buttonClass)
{
    // Build our content skeleton
    var $user = $(
            '<li>' + 
                '<img class="avatar">' + 
                '<img class="voip">' +
                '<div class="name"><span class="nickname"></span></div>' +
                '<span class="subtitle"></span>' +
            '</li>');

    // Add on our parameters in an HTML-safe way
    $user.attr('data-user-id', remoteUser.id);
    $user.append($('<button>').addClass(buttonClass));

    // Update our content
    updateRemoteUserAvatar($user, remoteUser);
    updateRemoteUser($user, remoteUser);
    
    $user.children('img.voip').attr('style', 'display:none');
    
    return $user;
}

function idForRemoteUserElement(contactElement)
{
    return $(contactElement).attr('data-user-id');
}

function remoteUserForElement(contactElement)
{
    var id = idForRemoteUserElement(contactElement);

    if (id)
    {
        return originSocial.getUserById(id);
    }

    return null;
}

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.$createRemoteUser = $createRemoteUser;
window.Origin.views.updateRemoteUser = updateRemoteUser;
window.Origin.views.updateRemoteUserAvatar = updateRemoteUserAvatar;
window.Origin.views.idForRemoteUserElement = idForRemoteUserElement;
window.Origin.views.remoteUserForElement = remoteUserForElement;


}(jQuery));
