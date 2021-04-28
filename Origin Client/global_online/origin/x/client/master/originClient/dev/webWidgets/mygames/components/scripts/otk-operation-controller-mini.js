
!function ($) {

    "use strict";

    // OPTIONAL HTML CONTENT BUILDER
    
    $.otkOperationControllerMiniCreate = function (element, options) {
        var $element = $(element), $html;
        $element.addClass("otk-operation-controller-mini");
        $html = $(
            '<div class="container">' +
                '<div class="progress-container">' + 
                    '<div class="otk-progressbar"></div>' +
                '</div>' + 
                '<div class="button-container">' +
                    '<button class="otk-button toolbar play"></button>' +
                '</div>' +
            '</div>' +
            '<div class="progress-info"></div>'
        );
        $element.append($html);

        $element.otkOperationControllerMini(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget("origin.otkOperationControllerMini", {
        // Default options.
        options:
            {
                playable: false,
                text: '',
                showButtons: true,
                playableHoverText: ''
            },
 
        _create: function () {
            //
            // See if there are already values for our defaults or we have new options
            //
            var textValue = this.element.find('.progress-info').text(),
                showButtonValue = this.element.attr('showButtons');
            
            // Create a progress bar if there isn't one
            if ((this.element.find('.otk-progressbar  > .bar').length) === 0)
            {
                $.otkProgressBarCreate(this.element.find('.otk-progressbar'), {});
            }
            else
            {
                // Init our existing progress bar
                this.element.find('.otk-progressbar').otkProgressBar({});
            }
            
            if(textValue && !(this.options.text))
            {
                this.options.text = textValue;
            }
            
            if(typeof showButtonValue !== "undefined")
            {
                this.options.showButtons = showButtonValue;
            }

            this._update();
        },
 
        _setOption: function( key, value )
        {
            this.options[ key ] = value;
            this._update();
        },

        _update: function()
        {
            this.element.find('.progress-info').text(this.options.text);
            this._setEnabled(this.element.find('.play'), this.options.playable);
            this.element.find('.play').attr('title', this.options.playableHoverText);
            this.setShowButtons(this.options.showButtons);
        },
        
        // Helper for showing actions
        _setVisible: function(item, visisble)
        {
            visisble ? item.show() : item.hide();
        },
        
        _setEnabled: function(item, enabled)
        {
            enabled ? item.removeAttr("disabled") : item.attr("disabled", "disabled");
        },
        
        // *** BEGIN PUBLIC INTERFACE ***

        // Returns the progress bar
        progressbar: function()
        {
            return this.element.find('.otk-progressbar');
        },
        
        buttonPlay: function()
        {
            return this.element.find('.play');
        },
        
        setShowButtons: function(showButtons)
        {
            this.options.showButtons = showButtons;
            this.element.attr('has-buttons', showButtons);
        }
    });
}(jQuery);