(function($){
"use strict";

function toggleContactGroupCollapsed($contactGroup)
{
    var groupId = $contactGroup.attr('id'),
        storageKey = 'collapsed-' + groupId;
    
    if ($contactGroup.hasClass('collapsed'))
    {
        $contactGroup.removeClass('collapsed');
        window.localStorage.removeItem(storageKey);
    }
    else
    {
        $contactGroup.addClass('collapsed');
        window.localStorage.setItem(storageKey, '1');
    }
}

function $createContactGroup(id, title)
{
    var $section, $title, $countIndicator;

    $section = $('<section class="contact-group">');
    $section.attr('id', id);

    // Title of the section
    $title = $('<h3>');
    $title.text(title);

    // Contact count indicator
    $countIndicator = $('<span class="item-count">');
    $title.append($countIndicator);

    $section.append($title);

    $title.on('click', function()
    {
        toggleContactGroupCollapsed($(this).parent());
    });

    // Pre-collapse the group if it was collapsed the last time we were shown
    if (window.localStorage.getItem('collapsed-' + id) === '1')
    {
        $section.addClass('collapsed');
    }

    // Contact list
    $section.append('<ol class="contacts">');

    return $section;
}

function updateContactGroupCounts()
{
    var $groups = $('section.contact-group');

    $groups.each(function()
    {
        var $group = $(this),
            contactCount = $group.find('ol.contacts > li.contact').length;

        $group.toggleClass('empty', contactCount === 0);
        $group.find('h3 > span.item-count').text(contactCount);
    });
}

function selectorForAllContactGroups()
{
    return 'section.contact-group';
}

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.$createContactGroup = $createContactGroup;
window.Origin.views.updateContactGroupCounts = updateContactGroupCounts;
window.Origin.views.selectorForAllContactGroups = selectorForAllContactGroups;
window.Origin.views.toggleContactGroupCollapsed = toggleContactGroupCollapsed;

}(jQuery));
