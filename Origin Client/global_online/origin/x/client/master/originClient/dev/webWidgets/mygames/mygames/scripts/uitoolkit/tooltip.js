;(function($){
"use strict";

var openTooltip = function($opener)
{
    var $tooltip = $('#tooltip').stop(true, false);
    
    $tooltip.find('> .content').html($opener.attr('data-tooltip') || '');
    
    $tooltip
        .show()
        .position(
        {
            of: $opener,
            my: 'center bottom',
            at: 'center top',
            offset: '0 -9px',
            collision: 'none none'
        })
        .hide()
        .fadeIn('fast');
};

var closeTooltip = function()
{
    $('#tooltip')
        .stop(true, false)
        .fadeOut('fast');
};

// Expose as jQuery plugin
$.fn.tooltip = function(settings)
{
    settings = $.extend(
    {
        selector: null // use this for event delegation
    }, settings);
    
    if ($('#tooltip').length === 0)
    {
        $('<span id="tooltip"><span class="content"></span><span class="pointer"></span></span>').appendTo(document.body);
    }
    
    if (settings.selector)
    {
        var $delegator = (Boolean(this.selector))? $(this.selector) : $(document.body);
        $delegator
            .on('mouseenter', settings.selector, function(evt)
            {
                openTooltip($(this));
            })
            .on('mouseleave', settings.selector, function(evt)
            {
                closeTooltip();
            });
        
        return this;
    }
    else // elements must already exist
    {
        return this.each(function(idx)
        {
            $(this)
                .on('mouseenter', function(evt)
                {
                    openTooltip($(this));
                })
                .on('mouseleave', function(evt)
                {
                    closeTooltip();
                });
        });
    }
};

})(jQuery);
