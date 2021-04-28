
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

    $.otkContactGroupCreate = function (element, options)
    {
        var $element = $(element), $html;
        $element.addClass("otk-contact-group");
        $html = $(
            '<div class="otk-expand-collapse-indicator"></div>' +
            '<span class="label"></span>' +
            '<span class="otk-count-bubble"></span>'
            );
        $element.append($html);
        $element.otkContactGroup(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkContactGroup",
    {
        // Default options.
        options:
        {
            label: "Group Name",
            count: "0",
            expanded: true
        },
 
        _create: function()
        {
            //
            // See if there are already values for our defaults
            //
            var labelValue = this.element.find('.label').text(),
                countValue = this.element.find('.otk-count-bubble').text(),
                expandedValue = this.element.hasClass("expanded");

            if (labelValue)
            {
                this.options.label = labelValue;
            }
            if (countValue)
            {
                this.options.count = countValue;
            }
            if (expandedValue)
            {
                this.options.expanded = expandedValue;
            }

            //
            // Toggle expanded/collapsed on click
            //
            this.element.on('click', function()
            {
                this.toggle();
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
            var toggleSelector;

            // Set the label
            this.element.find('.label').text(this.options.label);

            // Set the count bubble
            this.element.find('.otk-count-bubble').text(this.options.count);

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
        },

        // *** BEGIN PUBLIC INTERFACE ***

        // Toggles the toggle target between expanded and collapsed states
        toggle: function()
        {
            this.options.expanded = !this.options.expanded;
            this._update();
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

        // Returns the count value
        count: function()
        {
            return this.options.count;
        },

        // Sets the count value
        setCount: function(count)
        {
            this.options.count = count;
            this._update();
        },

        // Returns the label value
        label: function()
        {
            return this.options.label;
        },

        // Sets the label value
        setLabel: function(label)
        {
            this.options.label = label;
            this._update();
        }

    });

}(jQuery);

