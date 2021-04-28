;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }

var HOVERCARD_POINTER_TOP = 15;

/**
 * CLASS: HovercardManager
 */
var HovercardManager = function()
{
    this.timer = null;
    this.$currentAnchor = null;
    this.html = '';

    this.hideEvent = $.Callbacks();
    this.showEvent = $.Callbacks();
    
    // Create the hovercard element if it doesn't exist
    if ($('#hovercard').length === 0)
    {
        $('<div id="hovercard" class="hovercard" data-id=""><div class="content"></div><span class="pointer"></span></div>').appendTo(document.body);
    }
    
    // Bind to the hovercard mouseenter to reset the mouseleave timer
    $(document.body).on('hover', '.hovercard', $.proxy(function(evt)
    {
        switch (evt.type)
        {
            case ('mouseenter'):
            {
                this.clearTimer();
                break;
            }
            case ('mouseleave'):
            {
                // Figure out which element the mouse entered
                var $entered = $(evt.relatedTarget);

                // Don't hide if we mouse in to a tooltip
                if (!$entered.hasClass("tooltip") && 
                    !$entered.parent().hasClass("tooltip"))
                {
                    this.hideDelayed();
                }
                break;
            }
        }
    }, this));
    
    $(window).scroll($.proxy(function(evt)
    {
        this.updatePosition(true);
    }, this));
};

HovercardManager.prototype.clearTimer = function()
{
    if (this.timer)
    {
        window.clearTimeout(this.timer);
    }
    this.timer = null;
};

HovercardManager.prototype.isVisible = function(id)
{
    return ($('#hovercard[data-id="' + id + '"]:visible').length === 1);
};

HovercardManager.prototype.showDelayed = function($anchor, implementorView, data, id)
{
    id = id || '';
    if (this.isVisible(id))
    {
        this.clearTimer();
        return;
    }
    this.hide(true);
    this.timer = window.setTimeout($.proxy(function()
    {
        this.show($anchor, implementorView, data, id);
    }, this), 500);
};

// $anchor - the jQuery element to anchor the hovercard to
// implementorView - implements getHovercardHtml method
// data (optional) - gets passed to the implementorView
// id (optional) - gets set as the data-id attribute of the hovercard for optional use in the view
HovercardManager.prototype.show = function($anchor, implementorView, data, id)
{
    if (!implementorView || !$.isFunction(implementorView.getHovercardHtml))
    {
        console.log('HovercardManager.show:: Warning! Requires a valid hovercard implementor object - exiting.');
        return;
    }
    
    id = id || '';
    
    if (this.isVisible(id))
    {
        this.clearTimer();
        return;
    }
    
    var $hovercard = this.hide(true).attr('data-id', id);
    
    this.$currentAnchor = $anchor;
    
    // Update the HTML and set the id
    this.updateHtml(implementorView.getHovercardHtml(data));
    
    this.updatePosition();

    $hovercard.show();//fadeIn('fast');
    this.showEvent.fire(id);

    // Make sure we cut off any overflow on descriptions gracefully
    var $multiLineDescription = $('.info > .multi-line');
    if ($multiLineDescription.length > 0)
    {
        $multiLineDescription.dotdotdot();
    }
};

HovercardManager.prototype.hideDelayed = function()
{
    this.clearTimer();
    this.timer = window.setTimeout($.proxy(function()
    {
        this.hide();
    }, this), 500);
};

HovercardManager.prototype.hide = function(forceImmediate)
{
    this.clearTimer();
    
    this.$currentAnchor = null;
    
    var $hovercard = $('#hovercard').stop(true, false);

    var wasVisible = $hovercard.is(':visible');
    
    if (!forceImmediate)
    {
        $hovercard.hide();//.fadeOut('fast');
    }
    else
    {
        $hovercard.hide();
    }

    if (wasVisible)
    {
        this.hideEvent.fire();
    }

    return $hovercard;
};

HovercardManager.prototype.updateHtml = function(html)
{
    html = html || '';
    
    var $hovercard = $('#hovercard');
    if (html !== this.html)
    {
        this.html = html;
        $hovercard.find('> .content').html(html);
    }
    return $hovercard;
};

HovercardManager.prototype.updatePosition = function(onlyIfVisible)
{
    var $hovercard = $('#hovercard');
    if (!this.$currentAnchor || (onlyIfVisible && $hovercard.is(':hidden')))
    {
        return;
    }
    
    // If the hovercard is currently hidden, we need to show it for a few milliseconds to grab its dimensions, and then rehide it after
    var isHidden = $hovercard.is(':hidden');
    if (isHidden)
    {
        $hovercard.show();
    }
    
    var $anchor = this.$currentAnchor;
    var $window = $(window);
    var anchorLeft = $anchor.position().left;
    var winHeight = window.innerHeight;
    var scrollLeft = $window.scrollLeft();
    var scrollTop = $window.scrollTop();
    var offsetTop = $('body > header').outerHeight();
    var outerWidth = $hovercard.outerWidth();
    var outerHeight = $hovercard.outerHeight();
    var pointerHeight = $hovercard.find('> .pointer').outerHeight() + 10;
    
    $hovercard.position(
    {
        of: $anchor,
        my: 'right top',
        at: 'right top',
        offset: outerWidth + ' 20',
        using: function(pos)
        {
            var $hovercard = $(this);
            var $pointer = $hovercard.find('> .pointer');
            
            // Reset the hovercard
            $hovercard.removeClass('hflip');
            
            // Reset the pointer's position
            $pointer.css('top', (HOVERCARD_POINTER_TOP + 'px'));
            
            // Check for a horizontal flip
            if (pos.left < anchorLeft)
            {
                $hovercard.addClass('hflip');
            }
            
            // Make sure the hovercard isn't off of the left side of the screen
            if (pos.left < scrollLeft)
            {
                pos.left = scrollLeft;
            }
            
            // Check to see if vertical respositioning is needed
            var bottom = pos.top + outerHeight;
            if (bottom > (winHeight + scrollTop))
            {
                var diff = bottom - (winHeight + scrollTop);
                pos.top -= diff;
                
                var pointerTop = HOVERCARD_POINTER_TOP + diff;
                if ((pointerTop + pointerHeight) > outerHeight)
                {
                    pointerTop = outerHeight - pointerHeight;
                }
                $pointer.css('top', (pointerTop + 'px'));
            }
            else if (pos.top < (scrollTop + offsetTop))
            {
                var diff = (scrollTop + offsetTop) - pos.top;
                pos.top += diff;
            }
            
            // Position the hovercard
            $hovercard.css(
            {
                top: (pos.top + 'px'),
                left: (pos.left + 'px')
            });
            
            // If the card was hidden before we began, then re-hide it
            if (isHidden)
            {
                $hovercard.hide();
            }
        },
        collision: 'flip none'
    });
};

HovercardManager.prototype.isVisible = function(id)
{
    var selector = '#hovercard';
    if (id)
    {
        selector += '[data-id="' + id + '"]';
    }
    selector += ':visible';
    return ($(selector).length === 1);
};

// Expose to the world
Origin.HovercardManager = new HovercardManager();

})(jQuery);
