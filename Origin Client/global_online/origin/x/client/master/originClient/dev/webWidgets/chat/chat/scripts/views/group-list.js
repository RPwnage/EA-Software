(function($){
"use strict";

function $createGroups()
{
    var $section, $createGroupItem, $createTitle;

    $section = $('<section class="group-content">');

    // Contact list
    $section.append('<ol class="chat-groups">');

    return $section;
}

function selectorForAllGroupChats()
{
    return 'li.chat-group';
}

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.$createGroups = $createGroups;
window.Origin.views.selectorForAllGroupChats = selectorForAllGroupChats;

}(jQuery));
