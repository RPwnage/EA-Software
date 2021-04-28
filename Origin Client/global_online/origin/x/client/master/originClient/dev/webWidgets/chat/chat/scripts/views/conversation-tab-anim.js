(function($){
"use strict";

var animateConversationTabIn, animateConversationTabOut, finishAnimation,
    visibleProps, hiddenProps, resetProps, duration;

// Properties for a visible tab
visibleProps = {
    'opacity': '1',
    'margin-top': '0'
};

// Properties for a hidden tab
hiddenProps = {
    'opacity': '0',
    'margin-top': '-49px'
};

// Properties to reset the tab CSS
resetProps = {
    'opacity': '',
    'margin-top': ''
};

// Duration in ms
duration = 150;

finishAnimation = function($tab, callback)
{
    $tab.css(resetProps).removeClass('animating showing hiding');

    if (callback !== undefined)
    {
        callback();
    }
};

animateConversationTabIn = function($tab, callback)
{
    $tab.css(hiddenProps).addClass('animating showing')
        .animate(visibleProps, duration);
    
    $tab.one('webkitTransitionEnd', function(evt)
    {
        finishAnimation($tab, callback);
        evt.stopPropagation();
    });
};

animateConversationTabOut = function($tab, callback)
{
    $tab.css(visibleProps).addClass('animating hiding')
        .animate(hiddenProps, duration);
    
    $tab.one('webkitTransitionEnd', function(evt)
    {
        finishAnimation($tab, callback);
        evt.stopPropagation();
    });
};

window.Origin.views.animateConversationTabIn = animateConversationTabIn;
window.Origin.views.animateConversationTabOut = animateConversationTabOut;

}(Zepto));
