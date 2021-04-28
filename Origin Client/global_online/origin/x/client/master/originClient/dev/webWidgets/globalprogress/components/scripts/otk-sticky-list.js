
!function ($) {

    "use strict";

    // OPTIONAL HTML CONTENT BUILDER
    
    $.otkStickyListCreate = function (element, options)
    {
        var $element = $(element), $html;
        $element.addClass("otk-sticky-list");
        $html = $(
        );
        $element.append($html);
        $element.otkScrollPushList(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkScrollPushList",
    {
        // Default options.
        options:
        {
            title: "",
        },
 
        _create: function()
        {
            //
            // See if there are already values for our defaults or we have new options
            //
            this._update();
        },
 
        _setOption: function( key, value )
        {
            this.options[ key ] = value;
            this._update();
        },

        _update: function()
        {

        }
    });

}(jQuery);

