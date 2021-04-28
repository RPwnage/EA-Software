(function($){
"use strict";

var searchGroups;

searchGroups = function(searchText)
{
    if (searchText === '')
    {
        $('section#group-content').removeClass('filtering');
        $('li.chat-group').removeClass('filtered-out');
        $('li.chat-channel').removeClass('filtered-out');
    }
    else
    {
        $('section#group-content').addClass('filtering');
        $('li.chat-group').each(function(idx, group)
        {
            var $group, $channels, groupName, matched;

            $group = $(group);
            groupName = $group.find('h3.group-name').text();

            matched = groupName.toLowerCase().indexOf(searchText) !== -1;
            $group.toggleClass('filtered-out', !matched);
            
            $channels = $group.find('ol.chat-channels');
            $channels.find('li.chat-channel').toggleClass('filtered-out', !matched);
        });
    }
};

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.searchGroups = searchGroups;

}(jQuery));
