(function($){
"use strict";

var searchContacts;

searchContacts = function(searchText)
{
    if (searchText === '')
    {
        $('section#roster-content').removeClass('filtering');
        $('li.contact').removeClass('filtered-out');
    }
    else
    {
        $('section#roster-content').addClass('filtering');
        $('li.contact').each(function(idx, contact)
        {
            var $contact, nickname, matched;

            $contact = $(contact);
            nickname = $contact.find('span.nickname').text();

            matched = nickname.toLowerCase().indexOf(searchText) !== -1;
            $contact.toggleClass('filtered-out', !matched);
        });
    }
};

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.searchContacts = searchContacts;

}(jQuery));
