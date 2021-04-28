
!function ($) {

    "use strict";

    // OPTIONAL HTML CONTENT BUILDER

    $.otkContactCreate = function (element, options)
    {
        var $element = $(element), $html;
        $element.addClass("otk-contact");
        $html = $(
            '<div class="otk-presence-indicator"></div>' +
            '<img class="otk-user-avatar">' + 
            '<div class="name">' +
                '<span class="nickname"></span>' +
                '<span class="realname"></span>' +
            '</div>' +
            '<span class="presence"></span>' +
            '<button class="otk-hover-context-menu-button"></button>' 
            );
        $element.append($html);
        $element.otkContact(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkContact",
    {
        // Default options.
        options:
        {
            nicknameLabel: "Username",
            realnameLabel: "Real Name",
            presenceLabel: "Presence",
            presenceIndicator: "unavailable",
            avatarPath: ""
        },
 
        _create: function()
        {
            //
            // See if there are already values for our defaults
            //
            var $presenceIndicator = this.element.find('.otk-presence-indicator'),
                $avatar = this.element.find('.otk-user-avatar'),
                nicknameLabelValue = this.element.find('.nickname').text(),
                realnameLabelValue = this.element.find('.realname').text(),
                presenceLabelValue = this.element.find('.presence').text(),
                presenceIndicatorValue = $presenceIndicator.is('[data-presence]') ? $presenceIndicator.attr('data-presence') : "",
                avatarPathValue = $avatar.is('[src]') ? $avatar.attr('src') : "";

            if (nicknameLabelValue)
            {
                this.options.nicknameLabel = nicknameLabelValue;
            }
            if (realnameLabelValue)
            {
                this.options.realnameLabel = realnameLabelValue;
            }
            if (presenceLabelValue)
            {
                this.options.presenceLabel = presenceLabelValue;
            }
            if (presenceIndicatorValue)
            {
                this.options.presenceIndicator = presenceIndicatorValue;
            }
            if (avatarPathValue)
            {
                this.options.avatarPath = avatarPathValue;
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
            // Can we cache these lookups somehow??

            var $presenceArea = this.element.find('.presence');

            this.element.find('.nickname').text(this.options.nicknameLabel);

            this.element.removeClass("has-real-name");
            if (this.element.has('.realname').length) // realname element is optional
            {
                this.element.find('.realname').text(this.options.realnameLabel);

                if (this.options.realnameLabel)
                {
                    this.element.addClass("has-real-name");
                }
            }

            if ($presenceArea.hasClass('action-links') === false)
            {
                this.element.find('.presence').text(this.options.presenceLabel);
            }

            this.element.find('.otk-presence-indicator').attr("data-presence", this.options.presenceIndicator);

            if (this.options.avatarPath.length)
            {
                this.element.find('.otk-user-avatar').attr("src", this.options.avatarPath);
            }
            else
            {
                this.element.find('.otk-user-avatar').removeAttr("src");
            }
        },

        // *** BEGIN PUBLIC INTERFACE ***

        // Adds an action link
        actionLink: function(label, link)
        {
            var $presenceArea = this.element.find('.presence');

            if ($presenceArea.hasClass('action-links') === false)
            {
                $presenceArea.empty();
                $presenceArea.addClass('action-links');
            }

            // Add the action link
            $presenceArea.append($('<a class="otk-hyperlink">').attr("href", link).text(label));
        },

        // Removes all action links
        removeActionLinks: function()
        {
            var $presenceArea = this.element.find('.presence');
            if ($presenceArea.hasClass('action-links'))
            {
                $presenceArea.removeClass('action-links');
                $presenceArea.empty();
                $presenceArea.text(this.options.presenceLabel);
            }
        },

        // Returns the nicknameLabel value
        nicknameLabel: function()
        {
            return this.options.nicknameLabel;
        },

        // Sets the nicknameLabel value
        setNicknameLabel: function(label)
        {
            this.options.nicknameLabel = label;
            this._update();
        },

        // Returns the realnameLabel value
        realnameLabel: function()
        {
            return this.options.realnameLabel;
        },

        // Sets the realnameLabel value
        setRealnameLabel: function(label)
        {
            this.options.realnameLabel = label;
            this._update();
        },

        // Returns the presenceLabel value
        presenceLabel: function()
        {
            return this.options.presenceLabel;
        },

        // Sets the presenceLabel value
        setPresenceLabel: function(label)
        {
            this.options.presenceLabel = label;
            this._update();
        },

        // Returns the presenceIndicator value
        presenceIndicator: function()
        {
            return this.options.presenceIndicator;
        },

        // Sets the presenceIndicator value
        setPresenceIndicator: function(indicatorValue)
        {
            this.options.presenceIndicator = indicatorValue;
            this._update();
        },

        // Returns the avatarPath value
        avatarPath: function()
        {
            return this.options.avatarPath;
        },

        // Sets the avatarPath value
        setAvatarPath: function(path)
        {
            this.options.avatarPath = path;
            this._update();
        }

    });

}(jQuery);

