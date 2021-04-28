!function ($) {

    "use strict";

    // OPTIONAL HTML CONTENT BUILDER

    $.otkTooltipCreate = function (options)
    {
        var $element = $('<div></div>'), $html;
        $element.addClass("otk-tooltip");
        options.bindedElement.after($element);

        $element.otkTooltip(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkTooltip",
    {
        // Default options.
        options:
        {
            text: "",
            bindedElement: null,
            xOffset: 0,
            yOffset: 0
        },

        _create: function()
        {
            //
            // See if there are already values for our defaults
            //

            var textValue = this.element.text();

            if (textValue && !(this.options.text))
            {
                this.options.text = textValue;
            }

            this.element.ready(function() {
                this._adjustForScreenPosition();
            }.bind(this));

            $(window).resize(function() {
                this._adjustForScreenPosition();
            }.bind(this));

            this.element.resize(function() {
                this._adjustForScreenPosition();
            }.bind(this));

            this.options.bindedElement.mouseover(function() {
                this.element.toggle(true);
            }.bind(this));

            this.options.bindedElement.mouseout(function() {
                this.element.toggle(false);
            }.bind(this));

            this._update();
        },

        _setOption: function( key, value )
        {
            this.options[ key ] = value;
            this._update();
        },

        _update: function()
        {
            this.element.text(this.options.text);
            this._adjustForScreenPosition();
        },

        _adjustForScreenPosition: function()
        {
            this.element.toggle(false);
            this.element.removeClass("top right bottom left");
            var bindedPosition = this.options.bindedElement.offset(),
                centerOfParent, elementCenteredToParentX;

            bindedPosition.left += this.options.xOffset;
            bindedPosition.top += this.options.yOffset;

            centerOfParent = bindedPosition.left + (parseInt(this.options.bindedElement.width()) / 2),
            elementCenteredToParentX = centerOfParent - (parseInt(this.element.width()) / 2);


            // position tooltip above the binded element
            this.element.css({
                top : (bindedPosition.top - this.element.outerHeight() - 22) + 'px',
                left : elementCenteredToParentX + 'px'
            });
            this.element.addClass("bottom");
        },

        _finished: function()
        {
            this.element.hide();
            this.element.trigger("select");
        }
    });

}(jQuery);


