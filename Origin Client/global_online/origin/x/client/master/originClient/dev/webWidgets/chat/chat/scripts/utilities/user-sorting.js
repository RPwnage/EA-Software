;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.utilities) { Origin.utilities = {}; }

// From a remote user object, create a key that we can use to sort them by by presence first, then nickname
Origin.utilities.sortKeyForUser = function(remoteUser, sortByGameName)
{
    if(sortByGameName!=false){
        sortByGameName = true;
    }
    
    var presenceKey, gameTitle = "";

    // We may need a flag to control this but for now this is fine
    // Always place "us" at the very top of the list regardless
    if (remoteUser.id == originSocial.currentUser.id)
    {
        return '-9999';
    }
    if (remoteUser.playingGame)
    {
        if (remoteUser.playingGame.broadcastUrl)
        {
            presenceKey = '0';
        }
        else
        {
            gameTitle = remoteUser.playingGame.gameTitle;
            presenceKey = '1';
        }
    }
    else
    {
        switch(remoteUser.availability)
        {
            case 'CHAT':
            case 'ONLINE':
                presenceKey = '2';
                break;
            case 'DND':
                presenceKey = '3';
                break;
            case 'AWAY':
            case 'XA':
                presenceKey = '4';
                break;
            case 'UNAVAILABLE':
                presenceKey = '5';
                break;
            case null:
                presenceKey = '6';
                break;
        }
    }

    if (gameTitle && sortByGameName) // implied, default behaviour, that sortByGameName is true
    {
        return presenceKey + '-' + gameTitle + '-' + remoteUser.nickname.toLowerCase();
    }
    else
    {
        return presenceKey + '-' + remoteUser.nickname.toLowerCase();
    }
}

Origin.utilities.sortKeyForGroup = function(group)
{
    var inviteKey;
    
    if (group.inviteGroup)
    {
        inviteKey = '0';
    }
    else
    {
        inviteKey = '1';
    }

    return inviteKey + '-' + group.name;
}


}(jQuery));