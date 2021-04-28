;(function($, undefined){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }
if (!Origin.model) { Origin.model = {}; }
if (!Origin.util) { Origin.util = {}; }

Origin.util.badImageRender = function()
{	
	$('img.boxart').error(function()
    {
		$(this).attr('src', Origin.model.constants.DEFAULT_IMAGE_PATH);
	});
};
Origin.util.removeFilePrefix = function(path)
{
    // Don't want to show the user the "file:" prefix, so remove it.
    if(path.length > 0 && path.indexOf('file:') === 0)
    {
        path = path.replace('file:', '');
    }
    return path;
}

Origin.util.addFilePrefix = function(path)
{
    // Re-add the "file:" prefix for non-empty paths, so the override actually works, unless it starts with:
    if (path.length > 0 &&
        path.indexOf('file:') === -1 && // an existing "file:" prefix.
        path.indexOf('http:') === -1 && // an "http:" web prefix
        path.indexOf('https:') === -1 && // an "https:" web prefix
        path.indexOf('ftp:') === -1 ) //an "ftp:" web prefix
    {
        path = 'file:' + path;
    }
    return path;
}

// TODO: Convert general overrides to this format.
Origin.util.readOverride = function(section, override, offerId, element)
{
    // Pseudo-overload
    if(typeof element === 'undefined')
    {
        element = offerId;
        offerId = '';
    }

    if(section === undefined || override === undefined || element === undefined)
    {
        console.error('Origin.util.writeOverride called with invalid parameters: ' + section + ', ' + override + ', ' + element);
        return;
    }

    var value = '';
    if(offerId.length > 0)
    {
        value = window.originCommon.readOverride(section, override, offerId);
    }
    else
    {
        value = window.originCommon.readOverride(section, override);
    }

    if(element.attr('requires-file-prefix') === 'true')
    {
        value = Origin.util.removeFilePrefix(value);
    }

    if(element.is('input[type="text"]'))
    {
        element.val(value);
    }
    else if(element.is('input[type="checkbox"]'))
    {
        element.prop('checked', (value === 'true' || value === '1'));
    }
    else if(element.is('div.origin-ux-control.origin-ux-drop-down-control'))
    {
        if(element.find(' select > option[value="' + value + '"]').length > 0)
        {
            element.find(' select').val(value);
            element.find('.origin-ux-drop-down-selection span').text(element.find('select > option:selected').text());
        }
    }
    else if(element.is('section.software-builds'))
    {
        // No op.  This is handled by the logic that builds the software table.
    }
    else
    {
        console.error('Origin.util.readOverride called with invalid element: ' + element);
    }
}

// TODO: Convert general overrides to this format.
Origin.util.writeOverride = function(section, override, offerId, element)
{
    // Pseudo-overload
    if(typeof element === 'undefined')
    {
        element = offerId;
        offerId = '';
    }

    if(section === undefined || override === undefined || element === undefined)
    {
        console.error('Origin.util.writeOverride called with invalid parameters: ' + section + ', ' + override + ', ' + element);
        return;
    }

    var value = '';
    if(element.prop('disabled') || element.hasClass('disabled')) 
    {
        value = '';
    }
    else if(element.is('input[type="text"]'))
    {
        value = element.val();
    }
    else if(element.is('input[type="checkbox"]'))
    {
        value = element.prop('checked') ? 'true' : '';
    }
    else if(element.is('div.origin-ux-control.origin-ux-drop-down-control'))
    {
        value = element.find('select').val();
    }
    else if(element.is('section#software-builds'))
    {
        var checked = element.find('.build-override-checkbox:checked');
        if(checked.length > 0) 
        {
            value = checked.attr('build-id');
        }
    }
    else
    {
        console.error('Origin.util.writeOverride called with invalid element: ' + element.attr('id'));
        return;
    }

    if(element.attr('requires-file-prefix') === 'true')
    {
        value = Origin.util.addFilePrefix(value);
    }

    if(offerId.length > 0)
    {
        window.originCommon.writeOverride(section, override, offerId, value);
    }
    else
    {
        window.originCommon.writeOverride(section, override, value);
    }
}

Origin.util.resizeFixedDivs = function()
{
    // Fix div overlapping scrollbar since position:fixed doesn't play nicely with overflow:auto
    var $main = $("#main");
    $("#navbar").removeClass("scrollbarVisible");
    $main.find("footer").each(function()
    {
        $(this).removeClass("scrollbarVisible");
    });
    
    if($main && $main[0].scrollHeight > $main.innerHeight())
    {
        $("#navbar").addClass("scrollbarVisible");
        $main.find("footer").each(function()
        {
            $(this).addClass("scrollbarVisible");
        });
    }
}

$(window.document).ready(function() 
{
    if(window.originDeveloper) 
    {
        window.originCommon.reloadConfig();
        Origin.views.myGamesOverrides.init();
        Origin.views.generalOverrides.init();
        Origin.views.navbar.currentTab = "general";
        window.document.title = "Origin Developer Tool Version " + window.originDeveloper.odtVersion;
    }
});

})(jQuery);