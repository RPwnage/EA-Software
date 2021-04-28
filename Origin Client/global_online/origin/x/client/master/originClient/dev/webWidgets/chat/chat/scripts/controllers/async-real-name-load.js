(function($){
"use strict";

var requestedIds = [];

var requestRealName = function() {
        var $enteredContact = $(this), $prevContact, $nextContact, $contacts;

        $contacts = $([]);
        $contacts = $contacts.add($enteredContact);

        // Also request the surrounding contacts
        // C++ batches these requests so it helps us out a bit
        $prevContact = $enteredContact.prev('li.contact');
        $contacts = $contacts.add($prevContact);
        $contacts = $contacts.add($prevContact.prev('li.contact'));

        $nextContact = $enteredContact.next('li.contact');
        $contacts = $contacts.add($nextContact);
        $contacts = $contacts.add($nextContact.next('li.contact'));
        
        $contacts.each(function(index, contactElm)
        {
            var userId, contact, $contact = $(contactElm);

            userId = $contact.attr('data-user-id');

            // Avoid bugging C++ if we can
            if (requestedIds.hasOwnProperty(userId))
            {
                return;
            }

            contact = originSocial.getUserById(userId);

            if (contact)
            {
                contact.requestRealName();
            }

            requestedIds[userId] = true;
           
        });
 };
    
    
// Request user's real name when we mouse over them
// This translates to a web service request on the C++ side to fetch the 
// real name. We used to do these all up front but that was hard on the
// server and monopolized the client's HTTP requests to the Atom service
// for large rosters
$(document).ready(function()
{
    $('#roster-content').on('mouseenter', 'li.contact', requestRealName);
    $('#invite-friends-content').on('mouseenter', 'div.invite-friends-contact', requestRealName);
    $('#group-members-contact-list').on('mouseenter', 'div.user-admin-contact', requestRealName);       
});

}(jQuery));
