(function($){
"use strict";

var initChatLayout, layoutContent;

layoutContent = function()
{
};

initChatLayout = function()
{
    layoutContent();
};

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.views) { window.Origin.views = {}; }

window.Origin.views.initChatLayout = initChatLayout;

}(jQuery));
