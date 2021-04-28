(function($){
"use strict";

var closeAvailabilityDropdown, initAvailabilityDropdown, $availabilityOption,
    $dropdown, translateAvailability, mouseDownOnOption = false;

translateAvailability = function(availability, inGame, broadcasting)
{
    switch(availability)
    {
    case 'ONLINE':
        if (broadcasting)
        {
            return tr('ebisu_client_broadcasting');
        }
        else if (inGame)
        {
            return tr('ebisu_client_presence_ingame');
        }

        return tr('ebisu_client_presence_online');
    case 'AWAY':
        return tr('ebisu_client_presence_away');
    }
};

// Builds an option for the given availability
$availabilityOption = function(availability, inGame, broadcasting)
{
    // Build the <option>
    return $('<li>').addClass('availability visible')
                    .attr('data-availability', availability)
                    .text(translateAvailability(availability, inGame, broadcasting));
};

closeAvailabilityDropdown = function()
{
    $('span.availability-label').removeClass('active');
    $dropdown.removeClass('open');
};

initAvailabilityDropdown = function($currentUser)
{
    var updateAvailability, $label, updateVisibility;

    $label = $currentUser.children('.availability-label');
    $dropdown = $currentUser.children('.availability-dropdown');

    updateAvailability = function()
    {
        var currentAvailability, allowedAvailabilities, invisible,
            inGame, broadcasting, addIfAllowed, $invisible;

        invisible = originSocial.currentUser.visibility === 'INVISIBLE';
        inGame = originSocial.currentUser.playingGame !== null;
        broadcasting = originSocial.currentUser.playingGame && originSocial.currentUser.playingGame.broadcastUrl !== null;
        $dropdown.toggleClass('broadcasting', broadcasting);
        $dropdown.toggleClass('ingame', inGame);

        // Update our current availability
        currentAvailability = originSocial.currentUser.availability;

        if (invisible)
        {
            $label.text(tr("ebisu_client_presence_invisible"));
        }
        else
        {
            $label.text(translateAvailability(currentAvailability, inGame, broadcasting));
        }

        // Update our availability dropdown
        allowedAvailabilities = originSocial.currentUser.allowedAvailabilityTransitions.slice(0);
        $dropdown.empty();
        
        // Add our allowed availability in sorted order
        addIfAllowed = function(name)
        {
            if (allowedAvailabilities.indexOf(name) !== -1)
            {
                var $option = $availabilityOption(name, inGame, broadcasting);

                if (!invisible && (name === currentAvailability))
                {
                    $option.addClass('selected');
                }

                $option.appendTo($dropdown);
            }
        };

        addIfAllowed("ONLINE");
        addIfAllowed("AWAY");

        // We can always go invisible
        $invisible =  $('<li>').addClass('availability invisible')
                               .toggleClass('selected', invisible)
                               .text(tr("ebisu_client_presence_invisible"));

        $invisible.appendTo($dropdown);
    };

    originSocial.currentUser.presenceChanged.connect(updateAvailability);
    originSocial.currentUser.visibilityChanged.connect(updateAvailability);
    originSocial.currentUser.allowedAvailabilityTransitionsChanged.connect(updateAvailability);
    updateAvailability();

    // Show/hide depending on if our connection is established
    updateVisibility = function()
    {
        if (originSocial.connection.established)
        {
            $label.css({'visibility': ''});
        }
        else
        {
            $label.css({'visibility': 'hidden'});
            closeAvailabilityDropdown();
        }
    };
    originSocial.connection.changed.connect(updateVisibility);
    updateVisibility();

    // Show the availability dropdown on click
    $label.on('click', function()
    {
        var labelPos = $label.position(), $options, animPx;

        $dropdown.css({
            'position': 'absolute',
            'top': (labelPos.top + 22) + 'px',
            'left': labelPos.left + 'px'
        });

        if ($dropdown.hasClass('open'))
        {
            closeAvailabilityDropdown();
        }
        else
        {
            $label.addClass('active');
            $dropdown.addClass('open');

            // Animate the open by tweaking the margin of the first child
            $options = $dropdown.children('li');
            animPx = $dropdown.height();

            $options.first().css({'margin-top': '-' + animPx + 'px'})
                      .animate({'margin-top': '0'}, 100);
        }

        return false;
    });

    // Change our availability when a availability is clicked
    $dropdown.on('click', 'li.availability', function()
    {
        var availability, $option = $(this);
        
        if ($option.hasClass('invisible'))
        {
            originSocial.currentUser.requestedVisibility = 'INVISIBLE';
        }
        else
        {
            availability = $option.attr('data-availability');

            originSocial.currentUser.requestedAvailability = availability;
            originSocial.currentUser.requestedVisibility = 'VISIBLE';
        }
            
        closeAvailabilityDropdown();
        mouseDownOnOption = false;

        return false;
    });
    
    // Track if the mouse is currently down on an option item
    // We should close the dropdown immediately on mousedown unless
    // we're pressing on a dropdown option 
    $dropdown.on('mousedown', 'li.availability', function()
    {
        mouseDownOnOption = true;
    });
    
    $('body').on('mouseup', function()
    {
        mouseDownOnOption = false;
    });

    // Close the dropdown if we press the mouse anywhere on the page
    $('body').on('mousedown contextmenu', function(e)
    {
        // if this is the availability-label, then we want to return
        if ($(e.toElement).hasClass("availability-label"))
        {
            return;
        }
        if (!mouseDownOnOption)
        {
            closeAvailabilityDropdown();
        }
    });

    // Close the dropdown when our window loses focus
    $(window).on('blur', closeAvailabilityDropdown);
};

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.initAvailabilityDropdown = initAvailabilityDropdown;
window.Origin.views.closeAvailabilityDropdown = closeAvailabilityDropdown;

}(jQuery));
