
!function ($) {

    "use strict";

    // OPTIONAL HTML CONTENT BUILDER

    $.otkMiniLevelIndicatorCreate = function (element, options)
    {
        var $element = $(element);
        $element.addClass("otk-mini-level-indicator");
        $element.otkMiniLevelIndicator(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkMiniLevelIndicator",
    {
        // Default options.
        options:
        {
            numSegments: 6,
            orientation: "vertical",
            value: 0
        },
 
        _create: function()
        {
            // Create the proper number of divs
            this._update();
        },

         _setOption: function( key, value )
        {
            this.options[ key ] = value;
            this._update();
        },

        _update: function()
        {
            var numSegmentsHas = this.element.children(".segment").length;

            if (numSegmentsHas !== this.options.numSegments)
            {
                var $segment = $('<div class="segment"></div>');

                // Delete any current segments
                this.element.empty();

                // Create correct number of segments
                for (var i=0; i<this.options.numSegments; i++)
                {
                    this.element.append($segment.clone());
                }
            }

            this.element.attr("data-orientation", this.options.orientation);

            this._updateSegments();
        },

        _updateSegments: function()
        {
            // This function potentially gets called a LOT.
            // KEEP THINGS SNAPPY.

            var numSegments = this.options.numSegments,
                numSegmentsToLight;

            // Unlight all segments
            this.element.children(".segment.lit").removeClass("lit");

            // Calculate how many segments should be lit
            numSegmentsToLight = this.options.value * numSegments;

            // Light up just the segments we want
            if (this.options.orientation === "horizontal")
            {
                this.element.children(".segment").slice(0, numSegmentsToLight).addClass("lit");
            }
            else
            {
                this.element.children(".segment").slice(numSegments-numSegmentsToLight, numSegments).addClass("lit");
            }
        },

        // *** BEGIN PUBLIC INTERFACE ***

        // Returns the value
        value: function(val)
        {
            return this.options.value;
        },

        // Set the value
        setValue: function(val)
        {
            if (val !== this.options.value)
            {
                this.options.value = val;

                // Don't call _update() here as it's too expensive
                this._updateSegments();
            }
        }

    });

}(jQuery);

