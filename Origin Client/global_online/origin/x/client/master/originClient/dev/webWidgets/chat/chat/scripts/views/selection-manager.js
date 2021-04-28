(function($){
"use strict";

var KEY_LEFT  = 37,
    KEY_UP    = 38,
    KEY_RIGHT = 39,
    KEY_DOWN  = 40,
    KEY_SHIFT = 16,
    KEY_CTRL = 17,
    KEY_CMD = 91,
    delayedVisibleTimer = null,
    lastSelectedID = null,
    currentSelectedID = null,
    multiSelectKeyDown = false,
    shiftDown = false;

function selectorForSelectableElements()
{
    return Origin.views.selectorForAllContactGroups() + ' > h3, li.contact';
}

function $allSelectableElements()
{
    return $(selectorForSelectableElements());
}

function saveSelected(element)
{
    if (currentSelectedID)
    {
        lastSelectedID = currentSelectedID;
    }
    currentSelectedID = $(element).attr("data-user-id");
}

function ensureElementVerticallyVisible($element, $container, shouldDelay)
{
    var elementTop, elementBottom,
        containerTop, containerBottom;

    if (shouldDelay)
    {
        // Wait before making ourselves visible
        // This is what Qt does ??
        delayedVisibleTimer = window.setTimeout(function()
        {
            delayedVisibleTimer = null;
            ensureElementVerticallyVisible($element, $container, false);
        }, 500);
    }
    else
    {
        // Cancel any delayed visibility
        window.clearTimeout(delayedVisibleTimer);
        delayedVisibleTimer = null;

        elementTop = $element.offset().top;
        containerTop = $container.offset().top;

        // Make sure our top is visible
        if (elementTop < containerTop)
        {
            $container[0].scrollTop -= (containerTop - elementTop);
        }
        else 
        {
            elementBottom = elementTop + $element.height();
            containerBottom = containerTop +  $container.height();

            // Make sure our bottom is visible
            if (elementBottom > containerBottom)
            {
                $container[0].scrollTop += (elementBottom - containerBottom);
            }
        }
    }
}

function clearSelection(lostFocus)
{
    if (!lostFocus)
    {
        // We do not want to clear the selected elements if user is pressing CTRL/CMD or SHIFT
        if (multiSelectKeyDown || shiftDown)
        {
            // We don't want to clear the list user trying to multi select
            return;
        }
    }
    $allSelectableElements().removeClass('selected');
}

function selectElement($element, fromClick)
{
    // This will only clear all selcted item under certian condidtions
    clearSelection(false);
    ensureElementVerticallyVisible($element, $('#roster-content'), fromClick);
    $element.toggleClass('selected');
}

function selectElementsBetween($element, fromClick)
{
    var friendArray, first, second, newArray;
    // Select all elements between last and current
    if (lastSelectedID)
    {
        // get array of all friends in list
        friendArray = $('#roster-content li.contact');
        // find index of lastSelectedID and currentSelectedID
        first = friendArray.index($("[data-user-id='" + lastSelectedID + "']"));
        second = friendArray.index($("[data-user-id='" + currentSelectedID + "']"));
        // slice likes the lower number first and is non-inclusive, 
        // so we have to add one to the higher number
        if (first > second)
        {
            newArray = friendArray.slice(second, first+1);
        }
        else
        {
            newArray = friendArray.slice(first, second+1);
        }
        // loop through out left over friends and marke them as selected
         $(newArray).addClass('selected');
    }
    //if no last selected select current element and save current as last selected
    else
    {
        selectElement($element, fromClick);
        lastSelectedID = currentSelectedID;
    }
}

function $selectedElements()
{
    return $(Origin.views.selectorForAllContactGroups() + ' > h3.selected, ' + 
             'li.contact.selected'); 
}

function contactIsSelected(contact)
{ 
    var $selected, contactID;
    $selected = window.Origin.views.$selectedElements();
    contactID = contact.id;
    if ($selected.filter("[data-user-id='" + contactID + "']").length > 0)
    {
        return true;
    }
    return false;
}

function setGroupCollapsed(collapsed)
{
    var $selectedGroup = $(Origin.views.selectorForAllContactGroups() + ' > h3.selected').parent();

    if ($selectedGroup.length === 0)
    {
        // No selected group
        return false;
    }

    $selectedGroup.toggleClass('collapsed', collapsed);

    return true;
}

function moveSelection(direction)
{
    var $selected, $targetContact, $contactGroup, $nextGroup, $prevGroup, 
        $firstContact, $lastContact, $newSelected = null;
    
    // this is an array of all 'selected' elements
    $selected = $selectedElements();

    if ($selected.length === 0)
    {
        // No selection
        return false;
    }

    if ($selected.length > 1)
    {
        // multiple selection
        $selected = $selected.filter("[data-user-id='" + currentSelectedID + "']");
    }

    if ($selected.is('li.contact'))
    {
        // Find the contact in the given direction
        $targetContact = $selected[direction]();

        if (!($targetContact.is('li.contact')) && shiftDown)
        {
            // Our target is a group can't have that in multi-select
            // Need to find a contect element
            if (direction === 'prev')
            {
                // Highlight the last element of previous group
                $lastContact = $($selected.parent().parent().prev('section:not(.empty)').find('li.contact').last());
            
                if ($lastContact.length > 0)
                {
                    $targetContact = $lastContact;
                }
            }
            else if (direction === 'next')
            {
                // Highlight the first element of the next group
                $firstContact = $($selected.parent().parent().next('section:not(.empty)').find('li.contact').first());

                if ($firstContact.length > 0)
                {
                    $targetContact = $firstContact;
                }
            }
        }
        
        
        if ($targetContact.is('li.contact.selected'))
        {
            if (direction === 'prev')
            {
                $newSelected = $targetContact;
                $targetContact = $selected;
            }
            if (direction === 'next')
            {
                $newSelected = $targetContact;
                $targetContact = $selected;
            }
        }

        if ($targetContact.length > 0)
        {
            // Highlight the contact
            //Save our new 'selected' element as current selected
            if ($newSelected === null)
            {
                saveSelected($targetContact);
            }
            else
            {
                saveSelected($newSelected);
            }
            selectElement($targetContact);
            return true;
        }

        $contactGroup = $selected.parent().parent();

        if (direction === 'prev')
        {
            // Highlight our contact group
            selectElement($contactGroup.children('h3'));
            return true;
        }
        
        if (direction === 'next')
        {
            // Highlight the next contact group
            $nextGroup = $contactGroup.nextMatching('section:not(.empty)');

            if ($nextGroup.length > 0)
            {
                selectElement($nextGroup.children('h3'));
                return true;
            }
        }
    }
    else
    {
        // We're a highlighted contact group
        if (direction === 'prev')
        {
            // Highlight the last element of previous group
            $prevGroup = $selected.parent().prevMatching('section:not(.empty)');

            if ($prevGroup.length > 0)
            {
                $lastContact = $prevGroup.filter(':not(.collapsed)').find('ol > li:last-child');

                if ($lastContact.length > 0)
                {
                    selectElement($lastContact);
                    return true;
                }

                // Highlight the previous group
                selectElement($prevGroup.children('h3'));
            }
        }
        else if (direction === 'next')
        {
            // Highlight the first element of our group
            $firstContact = $selected.parent(':not(.collapsed)').find('ol > li:first-child');
            
            if ($firstContact.length > 0)
            {
                selectElement($firstContact);
                return true;
            }

            // Highlight the next group
            $nextGroup = $selected.parent().nextMatching('section:not(.empty)');

            if ($nextGroup.length > 0)
            {
                selectElement($nextGroup.children('h3'));
                return true;
            }
        }
    }

    return false;
}

// Handle our key events
$('body').on('keydown', function(evt) 
{
    if (evt.keyCode === KEY_UP)
    {
        if (moveSelection('prev'))
        {
            evt.preventDefault();
            evt.stopPropagation();
        }
    }
    else if (evt.keyCode === KEY_DOWN)
    {
        if (moveSelection('next'))
        {
            evt.preventDefault();
            evt.stopPropagation();
        }
    }
    else if (evt.keyCode === KEY_RIGHT)
    {
        if (setGroupCollapsed(false))
        {
            evt.preventDefault();
            evt.stopPropagation();
        }
    }
    else if (evt.keyCode === KEY_LEFT)
    {
        if (setGroupCollapsed(true))
        {
            evt.preventDefault();
            evt.stopPropagation();
        }
    }
    else if (evt.keyCode === KEY_SHIFT)
    {
    // PDH - disabling multiselect for now since there's no use for it
//        shiftDown = true;
    }
/*
    // PDH - disabling multiselect for now since there's no use for it
    // Hack to get around Qt swapping key codes for CMD and CTRL on Mac
    else if (evt.keyCode === KEY_CMD)
    {
        // Check to see if we are in a Browser on Mac
        if (Origin.utilities.currentPlatform() === 'MAC')
        {
            multiSelectKeyDown = true;
        }
    }
    else if (evt.keyCode === KEY_CTRL)
    {
        // Check to see if we are in Origin on Mac or anywhere on PC
        if (Origin.utilities.currentPlatform() === 'PC' || Origin.utilities.currentPlatform() === 'MAC-ORIGIN')
        {
            multiSelectKeyDown = true;
        }
    }
*/
});

$('body').on('keyup', function(evt)
{
/*
    // PDH - disabling multiselect for now since there's no use for it
    if (evt.keyCode === KEY_SHIFT)
    {
        shiftDown = false;
    }
    // Hack to get around Qt swapping key codes for CMD and CTRL
    else if (evt.keyCode === KEY_CMD)
    {
        // Check to see if we are in a Browser on Mac
        if (Origin.utilities.currentPlatform() === 'MAC')
        {
            multiSelectKeyDown = false;
        }
    }
    else if (evt.keyCode === KEY_CTRL)
    {
        // Check to see if we are in Origin on Mac or anywhere on PC
        if (Origin.utilities.currentPlatform() === 'PC' || Origin.utilities.currentPlatform() === 'MAC-ORIGIN')
        {
            multiSelectKeyDown = false;
        }
    }
*/
});

// Select on click if nothing else stops the event
$('body').on('click', 'li.contact', function()
{
    // save the current selection for later use
    saveSelected(this);
    if (shiftDown)
    {
        selectElementsBetween($(this), true);
    }
    // We do not want to set the lastselected if CTRL/CMD is down
    else if(multiSelectKeyDown)
    {
        selectElement($(this), true);
    }
    else
    {
        lastSelectedID = currentSelectedID;
        selectElement($(this), true);
    }
});

$(window).on('blur', function()
{
    // For now we ignore the blur event so we keep all selected contacts 
    // This is just until we find a good way to determine if the blur event is because of the context menu
    return false;
});

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.clearSelection = clearSelection;
window.Origin.views.selectElement = selectElement;
window.Origin.views.$selectedElements = $selectedElements;
window.Origin.views.contactIsSelected = contactIsSelected;

}(jQuery));
