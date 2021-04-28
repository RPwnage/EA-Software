
!function ($) {

    "use strict";

    // OPTIONAL HTML CONTENT BUILDER
    
    $.otkOperationControllerCreate = function (element, options) {
        var $element = $(element), $html;
        $element.addClass("otk-operation-controller");
        $html = $(
            '<div class="boxart-container"><img class="boxart"></img></div>' +
            '<div class="operation-info">' +
                '<div class="otk-progressbar-detailed"></div>' +
                '<div class="button-container">'+
                    '<button class="otk-button toolbar pause"></button>' +
                    '<button class="otk-button toolbar resume"></button>' +
                    '<button class="otk-button toolbar cancel"></button>' +
                    '<button class="otk-button tertiary play"></button>' +
                    '<button class="otk-button tertiary debug"></button>' +
                '</div>' +
            '</div>'
        );
        $element.append($html);

        $element.otkOperationController(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget("origin.otkOperationController", {
        // Default options.
        options:
            {
                boxartUrl: '',
                playable: false,
                pausable: false,
                cancellable: false,
                resumable: false,
                debugable: false,
                strings: {
                    play: '%1',
                    pause: '%1',
                    cancel: '%1',
                    resume: '%1',
                    disabled: '%1',
                    debug: '%1',
                    playTooltip: '%1'
                }
            },
 
        _create: function () {
            //
            // See if there are already values for our defaults or we have new options
            //
            
            var boxartValue = this.element.find('.boxart').src,
                playableValue = this.element.find('.play').is(':visible');
            
            // Create a progress bar if there isn't one
            if ((this.element.find('.otk-progressbar  > .bar').length) === 0)
            {
                $.otkProgressBarDetailedCreate(this.element.find('.otk-progressbar-detailed'), {});
            }
            else
            {
                // Init our existing progress bar
                this.element.find('.otk-progressbar-detailed').otkProgressBarDetailed({});
            }
            
            if(boxartValue && !(this.options.boxartUrl))
            {
                this.options.boxartUrl = boxartValue;
            }
            
            this.element.find('.boxart').error(function() {
                // There was an error loading the box art... set to default.
                this.options.boxartUrl = "./components/images/otk-operation-controller/default-boxart.jpg";
                this.element.find('.boxart').attr("src", this.options.boxartUrl);
            }.bind(this));
            
            this._update();
        },
 
        _setOption: function( key, value )
        {
            this.options[ key ] = value;
        },

        _update: function()
        {
            var $btnPause = this.element.find('.pause'),
                $btnCancel = this.element.find('.cancel'),
                $btnResume = this.element.find('.resume'),
                $btnPlay = this.element.find('.play'),
                $btnDebug = this.element.find('.debug');
            $btnPause.attr("title", this.options.pausable ? this.options.strings.pause : this.options.strings.disabled);
            $btnCancel.attr("title", this.options.cancellable ? this.options.strings.cancel : this.options.strings.disabled);
            $btnResume.attr("title", this.options.resumable ? this.options.strings.resume : this.options.strings.disabled);
            $btnPlay.attr("title", this.options.strings.playTooltip);
            if(this.options.strings.play)
            {
                $btnPlay.text(this.options.strings.play);
            }
            if(this.options.strings.debug)
            {
                $btnDebug.text(this.options.strings.debug);
            }
            if(this.options.boxartUrl)
            {
                this.element.find('.boxart').attr("src", this.options.boxartUrl);
            }
            this._setEnabled($btnPause, this.options.pausable);
            this._setEnabled($btnResume, this.options.resumable);
            this._setEnabled($btnCancel, this.options.cancellable);
            this._setVisible($btnResume, this.options.resumable);
            this._setVisible($btnPause, this.options.resumable === false);
            this._setEnabled($btnPlay, this.options.playable);
            this._setVisible($btnDebug, this.options.debugable);
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
        progressbardetailed: function()
        {
            return this.element.find('.otk-progressbar-detailed');
        },
        
        progressbar: function()
        {
            return this.element.find('.otk-progressbar');
        },
        
        buttonPause: function()
        {
            return this.element.find('.pause');
        },
        
        buttonCancel: function()
        {
            return this.element.find('.cancel');
        },
        
        buttonResume: function()
        {
            return this.element.find('.resume');
        },
        
        buttonPlay: function()
        {
            return this.element.find('.play');
        },
        
        buttonDebug: function()
        {
            return this.element.find('.debug');
        },
        
        update: function()
        {
            this._update();
        }
    });
}(jQuery);