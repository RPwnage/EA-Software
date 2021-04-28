(function($){
"use strict";

var initConversationTabDrag, startDrag, animationMs, dislodgeThresholdPx;

animationMs = 125;
dislodgeThresholdPx = 10;

startDrag = function(ev)
{
    var $tab, $tabContainer, handleMoveEvent, baseY, stopDrag,
        animateTabSwap, pauseDrag, hasDislodged = false; 

    $tab = $(this);
    $tabContainer = $tab.parent();

    if ($tab.hasClass('draggable') === false)
    {
        return;
    }

    // If we're not already scrolled don't show scrollbars while dragging a
    // tab past the bottom.
    // We could also not allow dragging the tab down that far but we never
    // deal with absolute positions so the calculations would be really 
    // annoying. We can also drag a tab past the top so this is symmetrical
    if ($tabContainer[0].scrollHeight === $tabContainer[0].offsetHeight)
    {
        $tabContainer.css('overflow', 'hidden');
    }

    baseY = ev.y + $tabContainer[0].scrollTop;

    animateTabSwap = function($otherTab, startTopPx)
    {
        $otherTab.css('top', startTopPx + 'px');
        $otherTab.animate({top: '0'}, animationMs);
        $otherTab.one('webkitTransitionEnd', function()
        {
            $otherTab.css('top', '');
        });
    };

    handleMoveEvent = function(ev)
    {
        var deltaY = ev.y + $tabContainer[0].scrollTop - baseY, $previousTab,
            $nextTab, previousTabHeight, nextTabHeight;

        if (ev.which !== 1)
        {
            // Not holding the LMB
            stopDrag();
            return;
        }
        
        for($previousTab = $tab.prev();
            $previousTab.length;
            $previousTab = $tab.prev())
        {
            // Account for the border
            previousTabHeight = $previousTab.height() + 1;
        
            if (deltaY > (-previousTabHeight / 2))
            {
                // We're done
                break;
            }
            
            hasDislodged = true;
            
            // Swap with the previous tab
            $tab.insertBefore($previousTab);

            deltaY += previousTabHeight;
            baseY -= previousTabHeight;

            animateTabSwap($previousTab, -($tab.height() + 1));
        }

        for($nextTab = $tab.next();
            $nextTab.length;
            $nextTab = $tab.next())
        {
            nextTabHeight = $nextTab.height() + 1;

            if (deltaY < (nextTabHeight / 2))
            {
                break;
            }

            hasDislodged = true;
            
            $tab.insertAfter($nextTab);

            deltaY -= nextTabHeight;
            baseY += nextTabHeight;
            
            animateTabSwap($nextTab, $tab.height() + 1);
        }
        
        if (hasDislodged || (Math.abs(deltaY) > dislodgeThresholdPx))
        {
            hasDislodged = true;

            $tab.addClass('dragging')
                .css({
                    'position': 'relative',
                    'z-index': '200',
                    'top': deltaY + 'px'
                });
        }
    };

	// Removes our visual drag effect to indicate we can't drop here
    // If the mouse re-enters the drag area the drag will continue
    pauseDrag = function()
    {
        $tab.animate({'top': 0}, animationMs);
    };
	
    stopDrag = function()
    {
        // Kill our z-index, top and position
        $tab.animate({'top': 0}, animationMs);
        $tab.one('webkitTransitionEnd', function()
        {
            $tab.removeClass('dragging')
                .css({
                    'top': '',
                    'z-index': '',
                    'position': ''
                });
        });

        $tabContainer.css('overflow', '');

        $tabContainer.off('mousemove', handleMoveEvent);
        $tabContainer.off('mouseleave', pauseDrag);
        $('body').off('mouseup', stopDrag);
    };

    $tabContainer.on('mousemove', handleMoveEvent);
    $tabContainer.on('mouseleave', pauseDrag);
    $('body').on('mouseup', stopDrag);
};

initConversationTabDrag = function()
{
    $('body').on('mousedown', 'li.tab', function (evt)
    {
        // Only allow dragging by the left button
        if (evt.button !== 0)
        {
            return;
        }

        // Don't allow the user to drag by the endcap
        if (!$(evt.toElement).is('footer'))
        {
            startDrag.call(this, evt);
        }
    });
};

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.initConversationTabDrag = initConversationTabDrag;

}(Zepto));
