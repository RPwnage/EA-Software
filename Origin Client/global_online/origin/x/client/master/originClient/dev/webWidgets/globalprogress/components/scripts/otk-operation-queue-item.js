
!function ($) {

    "use strict";

    // CONSTANTS
    var constants = new (function()
    {
        var animationExpandMs = 100,
            animationCollapseMs = 100;

        this.animationExpandMs = function() { return animationExpandMs; };
        this.animationCollapseMs = function() { return animationCollapseMs; };

    })();

    // OPTIONAL HTML CONTENT BUILDER

    $.otkOperationQueueItemCreate = function (element, options) {
        var $element = $(element), $html;
        $element.addClass("otk-operation-queue-item");
        $html = $(
            '<div class="otk-expand-collapse-indicator"></div>' +
            '<section class="title-container column">' +
                '<div class="title"></div>' +
                '<div class="sub-title"></div>' +
            '</section>' +
            '<section class="info-container column">' +
                '<a class="otk-hyperlink" href="#"></a>' +
                '<div class="status"></div>' +
            '</section>' +
            '<button class="column otk-close-button icon"></button>'
        );
        $element.append($html);
        $element.otkOperationQueueItem(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkOperationQueueItem", {
        // Default options.
        options:
        {
            title: "",
            subtitle: "",
            action: "",
            cancellable: false,
            progressPercent: 0.0,
            isMini: false,
            expanded: true,
            strings: {
                percentNotation: '%1',
                cancel: '%1'
            }
        },

        _create: function()
        {
            //
            // See if there are already values for our defaults or we have new options
            //
            var titleValue = this.element.find('.title').text(),
                subTitleValue = this.element.find('.sub-title').text(),
                actionValue = this.element.find('.otk-hyperlink').text(),
                isMiniValue = this.element.hasClass("mini"),
                expandedValue = this.element.hasClass("expanded");

            if(titleValue && !(this.options.title))
            {
                this.options.title = titleValue;
            }

            if(subTitleValue && !(this.options.subtitle))
            {
                this.options.subtitle = subTitleValue;
            }

            this.element.find('.title').bind('mouseenter', function() {
                var $this = $(this);
                if(this.offsetWidth < this.scrollWidth && !$this.attr('title')) {
                    $this.attr('title', $this.text());
                }
            });

            this.element.find('.sub-title').bind('mouseenter', function() {
                var $this = $(this);
                if(this.offsetWidth < this.scrollWidth && !$this.attr('title')) {
                    $this.attr('title', $this.text());
                }
            });

            if(actionValue && !(this.options.action))
            {
                this.options.action = actionValue;
            }

            if(isMiniValue && !(this.options.isMini))
            {
                this.options.isMini = isMiniValue;
            }

            if (expandedValue)
            {
                this.options.expanded = expandedValue;
            }

            // Toggle expanded/collapsed on click
            this.element.on('click', function()
            {
                this.toggle();
            }.bind(this));


            this._update();
        },

        _setOption: function( key, value )
        {
            this.options[ key ] = value;
        },

        _update: function()
        {
            var toggleSelector;
            // Add/remove expanded class based on expanded option
            this.element.toggleClass('expanded', this.options.expanded);

            // Animate the visibility of the toggle target (if there is one specified)
            toggleSelector = this.element.attr('data-toggle-target');
            if (toggleSelector)
            {
                if (this.element.hasClass('expanded'))
                {
                    $(toggleSelector).slideDown(constants.animationExpandMs());
                }
                else
                {
                    $(toggleSelector).slideUp(constants.animationCollapseMs());
                }
            }

            // update the expand / collapse indicator
            if (this.element.hasClass('expanded'))
            {
                this.element.find('.otk-expand-collapse-indicator').addClass('expanded');
            }
            else
            {
                this.element.find('.otk-expand-collapse-indicator').removeClass('expanded');
            }

            if(this.options.title)
            {
                this.element.find('.title').text(this.options.title);
            }

            if(this.options.subtitle)
            {
                this.element.find('.sub-title').text(this.options.subtitle);
            }

            if(this.options.strings.cancel)
            {
                this.element.find('.otk-close-button').attr('title', this.options.strings.cancel);
            }

            this._setVisible(this.element.find('.otk-close-button'), this.options.cancellable);
            this.element.find('.otk-hyperlink').text(this.options.action);
            this.setIsMini(this.options.isMini);

            if(this.options.progressPercent)
            {
                var progress = this.options.progressPercent;
                if(progress < 0)
                {
                    progress = 0;
                }
                else if(progress > 1)
                {
                    progress = 1;
                }
                progress = Math.floor(progress * 100);
                this.element.find('.status').html(this.options.strings.percentNotation.replace('%1', progress));
            }
        },

        // Helper for showing actions
        _setVisible: function(item, visisble)
        {
            visisble ? item.css('display','block') : item.hide();
        },

        // *** BEGIN PUBLIC INTERFACE ***

        // Returns the action button
        action: function()
        {
            return this.element.find('.otk-hyperlink');
        },

        // Returns the close button
        closeButton: function()
        {
            return this.element.find('.otk-close-button');
        },

        setIsMini: function(isMini)
        {
            if(isMini)
            {
                this.element.addClass("mini");
                this.element.find('.otk-hyperlink').attr("disabled", "disabled");
            }
            else
            {
                this.element.removeClass("mini");
                this.element.find('.otk-hyperlink').remove("disabled");
            }
        },

        // Toggles the toggle target between expanded and collapsed states
        toggle: function()
        {
            if(this.element.hasClass("expandable"))
            {
                this.options.expanded = !this.options.expanded;
                this._update();
            }
        },

        // Returns true if the toggle target is expanded, else false
        expanded: function()
        {
            return this.options.expanded;
        },

        // Sets the expanded state of the toggle target
        setExpanded: function(expanded)
        {
            this.options.expanded = expanded;
            this._update();
        },

        // Sets the expanded state of the toggle target
        setExpandable: function(expandable, toggleListId)
        {
            if(expandable)
            {
                this.element.addClass("expandable");
                this.element.attr('data-toggle-target', toggleListId);
            }
            else
            {
                this.element.removeClass("expandable");
                this.element.removeAttr('data-toggle-target');
            }
        },

        update: function()
        {
            this._update();
        }
    });

}(jQuery);

