;(function($){
"use strict";

if (!window.OriginDropDown) { console.log('Must include UIToolkit OriginDropDown class to use the jQuery OriginDropDown plugin - exiting.'); return; }

// Expose as jQuery plugin
$.fn.dropdown = function(settings)
{
    settings = $.extend(
    {
    }, settings);
    
    return this.each(function(idx)
    {
        new OriginDropDown($(this), settings);
    });
};

})(jQuery);
