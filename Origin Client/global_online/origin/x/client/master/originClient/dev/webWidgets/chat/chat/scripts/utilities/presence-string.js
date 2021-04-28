;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.utilities) { Origin.utilities = {}; }


// From a remote user object, distill a single string representing the user's current presence state
Origin.utilities.presenceStringForUser = function(remoteUser)
{
    var availability, playingGame, presenceString;

    availability = remoteUser.availability;
    playingGame = remoteUser.playingGame;
    
    presenceString = "unknown";
    if (playingGame)
    {
        if (playingGame.broadcastUrl)
        {
            presenceString = "broadcasting";
        }
        else if (playingGame.joinable)
        {
            presenceString = "joinable";
        }
        else
        {
            presenceString = "ingame";
        }
    }
    else if (!!availability)
    {
        presenceString = availability.toLowerCase();
    }

    return presenceString;
};

}(jQuery));