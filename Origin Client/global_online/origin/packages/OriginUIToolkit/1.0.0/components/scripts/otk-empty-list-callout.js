
!function ($) {

    "use strict";

    // OPTIONAL HTML CONTENT BUILDER

    $.otkEmptyListCalloutCreate = function (element, options)
    {
        var $element = $(element), $html;
        $element.addClass("otk-empty-list-callout");
        $html = $(
            '<h3 class="title"></h3>' +
            '<p class="description"></p>' +
            '<button class="otk-button tertiary"></button>' 
            );
        $element.append($html);
        $element.otkEmptyListCallout(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkEmptyListCallout",
    {
        // Default options.
        options:
        {
            titleLabel: "This is the Title",
            descriptionLabel: "",
            buttonLabel: "Callout",
            /*
                The following styles are supported:
                    "plain-style"
                    "info-style"
                    "etched-style"
            */
            style: "plain-style"
        },
 
        _create: function()
        {
            //
            // See if there are already values for our defaults
            //
            var titleLabelValue = this.element.find('.title').text(),
                descriptionLabelValue = this.element.find('.description').text(),
                buttonLabelValue = this.element.find('.otk-button').text(),
                styleValue = this.element.is('[data-style]') ? this.element.attr('data-style') : "";

            if (titleLabelValue)
            {
                this.options.titleLabel = titleLabelValue;
            }
            if (descriptionLabelValue)
            {
                this.options.descriptionLabel = descriptionLabelValue;
            }
            if (buttonLabelValue)
            {
                this.options.buttonLabel = buttonLabelValue;
            }
            if (styleValue)
            {
                this.options.style = styleValue;
            }

           this.element.on('click', '.otk-button', function(evt)
            {
                evt.preventDefault();
                evt.stopImmediatePropagation();

                // Signal that the button has been clicked
                this.element.trigger("buttonClicked");
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
            this.element.find('.title').text(this.options.titleLabel);
            this.element.find('.description').text(this.options.descriptionLabel);
            this.element.find('.otk-button').text(this.options.buttonLabel);
            this.element.attr('data-style', this.options.style);
        }

    });

}(jQuery);

