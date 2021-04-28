(function($){
"use strict";

$(document).ready(function()
{
    var $currentUser, $originId, $avatar, $statusText, $typedown, 
        updateStatusText, updateAvatar, updateAvatarState;

    // Get the important elements
    $currentUser = $('header#current-user');
    $originId = $currentUser.children('h1.nickname');
    $avatar = $currentUser.children('img.avatar');
    $statusText = $currentUser.children('span.status');
    $typedown = $currentUser.children('input.typedown');

    // Set our Origin ID
    $currentUser.children('h1').text(originSocial.currentUser.nickname);

    // Set up our avatar 
    updateAvatar = function()
    {
        // If largeAvatarUrl is null this will remove the attribute
        // This is more or less what we want
        $avatar.attr('src', originSocial.currentUser.largeAvatarUrl);
    };

    // Gray out the avatar if offline
    updateAvatarState = function()
    {
        if (onlineStatus.onlineState)
        {
            $avatar.removeClass("offline");
        }
        else
        {
            $avatar.addClass("offline");
        }
    };

    originSocial.currentUser.largeAvatarChanged.connect(updateAvatar);
    originSocial.connection.changed.connect(updateAvatarState);
    updateAvatar();

    // Set up our status text
    updateStatusText = function() 
    {
        var statusText = originSocial.currentUser.statusText;

        $currentUser.toggleClass('has-status', statusText !== null);
        $statusText.text(statusText);
    };

    originSocial.currentUser.presenceChanged.connect(updateStatusText);
    updateStatusText();

    // Make the avatar and origin ID clickable
    [$originId, $avatar].forEach(function($el)
    {
        $el.on('click', function()
        {
            originSocial.currentUser.showProfile("FRIENDS_LIST");
        });
    });

    // Set up the availability dropdown
    Origin.views.initAvailabilityDropdown($currentUser);
});

}(jQuery));
