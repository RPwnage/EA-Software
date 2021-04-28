
!function ($) {

    "use strict";

    // OPTIONAL HTML CONTENT BUILDER

    $.otkCustomGlyphCreate = function (element, options)
    {
        var $element = $(element), $html;
        $element.addClass("otk-custom-glyph");
        $html = $(
			'<span class="shadow layer"></span>' +
			'<span class="glyph layer"></span>'
        );
        $element.append($html);
        $element.otkCustomGlyph(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkCustomGlyph",
    {
        // Default options.
        options:
        {
            maskImageURL: "",
            glyphColor: "",
            shadowColor: "",
            shadowOffsetX: 0,
            shadowOffsetY: 1,
            shadowOpacity: 1,
            autoSize: true
        },
 
        _create: function()
        {
            //
            // See if there are already values for our defaults
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
            var $shadow = this.element.find(".shadow"),
                $glyph = this.element.find(".glyph");

            this.element.find(".layer").css({
                        "-webkit-mask-image": "url('" + this.options.maskImageURL + "')",
                        "mask-image": "url('" + this.options.maskImageURL + "')"
                        });

            $glyph.css("background-color", this.options.glyphColor);

            $shadow.css({
                        "background-color": this.options.shadowColor,
                        "left": this.options.shadowOffsetX,
                        "right": -(this.options.shadowOffsetX),
                        "top": this.options.shadowOffsetY,
                        "bottom": -(this.options.shadowOffsetY),
                        "opacity": this.options.shadowOpacity
                        });

            // Set our size to the size of the image
            // Note this doesn't increase the size of the glyph to account for the shadow offset
            // Not sure if it should or not
            if (this.options.autoSize)
            {
                var img = new Image();
                img.onload = function()
                {
                    this.element.css({
                        "width": img.width,
                        "height": img.height
                        });
                }.bind(this);
                img.src = this.options.maskImageURL;
            }

        },

        // *** BEGIN PUBLIC INTERFACE ***

        maskImageURL: function()
        {
            return this.options.maskImageURL;
        },

        setMaskImageURL: function(url)
        {
            this.options.maskImageURL = url;
            this._update();
        },

        glyphColor: function()
        {
            return this.options.glyphColor;
        },

        setGlyphColor: function(color)
        {
            this.options.glyphColor = color;
            this._update();
        },

        shadowColor: function()
        {
            return this.options.shadowColor;
        },

        setShadowColor: function(color)
        {
            this.options.shadowColor = color;
            this._update();
        },

        shadowOffsetX: function()
        {
            return this.options.shadowOffsetX;
        },

        setShadowOffsetX: function(offset)
        {
            this.options.shadowOffsetX = offset;
            this._update();
        },

        shadowOffsetY: function()
        {
            return this.options.shadowOffsetY;
        },

        setShadowOffsetY: function(offset)
        {
            this.options.shadowOffsetY = offset;
            this._update();
        },

        shadowOpacity: function()
        {
            return this.options.shadowOpacity;
        },

        setShadowOpacity: function(opacity)
        {
            this.options.shadowOpacity = opacity;
            this._update();
        },

        autoSize: function()
        {
            return this.options.autoSize;
        },

        setAutoSize: function(asize)
        {
            this.options.autoSize = asize;
            this._update();
        }

    });

}(jQuery);

