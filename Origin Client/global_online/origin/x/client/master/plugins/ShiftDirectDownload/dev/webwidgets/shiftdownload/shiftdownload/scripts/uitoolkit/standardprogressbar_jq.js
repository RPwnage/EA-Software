;(function($){
"use strict";

if (!window.StandardProgressBar) { console.log('Must include UIToolkit StandardProgressBar class to use the jQuery StandardProgressBar plugin - exiting.'); return; }

// Expose as jQuery plugin
$.fn.progressBar = function(settings)
{
    settings = $.extend(
    {
        type: 'Standard'
    }, settings);
    
    var construct = window[settings.type + 'ProgressBar'];
    
    return this.each(function(idx)
    {
        if (!construct) { return false; }
        var $elm = $(this);
        
        new construct($elm);
    });
};

})(jQuery);
