/*global clientNavigation, console, document, jQuery, onlineStatus, Origin, originChat, originSocial, tr, window */

(function ($)
{
"use strict";

var onRemoteUserEvent, loadTimeoutTimer = null, showContactMenu,
    updateInvitationCount, inviteCount=0;

function getFriendTabNotificationCount()
{
    return inviteCount;
}

function clearFriendTabNotificationCount()
{
    inviteCount = 0;
}

function updateInvitationCount()
{
    var numInvites, $contacts;

    $contacts = $('li.contact');
    numInvites = $contacts.find(".friend-invite").length;
    if (numInvites != inviteCount)
    {
        inviteCount = numInvites;

        // Update the count badge on the friends tab
        Origin.views.updateFriendTabCountBadge(inviteCount);
    }
}

function $listForRemoteUser(remoteUser)
{
    var groupName, availability, subscriptionState;

    subscriptionState = remoteUser.subscriptionState;

    if (subscriptionState.direction === 'NONE')
    {
        if (subscriptionState.pendingContactApproval)
        {
            groupName = "pending-approval";
        }
        else if (subscriptionState.pendingCurrentUserApproval)
        {
            groupName = "requests";
        }
        else
        {
            // We're not subscribed or pending any actions
            // This doesn't belong in any group
            // Likely its a request we just sent out or received that's about
            // to transition to a pending state
            return null;
        }
    }
    else
    {
        availability = remoteUser.availability;

        if (remoteUser.playingGame)
        {
            groupName = "ingame";
        }
        else if ((availability === null) || (availability === "UNAVAILABLE"))
        {
            groupName = "offline";
        }
        else
        {
            groupName = "online";
        }
    }

    return $('#' + groupName + ' ol.contacts');
}

function addRemoteUserToList(remoteUser, $contact, disableSort)
{
    var $list, newSortKey, inserted;

    $list = $listForRemoteUser(remoteUser);

    if ($list === null)
    {
        // Leave it out of the DOM
        return;
    }

    if (disableSort)
    {
        // Pre-sorted
        $list.append($contact);
    }
    else
    {
        newSortKey = $contact.attr('data-sort-key');
        inserted = false;

        // Find our correct sort position
        $.each($list.children(), function() 
        {
            var $existingContact = $(this);

            if (newSortKey < $existingContact.attr('data-sort-key'))
            {
                // We found our insertion place
                $contact.insertBefore($existingContact);
                inserted = true;
                return false;
            }
        });

        if (!inserted)
        {
            // Add to the end
            $list.append($contact);
        }
    }
}

// Update an existing contact
function remoteUserChanged($contact, remoteUser, forceReinsert)
{
    var newSortKey, oldSortKey;

    // Update the contents of the contact
    Origin.views.updateRemoteUser($contact, remoteUser);

    newSortKey = Origin.utilities.sortKeyForUser(remoteUser);
    oldSortKey = $contact.attr('data-sort-key');

    if (forceReinsert || (newSortKey !== oldSortKey))
    {
        $contact.attr('data-sort-key', newSortKey);

        // Add us to the correct list
        $contact.detach();
        addRemoteUserToList(remoteUser, $contact);

        Origin.views.updateContactGroupCounts();
        updateInvitationCount();
    }
}

function addRemoteUser(remoteUser, presortKey)
{
    var $contact, avatarChangedForContact, changedForContact,
        subscriptionStateChangedForContact, disconnectAll;
        
    $contact = Origin.views.$createRemoteUser(remoteUser, 'menu').addClass('contact');

    $contact.attr('data-sort-key', presortKey || Origin.utilities.sortKeyForUser(remoteUser));

    // Add to the correct list
    // If we have a presort key then diable sort on insert
    addRemoteUserToList(remoteUser, $contact, presortKey !== undefined);

    // Build our handlers
    avatarChangedForContact = function()
    {
        Origin.views.updateRemoteUserAvatar($contact, remoteUser);
    };

    changedForContact = function()
    {
        remoteUserChanged($contact, remoteUser, false);
    };
    
    subscriptionStateChangedForContact = function()
    {
        remoteUserChanged($contact, remoteUser, true);
    };

    // Connect our signals
    remoteUser.avatarChanged.connect(avatarChangedForContact);
    remoteUser.presenceChanged.connect(changedForContact);
    remoteUser.nameChanged.connect(changedForContact);
    remoteUser.subscriptionStateChanged.connect(subscriptionStateChangedForContact);
    remoteUser.blockChanged.connect(changedForContact);

    disconnectAll = function()
    {
        // Kill our signals
        // Our signal callbacks capture $contact but the RemoteUser proxy itself
        // never actually goes away. The same RemoteUser instance can come
        // back if a new subscription request is sent. In that case we'll both
        // create and new $contact and re-insert the old zombie $contact our
        // signal callbacks have captured
        remoteUser.avatarChanged.disconnect(avatarChangedForContact);
        remoteUser.presenceChanged.disconnect(changedForContact);
        remoteUser.nameChanged.disconnect(changedForContact);
        remoteUser.subscriptionStateChanged.disconnect(subscriptionStateChangedForContact);
        remoteUser.blockChanged.disconnect(changedForContact);

        remoteUser.removedFromRoster.disconnect(disconnectAll);
    };

    remoteUser.removedFromRoster.connect(disconnectAll);
}

$(document).ready(function()
{
    var $body, $rosterContent;

    // Add our roster sections
    $body = $('body');
    $rosterContent = $('#roster-content');

    $rosterContent.append(Origin.views.$createContactGroup('requests', tr('ebisu_client_friends_list_requests')))
                  .append(Origin.views.$createContactGroup('pending-approval', tr("ebisu_client_pending_approval")))
                  .append(Origin.views.$createContactGroup('ingame', tr('ebisu_client_friends_list_in_game')))
                  .append(Origin.views.$createContactGroup('online', tr('ebisu_client_friends_list_online')))
                  .append(Origin.views.$createContactGroup('offline', tr('ebisu_client_friends_list_offline'))); 

    // Figure out our sort keys
    originSocial.roster.contacts.map(function(c)
    {
        return {key: Origin.utilities.sortKeyForUser(c), contact: c};
    })
    // Sort by the keys
    .sort(function(a, b)
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
    })
    // Add the contacts
    .forEach(function(c)
    {
        // Pass a hint that we're already sorted
        addRemoteUser(c.contact, c.key);
    });
    
    // Update our group counts
    Origin.views.updateContactGroupCounts();

    // Update the invitation count
    updateInvitationCount();

    // Listen for new contacts
    originSocial.roster.contactAdded.connect(function(remoteUser)
    {
        addRemoteUser(remoteUser);
        Origin.views.updateContactGroupCounts();
        updateInvitationCount();
        
        // Might have to get rid of the add friends callout
        window.Origin.views.updateMessaging();
    });

    // Remove old contacts from DOM
    originSocial.roster.contactRemoved.connect(function(remoteUser)
    {
        var selector, $contact;

        selector = 'li.contact[data-user-id="' + remoteUser.id + '"]';
        $contact = $(selector);

        $contact.remove();
        
        Origin.views.updateContactGroupCounts();
        updateInvitationCount();

        // Might have to re-add the add friends callout
        window.Origin.views.updateMessaging();
    });

    showContactMenu = function (contact)
    {
        var $selected, contactIds, contacts = [];
        
        $selected = Origin.views.$selectedElements();

        // Is our clicked user in the list of selected users?
        if (!Origin.views.contactIsSelected(contact))
        {
            // if not set it here so our context menu gets handled correctly
            contacts = [contact];
            $selected = $("li.contact").filter("[data-user-id='" + contact.id + "']");
        }
        else
        {
            contactIds = $selected.map(function ()
            {
                return $(this).attr("data-user-id");
            });

            if ((contactIds.length > 1) && !originChat.multiUserChatCapable)
            {
                // All options in the right click menu for multiple contacts are 
                // MUC related. If we don't support MUC then just refuse to show
                // the menu
                return;
            }

            // We use each() here and push each element into the array contacts[]
            // to ensure that we have to proper format for the startMultiUserConversation()
            contactIds.each(function (index, id)
            {
                contacts.push(originSocial.getRosterContactById(id));
            });
        }

        Origin.RemoteUserContextMenuManager.popup(contacts, true);
        $selected.addClass('showing-menu');
    };

    // Helper to add events on all roster contacts
    onRemoteUserEvent = function(eventName, callback)
    {
        $rosterContent.on(eventName, 'li.contact', function(evt)
        {
            var remoteUser = Origin.views.remoteUserForElement(this);

            if (remoteUser)
            {
                // Grab the event
                evt.stopImmediatePropagation();
                evt.preventDefault();

                // Call our inner callback
                callback.call(this, remoteUser, evt);
            }
        });
    };

    // Start a conversation on double click
    onRemoteUserEvent('dblclick', function(contact, evt)
    {
        // Don't start a conversation if they're double clicking a link
        // The link will eat the click event but it's still possible to 
        // cause a dblclick if they click twice on the same area in a short
        // period of time. An easy way to reproduce this is ignoring/revoking
        // a bunch of requests at once
        if ($(evt.toElement).is(':not(a)'))
        {
            if (contact.subscriptionState.direction === 'BOTH')
            {
                // We're a subscribed contact - start a conversation
                contact.startConversation();
            }
        }
    });
    
    onRemoteUserEvent('contextmenu', function (contact)
    {
        showContactMenu(contact);
    });

    // Show our contact menu when the menu button is clicked
    $rosterContent.on('click', 'li.contact button.menu', function ()
    {
        var contact = Origin.views.remoteUserForElement(this.parentElement);
        showContactMenu(contact);

        return false;
    });
    
    // Handle the accept/ignore links for subscription requests
    $rosterContent.on('click', 'li.contact > span.request-controls > a', function(evt)
    {
        var contact, $requestLink;

        evt.stopImmediatePropagation();
        evt.preventDefault();

        contact = Origin.views.remoteUserForElement(this.parentElement.parentElement);

        $requestLink = $(this);
        if ($requestLink.hasClass('accept'))
        {
            contact.acceptSubscriptionRequest();
        }
        else if ($requestLink.hasClass('reject'))
        {
            contact.rejectSubscriptionRequest();
        }
        else if ($requestLink.hasClass('revoke'))
        {
            contact.cancelSubscriptionRequest();
        }
    });

    // If the user presses enter while a contact is selected start a conversation
    // If a contact group is selected toggle the group
    $body.on('keydown', function(evt)
    {
        var contact, $selected;

        if (evt.keyCode === 13)
        {
            $selected = Origin.views.$selectedElements();

            if ($selected.length > 0)
            {
                contact = Origin.views.remoteUserForElement($selected.get(0)); 

                if (contact && $('#roster-content').is(":visible"))
                { 
                    if (contact.subscriptionState.direction === 'BOTH')
                    {
                        // We're a subscribed contact - start a conversation
                        contact.startConversation();
                    }
                    else
                    {
                        // Accept the subscription request
                        contact.acceptSubscriptionRequest();
                    }

                    evt.stopPropagation();
                    evt.preventDefault();
                }
                else if ($selected.is(Origin.views.selectorForAllContactGroups() + '> h3'))
                {
                    // We're a contact group
                    Origin.views.toggleContactGroupCollapsed($selected.parent());
                }
                        
            }
        }
    });

    // Whenever we close a menu make sure that we clear out the "opened" state
    // on our menu button and the showing-menu state on the contact
    Origin.RemoteUserContextMenuManager.hideCallback = function()
    {
        $('li.contact').removeClass('showing-menu');
        $('button.menu').removeClass('opened');
    };

    // Hook up the add friends callout button
    $('#roster-content  button.find-friends').on('click', function()
    {
        clientNavigation.showFriendSearchDialog();
    });
    
    // Hook up the go online button for the offline callout
    $('#roster-content button.go-online').on('click', function()
    {
        onlineStatus.requestOnlineMode();
    });
    
});

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.clearFriendTabNotificationCount = clearFriendTabNotificationCount;
window.Origin.views.getFriendTabNotificationCount = getFriendTabNotificationCount;

}(jQuery));
