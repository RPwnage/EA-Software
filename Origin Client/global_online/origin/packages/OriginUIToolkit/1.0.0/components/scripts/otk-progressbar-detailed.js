
!function ($) {

    "use strict";

    // OPTIONAL HTML CONTENT BUILDER

    $.otkProgressBarDetailedCreate = function (element, options)
    {
        var $element = $(element), $html;
        $element.addClass("otk-progressbar-detailed");
        $html = $(
            '<h4 class="title"></h4>' +
            '<h5 class="sub-title"></h5>' +
            '<div class="otk-progressbar"></div>' +
            '<div class="status-container">' +
                '<h5 class="time-remaining"></h5>' +
                '<h5 class="bytes-per-second"></h5>' +
            '</div>' +
            '<h5 class="transfer-rate"></h5>'
        );
        $element.append($html);
        $element.otkProgressBarDetailed(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkProgressBarDetailed",
    {
        // Default options.
        options:
        {
            title: "",
            subtitle: "",
            bytesDownloaded: 0,
            totalFileSize: 0,
            bytesPerSecond: 0,
            percent: 0,
            secondsRemaining: '',
            formatBytesFunction: null,
            strings: {
                percentNotation: '%1',
                timeRemaining: '%1',
                seconds: '%1',
                bps: '%1'
            }
        },

        _create: function()
        {
            //
            // See if there are already values for our defaults or we have new options
            //

            var titleValue = this.element.find('.title').text(),
                subtitleValue = this.element.find('.sub-title').text(),
                $progressBar = this.element.find('.otk-progressbar');

            // Create a progress bar if there isn't one
            if((this.element.find('.otk-progressbar  > .bar').length) === 0)
            {
                $.otkProgressBarCreate($progressBar, {});
            }
            else
            {
                // Init our existing progress bar
                $progressBar.otkProgressBar({});
            }

            if(titleValue && !(this.options.title))
            {
                this.options.title = titleValue;
            }

            if(subtitleValue && !(this.options.subtitle))
            {
                this.options.subtitle = subtitleValue;
            }

            this.element.find('.title').bind('mouseenter', function(){
                var $this = $(this);
                if(this.offsetWidth < this.scrollWidth && !$this.attr('title')){
                    $this.attr('title', $this.text());
                }
            });

            this.element.find('.sub-title').bind('mouseenter', function(){
                var $this = $(this);
                if(this.offsetWidth < this.scrollWidth && !$this.attr('title')){
                    $this.attr('title', $this.text());
                }
            });

            this._update();
        },

        _setOption: function( key, value )
        {
            this.options[ key ] = value;
        },

        _update: function()
        {
            var percentValue = this.options.progress,
                $bytesPerSecond = this.element.find('.bytes-per-second'),
                $progressBar = this.element.find('.otk-progressbar'),
                $transferRate = this.element.find('.transfer-rate'),
                $timeRemaining = this.element.find('.time-remaining');
            // Take out any HTML the user might have been trying to sneak in with the strings.
            this.options.strings.percentNotation = this.options.strings.percentNotation.replace(/(<([^>]+)>)/ig,"");
            this.options.strings.seconds = this.options.strings.seconds.replace(/(<([^>]+)>)/ig,"");
            this.options.strings.bps = this.options.strings.bps.replace(/(<([^>]+)>)/ig,"");

            if(this.options.title !== null)
            {
                this.element.find('.title').text(this.options.title);
            }
            if(this.options.subtitle !== null)
            {
                this.element.find('.sub-title').text(this.options.subtitle);
            }

            //// 10%
            if(percentValue < 0)
            {
                percentValue = 0;
            }
            else if(percentValue > 1)
            {
                percentValue = 1;
            }
            percentValue = Math.floor(percentValue * 100);

            if(this.options.formatBytesFunction)
            {
                //// 4 GB / 12 GB
                if(this.options.totalFileSize || this.options.bytesDownloaded)
                {
                    $transferRate.text(this.options.formatBytesFunction(this.options.bytesDownloaded) + ' / ' + this.options.formatBytesFunction(this.options.totalFileSize));
                }
                else
                {
                    $transferRate.text("");
                }

                //// 100.00 B/sec
                if($.isNumeric(this.options.bytesPerSecond))
                {
                    $bytesPerSecond.html(this.options.strings.bps + " " + this.options.formatBytesFunction(this.options.bytesPerSecond) + '/' + this.options.strings.seconds);

                }
                else
                {
                    $bytesPerSecond.text(this.options.bytesPerSecond);
                }
            }

            //// 04:04:43 Remaining
            if(this.options.secondsRemaining)
            {
                $timeRemaining.text(this.options.strings.timeRemaining.replace("%1", this.options.secondsRemaining));
            }
            else
            {
                $timeRemaining.text("");
            }

        },

        // *** BEGIN PUBLIC INTERFACE ***

        // Returns the progress bar
        progressbar: function()
        {
            return this.element.find('.otk-progressbar');
        },

        update: function()
        {
            this._update();
        }
    });

}(jQuery);

