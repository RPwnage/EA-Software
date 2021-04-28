;(function($, undefined){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }
if (!Origin.model) { Origin.model = {}; }
if (!Origin.util) { Origin.util = {}; }

var NavBarView = function()
{
    this._scrollPositionMap = {};
    
    // Add our platform attribute for the benefit of CSS
    $(document.body).attr('data-platform', Origin.views.currentPlatform());
}

NavBarView.prototype.__defineGetter__('currentTab', function()
{
    return $("#main").find(".tab-content.current").attr("id");
});

NavBarView.prototype.__defineSetter__('currentTab', function(val)
{
    if (this.currentTab == val)
    {
        return;
    }

    // Save current tab's scroll position.
    if (this.currentTab != undefined && this.currentTab != "build-viewer")
    {
        Origin.views.navbar._scrollPositionMap[this.currentTab] = $('#main').scrollTop();
    }

    // Clear and reset current content
    var content = $("#main").find(".tab-content");
    content.removeClass("current");
    content.each(function()
    {
        if ($(this).attr("id") == val)
        {
            $(this).addClass("current");
        }
    });

    // Re-apply persisted scroll position.
    var prevScrollPosition = Origin.views.navbar._scrollPositionMap[val];
    if (prevScrollPosition == undefined)
    {
        prevScrollPosition = 0;
    }
    $('#main').scrollTop(prevScrollPosition);
});

// Expose to the world
Origin.views.navbar = new NavBarView();

})(jQuery);