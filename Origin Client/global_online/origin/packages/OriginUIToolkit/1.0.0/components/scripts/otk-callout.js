!function ($) {

    "use strict";

    // OPTIONAL HTML CONTENT BUILDER

    $.otkCalloutCreate = function (options)
    {
        var $element = $('<div></div>'), $html;
        $element.addClass("otk-callout");
        $html = $(
            '<header>' +
                '<span class="content-line">' +
                    '<img class="icon"></img>' +
                    '<div class="titles-container">' +
                        '<h5 class="new-title"></h5>' +
                        '<h5 class="title"></h5>' +
                    '</div>' +
                    '<button class="otk-close-button small"></button>' +
                '</span>' +
            '</header>'
            );
        $element.append($html);
        options.bindedElement.after($element);

        $element.otkCallout(options);
        return $element;
    };

    // COMPONENT PLUGIN

    $.widget( "origin.otkCallout",
    {
        // Default options.
        options:
        {
            newTitle: "",
            title: "",
            content: [/*{imageUrl:'', text:''}*/],
            bindedElement: null,
            xOffset: 0,
            yOffset: 0
        },
 
        _create: function()
        {
            //
            // See if there are already values for our defaults
            //

            var newTitleValue = this.element.find('.new-title').text(),
                titleValue = this.element.find('.title').text();

            if (newTitleValue && !(this.options.newTitle))
            {
                this.options.newTitle = newTitleValue;
            }
            
            if (titleValue && !(this.options.title))
            {
                this.options.title = titleValue;
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
            
            this.options.bindedElement.click(function() {
                this._finished();
            }.bind(this));
            
            this.element.find('.otk-close-button').click(function() {
                this._finished();
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
            var section = $('<section></section>'),
                contentLineContainerTemplate = $('<span class="content-line"></span>'),
                contentImageTemplate = $('<div class="content-img"><img></img></div>'),
                contentTextTemplate = $('<p class="content-text"></p>');

            this.element.find('.new-title').text(this.options.newTitle);
            this.element.find('.title').text(this.options.title);
            
            // Remove prior content
            if(this.element.find('.content'))
            {
                this.element.find('.content').remove();
            }
            
            if(this.options.content.length !== 0)
            {
                // Set the content
                for(var i = 0, num = this.options.content.length; i < num; i++)
                {
                    var contentLine = contentLineContainerTemplate.clone();
                    
                    if(this.options.content[i].imageUrl)
                    {
                        contentLine.append(contentImageTemplate.clone());
                        contentLine.find('img').attr('src', this.options.content[i].imageUrl)
                    }
                    
                    if(this.options.content[i].text)
                    {
                        contentLine.append(contentTextTemplate.clone().text(this.options.content[i].text));
                    }
    
                    section.append(contentLine);
                }
                this.element.find('header').after(section);
            }
            
            this._adjustForScreenPosition();
        },
        
        _adjustForScreenPosition: function()
        {
            this.element.removeClass("top right bottom left");
            var bindedPosition = this.options.bindedElement.offset(),
                centerOfParent, elementCenteredToParentX;

            bindedPosition.left += this.options.xOffset;
            bindedPosition.top += this.options.yOffset;

            centerOfParent = bindedPosition.left + (parseInt(this.options.bindedElement.width()) / 2),
            elementCenteredToParentX = centerOfParent - (parseInt(this.element.width()) / 2);

            // Center us by default onto the binded element
            this.element.css({
                top : (bindedPosition.top + 15) + 'px',
                left : elementCenteredToParentX + 'px'
            });
            
            // If we are off screen right
            // body width < Right point
            if(parseInt($('body').width()) < (elementCenteredToParentX + parseInt(this.element.width())))
            {
                // Move callout back onto the screen
                this.element.css({
                    right : (parseInt($(window).width()) - centerOfParent) + 'px'
                });
                this.element.addClass("right");
            }
            // If we are off screen left
            else if(elementCenteredToParentX < 0)
            {
                // Move callout back onto the screen
                this.element.css({
                    left : ((parseInt(this.options.bindedElement.width()) / 2) + bindedPosition.left) + 'px'
                });
                this.element.addClass("left");
            }
            
            // Are we off the bottom of the screen?
            // If so, position callout above the binded element
            if(this.element.offset().top >= $('body').height())
            {
                this.element.css({
                    top : (bindedPosition.top - this.element.outerHeight() - 22) + 'px'
                });
                this.element.addClass("bottom");
            }
            else
            {
                this.element.addClass("top");
            }
        },
        
        _finished: function()
        {
            this.element.hide();
            this.element.trigger("select");
        }
    });

}(jQuery);


