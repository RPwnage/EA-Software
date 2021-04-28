
!function ($) {

    "use strict";

    var ProgressBarStates = {
        Active: 'State-Active',
        Indeterminate: 'State-Indeterminate',
        Paused: 'State-Paused',
        Complete: 'State-Complete'
    }

    var ProgressBarColors = {
        ColorBlue: 'Color-Blue',
        ColorGreen: 'Color-Green'
    }

    // OPTIONAL HTML CONTENT BUILDER

    $.otkProgressBarCreate = function (element, options)
    {
        var $element = $(element), $html;
        $element.addClass("otk-progressbar");
        $html = $(
            '<div class="bar" data-progress-value="0"></div>' +
            '<h5 class="text"></h5>'
            );
        $element.append($html);

        $element.otkProgressBar(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkProgressBar",
    {
        // Default options.
        options:
        {
            progress: 0,
            state: '',
            text: '',
            color: 'Color-Blue',
            _dividers: [/*{id:'', percent:'', dividerElement:null, flagElement:null, color:''}*/]
        },

        _create: function()
        {
            //
            // See if there are already values for our defaults
            //

            var progressValue = Number(this.element.find('> .bar').attr('data-progress-value')),
                textValue = this.element.find('.text').text(),
                colorValue = this.element.attr("color");

            this.options._dividers = [];

            if(progressValue && !(this.options.progress))
            {
                this.options.progress = progressValue;
            }

            if(textValue && !(this.options.text))
            {
                this.options.text = textValue;
            }

            if(!(this.options.state))
            {
                if(this.element.hasClass('indeterminate'))
                {
                    this.options.state = ProgressBarStates.Indeterminate;
                }
                else if(this.element.hasClass('paused'))
                {
                    this.options.state = ProgressBarStates.Paused;
                }
                else if(this.element.hasClass('complete'))
                {
                    this.options.state = ProgressBarStates.Complete;
                }
                else if(this.element.hasClass('active'))
                {
                    this.options.state = ProgressBarStates.Active;
                }
            }

            if(colorValue && !(this.options.color))
            {
                this.options.color = colorValue;
            }

            this._update();
        },

        _setOption: function( key, value )
        {
            this.options[ key ] = value;
        },

        _update: function()
        {
            if(this.options.text)
            {
                this.setText(this.options.text);
            }

            this.setProgress(this.options.progress);

            switch(this.options.state)
            {
            case ProgressBarStates.Indeterminate:
                this.setIndeterminate(true);
                break;
            case ProgressBarStates.Paused:
                this.setPaused(true);
                break;
            case ProgressBarStates.Complete:
                this.setComplete(true);
                break;
            case ProgressBarStates.Active:
            default:
                this.element.toggleClass('active', true);
                this.setIndeterminate(false);
                this.setPaused(false);
                this.setComplete(false);
                break;
            }
        },

        // Clears all classes on our element. Element can have the classes
        // active, indeterminate, paused, complete on it at once. This will
        // ensure we don't have all of them at once.
        _resetToDefaultClass: function()
        {
            this.element.removeClass();
            this.element.addClass("otk-progressbar");
        },

        // *** BEGIN PUBLIC INTERFACE ***

        setText: function(text)
        {
            this.element.find('.text').text(text);
        },

        setProgress: function(val)
        {
            var percent = val,
                color = this.options.color,
                iDivider = 0;
            if(val < 0)
            {
                val = 0;
            }
            else if(val > 1)
            {
                val = 1;
            }
            val = Math.floor(val * 100);

            this.element.find('.bar').width(val + '%').attr('data-progress-value', val);
        },

        setPaused: function(isPaused)
        {
            this.element.toggleClass('paused', isPaused);
        },

        setComplete: function(isComplete)
        {
            if(isComplete)
            {
                this._resetToDefaultClass();
                this.element.find('.bar').width('100%');
            }
            this.element.toggleClass('complete', isComplete);
        },

        setIndeterminate: function(isIndeterminate)
        {
            if(isIndeterminate)
            {
                this._resetToDefaultClass();
                this.element.find('.bar').width('100%').attr('data-progress-value', '100');
                this.setText('');
            }
            this.element.toggleClass('indeterminate', isIndeterminate);
        },

        addProgressFlag: function(id, percent, flagText, color)
        {
            var offsetLeft = (this.element.innerWidth() * percent),
                $flag = null,
                $divider = null;
            if(percent > 0)
            {
                if(flagText)
                {
                    $flag = $('<div></div>');
                    $flag.addClass("otk-divider-flag");
                    $flag.text(flagText);
                    this.element.before($flag);

                    // Flag will be on the right side of the 'pole'
                    if(this.element.width() - offsetLeft > $flag.outerWidth())
                    {
                        $flag.css({
                            'margin-left': (offsetLeft + 1) + 'px'
                        });
                    }
                    // Flag will be on the left side of the 'pole'
                    else
                    {
                        $flag.css({
                            'margin-left': (offsetLeft + 3) - $flag.outerWidth() + 'px'
                        });
                    }
                }

                $divider = $('<div></div>');
                $divider.addClass("otk-divider vertical");
                this.element.append($divider);

                $divider.css({
                    left: offsetLeft + 'px'
                });
            }

            $.merge(this.options._dividers, [{id:id, percent:percent, dividerElement:$divider, flagElement:$flag, color:color}]);
        },

        setFlagSettings: function(id, percent, flagText, color)
        {
            var divider = this.divider(id);
            if(!divider)
            {
                this.addProgressFlag(id, percent, flagText, color);
                return;
            }

            if(divider.percent != percent)
            {
                this.removeProgressFlag(id);
                this.addProgressFlag(id, percent, flagText, color);
            }
            else
            {
                if(divider.color != color)
                {
                    divider.color = color;
                }
                if(divider.flagElement && (divider.flagElement.text != flagText))
                {
                    divider.flagElement.text(flagText);
                }
            }
        },

        divider: function(id)
        {
            var div = null,
                iDivider = 0;
            for(iDivider = 0; iDivider < this.options._dividers.length; iDivider++)
            {
                if(this.options._dividers[iDivider].id === id)
                {
                    div = this.options._dividers[iDivider];
                    break;
                }
            }
            return div;
        },

        toggleColorProgressFlag: function(id, toggleOn)
        {
            var divider = this.divider(id),
                color = this.element.attr("color");
            if(divider)
            {
                color = toggleOn ? divider.color : this.options.color;
            }

            if(color !== this.element.attr("color"))
            {
                // We don't want to change the options colors. This is in the case we want to start
                // the progress over.
                this.element.attr("color", color);
                if(divider)
                {
                    if(divider.flagElement)
                    {
                        divider.flagElement.attr("color", color);
                    }
                    if(divider.dividerElement)
                    {
                        divider.dividerElement.attr("color", color);
                    }
                }
            }
        },

        removeProgressFlag: function(id)
        {
            var divider = this.divider(id);
            if(divider)
            {
                if(divider.dividerElement)
                {
                    divider.dividerElement.remove();
                }
                if(divider.flagElement)
                {
                    divider.flagElement.remove();
                }
            }
            this.options._dividers = $.grep(this.options._dividers, function(div) {return div.id != id;});
            this.element.attr("color", this.options.color);
        },

        removeAllProgressFlags: function()
        {
            var iDivider = 0;
            for(iDivider = 0; iDivider < this.options._dividers.length; iDivider++)
            {
                if(this.options._dividers[iDivider].dividerElement)
                {
                    this.options._dividers[iDivider].dividerElement.remove();
                }
                if(this.options._dividers[iDivider].flagElement)
                {
                    this.options._dividers[iDivider].flagElement.remove();
                }
            }
            this.options._dividers = [];
            this.element.attr("color", this.options.color);
        },

        update: function()
        {
            this._update();
        }

    });

}(jQuery);

