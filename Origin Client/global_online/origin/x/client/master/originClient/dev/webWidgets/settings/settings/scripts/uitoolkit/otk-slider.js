
!function ($) {

    "use strict";

    // OPTIONAL HTML CONTENT BUILDER

    $.otkSliderCreate = function (element, options)
    {
        var $element = $(element), $html;
        $element.addClass("otk-slider");
        $html = $(
			'<div class="track outer">' +
				'<div class="track inner">' +
					'<div class="thumb"></div>' +
					'<div class="track unfilled"></div>' +
				'</div>' +
		    '</div>'
            );
        $element.append($html);
        $element.otkSlider(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkSlider",
    {
        // Default options.
        options:
        {
            value: .5,
            pointerDirection: "none" // none (default), up, down
        },
 
        _create: function()
        {
            // See if there's a current pointer direction set
            if (this.element.hasClass('pointer-direction-up'))
            {
                this.options.pointerDirection = "up";
            }
            else if (this.element.hasClass('pointer-direction-down'))
            {
                this.options.pointerDirection = "down";
            }
            // else none

            // Start dragging the slider thumb on mousedown
            this.element.on('mousedown', function(evt)
            {
                if (evt.button !== 0) { return; }

                evt.preventDefault();

                this._startDrag(evt);
            }.bind(this));

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
            this.element.removeClass("pointer-direction-up pointer-direction-down");
            if (this.options.pointerDirection == "up")
            {
                this.element.addClass("pointer-direction-up");
            }
            else if (this.options.pointerDirection == "down")
            {
                this.element.addClass("pointer-direction-down");
            }
            else
            {
                // pointerDirection == none
            }

            this._updateSliderFromValue();
        },

        _updateSliderFromValue: function()
        {
            var $track, $thumb, $unfilled,
                thumbWidth, trackWidth,
                thumbX;

            $thumb = this.element.find('.thumb');
            $track = this.element.find('.track.inner');
            $unfilled = this.element.find('.track.unfilled');
            thumbWidth = parseInt($thumb.width());
            trackWidth = parseInt($track.width())-1;

            thumbX = (this.options.value * trackWidth) - (thumbWidth/2);

            $thumb.css({'left': thumbX + 'px'});
            $unfilled.css({'left': thumbX + 'px'});
        },

        _updateValueFromSlider: function()
        {
            var $track, $thumb, $unfilled,
                trackX, thumbWidth, trackWidth,
                thumbX;

            $thumb = this.element.find('.thumb');
            $track = this.element.find('.track.inner');
            $unfilled = this.element.find('.track.unfilled');
            thumbWidth = parseInt($thumb.width());
            trackWidth = parseInt($track.width())-1;
            trackX = parseInt($track.offset().left);
            thumbX = parseInt($thumb.position().left) + (thumbWidth/2);

            this.options.value = thumbX / trackWidth;
        },

        _onClick: function(ev)
        {
            var $track, $thumb, $unfilled, self,
                handleMoveEvent, stopDrag,
                trackX, thumbWidth, trackWidth,
                mouseX, thumbX;

            if (ev.which !== 1)
            {
                return;
            }

            self = this;
            $thumb = this.element.find('.thumb');
            $track = this.element.find('.track.inner');
            $unfilled = this.element.find('.track.unfilled');
            thumbWidth = parseInt($thumb.width());
            trackWidth = parseInt($track.width());
            trackX = parseInt($track.offset().left);

            mouseX = ev.pageX;
            thumbX = (mouseX - trackX);

            // Keep it in range
            if (thumbX < 0)
            {
                thumbX = 0;
            }
            if (thumbX > trackWidth)
            {
                thumbX = trackWidth;
            }

            $thumb.css({'left': thumbX-(thumbWidth/2) + 'px'});
            $unfilled.css({'left': thumbX + 'px'});

            self._updateValueFromSlider();

            self.element.trigger("valueChanged", self.value());
        },

        _startDrag: function(ev)
        {
            var $track, $thumb, $unfilled, self,
                handleMoveEvent, stopDrag,
                trackX, thumbWidth, trackWidth,
                mouseX, thumbX;

            if (ev.which !== 1)
            {
                return;
            }

            self = this;
            $thumb = this.element.find('.thumb');
            $track = this.element.find('.track.inner');
            $unfilled = this.element.find('.track.unfilled');
            thumbWidth = parseInt($thumb.width());
            trackWidth = parseInt($track.width());
            trackX = parseInt($track.offset().left);

            $thumb.addClass('dragging');

            // immediately jump to where the mouse is rather than waiting for mouse move events
            mouseX = ev.pageX;
            thumbX = (mouseX - trackX);

            // Keep it in range
            if (thumbX < 0)
            {
                thumbX = 0;
            }
            if (thumbX > trackWidth)
            {
                thumbX = trackWidth;
            }

            $thumb.css({'left': thumbX-(thumbWidth/2) + 'px'});
            $unfilled.css({'left': thumbX + 'px'});

            this._updateValueFromSlider();

            this.element.trigger("valueChanging", this.value());

            // Handle dragging
            handleMoveEvent = function(ev)
            {
                if (ev.which !== 1)
                {
                    // Not holding the LMB
                    stopDrag();
                    return;
                }

                mouseX = ev.pageX;
                thumbX = (mouseX - trackX);

                // Keep it in range
                if (thumbX < 0)
                {
                    thumbX = 0;
                }
                if (thumbX > trackWidth)
                {
                    thumbX = trackWidth;
                }

                $thumb.css({'left': thumbX-(thumbWidth/2) + 'px'});

                $unfilled.css({'left': thumbX + 'px'});

                self._updateValueFromSlider();

                self.element.trigger("valueChanging", self.value());
            };

            stopDrag = function()
            {
                if ($thumb.hasClass("dragging"))
                {
                    $thumb.removeClass('dragging');

                    self._updateValueFromSlider();

                    self.element.trigger("valueChanged", self.value());
                }

                self.element.off('mousemove', handleMoveEvent);
                $('body').off('mouseup', stopDrag);
            };

            self.element.on('mousemove', handleMoveEvent);
            $('body').on('mouseup', stopDrag);
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

