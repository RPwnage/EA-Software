
!function ($) {

    "use strict";

    // OPTIONAL HTML CONTENT BUILDER

    $.otkCountBadgeCreate = function (element, options)
    {
        var $element = $(element);
        $element.addClass("otk-count-badge");
        $element.otkCountBadge(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkCountBadge",
    {
        // Default options.
        options:
        {
            value: 0,
            autoHide: false
        },
 
        _create: function()
        {
            //
            // See if there are already values for our defaults
            //
            var valueValue = this.element.text();

            if (valueValue)
            {
                this.options.value = valueValue;
            }
            if (this.element.hasClass("auto-hide"))
            {
                this.options.autoHide = true;
            }

            if(navigator.platform.match('Mac') !== null)
            {
            	// hack to get the label to align properly on Mac
                this.element.addClass('platform-mac');
            }

            // Listen for the animation to end and clear out the value after it's complete
            this.element.bind('webkitAnimationEnd', function()
            {
                $(this).css("webkitAnimationName", "");
            });

            this._update();
        },

         _setOption: function( key, value )
        {
            this.options[ key ] = value;
            this._update();
        },

        _update: function()
        {
            // Set the label
            if (this.options.value <= 99)
            {
                this.element.text(this.options.value);
            }
            else
            {
                this.element.text("... ");
            }

            if (this.options.autoHide)
            {
                this.element.addClass('auto-hide');
                if (this.options.value === 0)
                {
                    this.element.css('display', 'none');
                }
                else
                {
                    // Can't use hide()/show() here because show() sets display to "block"
                    this.element.css('display', 'inline-block');
                }
            }
            else
            {
                this.element.removeClass('auto-hide');
            }
        },

        _pop: function()
        {
            // Trigger our pop animation
            this.element.css("webkitAnimationName", "pop");
        },

        // *** BEGIN PUBLIC INTERFACE ***

        // Resets the count value back to 0
        clear: function()
        {
            this.options.value = 0;
            this._update();
            this._pop();
        },

        // Increment the count value by 1
        increment: function()
        {
            this.options.value++;
            this._update();
            this._pop();
        },

        // Returns the count value
        value: function(val)
        {
            return this.options.value;
        },

        // Set the value
        setValue: function(val)
        {
            this.options.value = val;
            this._update();
            this._pop();
        }

    });

}(jQuery);

