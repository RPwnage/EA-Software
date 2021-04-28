
!function ($) {

    "use strict";

    // OPTIONAL HTML CONTENT BUILDER

    $.otkRadioRatingCreate = function (element, options)
    {
        var $element = $(element), $html;
        $element.addClass("otk-radio-rating");
        $html = $(
            '<span class="lessLabel"></span>' +
            '<span class="moreLabel"></span>' +
            '<div class="divider"/>' +
            '<span class="radioContainer"></span></div>'
            );
        $element.append($html);

        $element.otkRadioRating(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkRadioRating",
    {
        // Default options.
        options:
        {
            lessLabel: "",
            moreLabel: "",
            count: 11,
            name: ""
        },
 
        _create: function()
        {
            //
            // See if there are already values for our defaults
            //

            var lessLabelValue = this.element.find('.lessLabel').text(),
                moreLabelValue = this.element.find('.moreLabel').text(),
                countValue = this.element.find('.otk-radio').length;

            if (lessLabelValue)
            {
                this.options.lessLabel = lessLabelValue;
            }
            if (moreLabelValue)
            {
                this.options.moreLabel = moreLabelValue;
            }

            if (countValue)
            {
                this.options.count = countValue;
            }
            
            this.element.ready(function() {
                if(this.element.css('width') !== this.element.parent().css('width'))
                {
                    this.element.css('width', this.element.find('.radioContainer').width() + 'px');
                }
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
            var $radioContainer = this.element.find('.radioContainer'),
                $radioButton,
                marginSpacing = 0;

            // Set the less label
            this.element.find('.lessLabel').text(this.options.lessLabel);

            // Set the more label
            this.element.find('.moreLabel').text(this.options.moreLabel);
            
            if(this.element.find('.otk-radio').length !== this.options.count)
            {
                $radioContainer.empty();
                for(var i = 0; i < this.options.count; i++)
                {
                    $radioButton = $("<label class='vertical'><input class='otk-radio' type='radio' value='" + i + "' name='" + this.options.name + "'/>" + i + "</label>");
                    $radioContainer.append($radioButton);
                }

                if(this.element.css('width') === this.element.parent().css('width'))
                {
                    // Get the width of radioContainer - getting it directly isn't completely reliable.
                    marginSpacing = $radioButton.width() * this.options.count;
                    // Take (Element - radionContainer) / numOptions
                    marginSpacing = (this.element.width() - marginSpacing) / this.options.count;
                    // Divide that by two to get even spacing on both sides.
                    marginSpacing = marginSpacing / 2;
                    this.element.find('.radioContainer > label').css({
                        'margin-left' : marginSpacing + 'px',
                        'margin-right' : marginSpacing + 'px'
                    });
                }
                else
                {
                    $(this).find('.otk-radio-rating').css('width', $(this).find('.radioContainer').width() + 'px');
                }
            }
        },

        // *** BEGIN PUBLIC INTERFACE ***
        
        value: function()
        {
            var $selected = this.element.find(".otk-radio[name='" + this.options.name  + "']:checked");
            return $selected.length ? $selected.val() : -1;
        }

    });

}(jQuery);

