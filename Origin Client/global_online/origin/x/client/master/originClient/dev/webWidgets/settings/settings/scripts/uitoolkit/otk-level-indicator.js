
!function ($) {

    "use strict";

    // OPTIONAL HTML CONTENT BUILDER

    $.otkLevelIndicatorCreate = function (element, options)
    {
        var $element = $(element), $html;
        $element.addClass("otk-level-indicator");
        $html = $(
			'<div class="track outer">' +
				'<div class="track inner">' +
					'<div class="track unfilled"></div>' +
                    '<div class="segments">' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                        '<div class="segment"></div>' +
                    '</div>' +
				'</div>' +
		    '</div>'
            );
        $element.append($html);
        $element.otkLevelIndicator(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkLevelIndicator",
    {
        // Default options.
        options:
        {
            value: .5
        },
 
        _create: function()
        {
            this.element.ready(function()
            {
                this._update();
            }.bind(this));
        },
 
        _setOption: function( key, value )
        {
            this.options[ key ] = value;
            this._update();
        },

        _update: function()
        {
            this._updateLevelFromValue();
        },

        _updateLevelFromValue: function()
        {
            var $track, $unfilled,
                trackWidth,
                valueX;

            $track = this.element.find('.track.inner');
            $unfilled = this.element.find('.track.unfilled');
            trackWidth = parseInt($track.width())-1;

            valueX = (this.options.value * trackWidth);

            // Keep it in range
            if (valueX < 0)
            {
                valueX = 0;
            }
            else if (valueX > trackWidth)
            {
                valueX = trackWidth;
            }

            $unfilled.css({'left': valueX + 'px'});
        },

        // *** BEGIN PUBLIC INTERFACE ***

        // Returns the current slider value from 0-1
        value: function()
        {
            return this.options.value;
        },

        // Sets the slider to a value from 0-1
        setValue: function(value)
        {
            this.options.value = value;
            this._update();
        }

    });

}(jQuery);

