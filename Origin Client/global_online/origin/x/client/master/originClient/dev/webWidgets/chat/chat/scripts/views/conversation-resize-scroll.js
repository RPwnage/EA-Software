(function($){
"use strict";

var initConversationResizeScroll, isAtBottom = true, isManuallyScrolled;

initConversationResizeScroll = function()
{
    var $history, historyEl, lastSeenSize, updateLastSeenSize;
    
    $history = $('ol#history');
    historyEl = $history[0];

    updateLastSeenSize = function()
    {
        // Keep track of our size to detect resizes
        lastSeenSize = {
            width: historyEl.clientWidth,
            height: historyEl.clientHeight
        };
    };

    $history.on('scroll', function(ev)
    {
        if ((lastSeenSize.width !== historyEl.clientWidth) ||
            (lastSeenSize.height !== historyEl.clientHeight))
        {
            // We probably scrolled due to a resize
            // The user didn't initiate this so don't update isAtBottom
            updateLastSeenSize();
            return;
        }

        // Are we all the way scrolled?
        isAtBottom = historyEl.scrollHeight - historyEl.scrollTop === historyEl.clientHeight; 
    });

    $(window).on('resize', function()
    {
        if (isAtBottom)
        {
            // Force the scroll to the bottom
            historyEl.scrollTop = historyEl.scrollHeight - historyEl.clientHeight;
        }
    });

    updateLastSeenSize();
};

isManuallyScrolled = function ()
{
    return !isAtBottom;
};

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.initConversationResizeScroll = initConversationResizeScroll;
window.Origin.views.isManuallyScrolled = isManuallyScrolled;

}(jQuery));
