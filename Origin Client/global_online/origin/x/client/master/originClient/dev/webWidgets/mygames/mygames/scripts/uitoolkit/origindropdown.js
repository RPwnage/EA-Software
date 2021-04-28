;(function($){
"use strict";

var OriginDropDown = function($select, settings)
{
    if (!$select || $select.length === 0)
    {
        console.log('OriginDropDown (contructor):: select element is a required parameter.');
        return;
    }
    var id = $select.attr('name');
    if (!id)
    {
        console.log('OriginDropDown (contructor):: select element must have a name attribute.');
        return;
    }
    
    this.id = id;
    
    var selectSelector = 'select[name="' + id + '"]';
    var dropdownSelector = '.dropdown[data-id="' + id + '"]';
    var dropdownOptionsSelector = '.dropdown-options[data-id="' + id + '"]';
    
    var select = $select[0];
    var selectedValue = $select.val();
    
    var dropdownLabelHtml = '';
    var dropdownOptionsHtml = '<div class="dropdown-options" data-id="' + id + '"><ul>';
    $select.find('> option').each(function(idx, option)
    {
        var label = option.text;
        var value = option.value;
        
        // Build the widhet option
        dropdownOptionsHtml += '<li class="dropdown-option';
        if (value === selectedValue)
        {
            dropdownOptionsHtml += ' selected';
            
            // Set the label
            dropdownLabelHtml = '<span class="label">' + label + '</span>';
        }
        dropdownOptionsHtml += '" data-value="' + value + '"><span>' + label + '</span></li>';
    });
    dropdownOptionsHtml += '</ul></div>';
    
    var $dropdown = $('<span class="dropdown" data-id="' + id + '">' + dropdownLabelHtml + '</span>');
    $select.hide().after($dropdown);
    
    var $dropdownOptions = $(dropdownOptionsHtml).appendTo(document.body);
    
    $dropdownOptions
        .hide()
        .on('mouseover mouseout', function(evt)
        {
            evt.stopImmediatePropagation();
        })
        .find('> ul > li > span')
            .on('click', function(evt)
            {
                evt.stopImmediatePropagation();
                
                var $li = $(this).parent();
                if ($li.hasClass('disabled')) { return; }
                
                var value = $li.attr('data-value');
                $(selectSelector)
                    .val(value)
                    .change();
            });
    
    $dropdown
        .on('click', function(evt)
        {
            evt.stopImmediatePropagation();

            if ($(this).hasClass('open'))
            {
                OriginDropDown.instances[id].hideOptionDropDown();
            }
            else
            {
                $(this).addClass('open');
            
                $('.dropdown-options').hide();

                var $dropdownOptions = $(dropdownOptionsSelector);

                if ($dropdownOptions.is(':animated'))
                {
                    // Just animate in; we're already positioned correctly
                    $dropdownOptions.fadeIn('fast');
                }
                else
                {
                    // Position us and fade in
                    $dropdownOptions
                        .fadeIn('fast')
                        .position(
                        {
                            of: $dropdown,
                            my: 'left top',
                            at: 'left bottom',
                            offset: '0 -1px',
                            collision: 'none none'
                        });
                }
            }
        });
        
    // Bind to the option change event
    $select.on('change', function(evt)
    {
        var $select = $(this);
        
        // Set the new label
        $dropdown
            .find('> span')
                .html($select.find('> option:selected').html());
        
        var $dropdownOptions = $(dropdownOptionsSelector);
        
        // Deselect previous option
        $dropdownOptions
            .find('> li.selected')
                .removeClass('selected');
        
        // Select the new option
        $dropdownOptions
            .find('> li[data-value="' + $select.val() + '"]')
                .addClass('selected');
        
        // Hide our options
        OriginDropDown.instances[id].hideOptionDropDown();
    });
    
    this.hideOptionDropDown = function()
    {
        $dropdown.removeClass('open');
        $dropdownOptions.fadeOut('fast');
    };
    
    OriginDropDown.init();
    
    OriginDropDown.instances[id] = this;
};

OriginDropDown.prototype.enableOption = function(value)
{
    this.getOption$Elm(value).removeClass("disabled");
};

OriginDropDown.prototype.disableOption = function(value)
{
    this.getOption$Elm(value).addClass("disabled");
};

OriginDropDown.prototype.getOption$Elm = function(value)
{
    return $('.dropdown-options[data-id="' + this.id + '"] > ul > li[data-value="' + value + '"]');
};

OriginDropDown.prototype.get$Elm = function()
{
    return $('.dropdown[data-id="' + this.id + '"]');
};

OriginDropDown.prototype.getScrollbarWidth = function()
{
    if (typeof(this._scrollbarWidth) === 'number') { return this._scrollbarWidth; }
    
    // Determine the real scrollbar width
    var $testContainer = $('<div style="position:absolute;left:-200px;top:0;width:200px;height:150px;overflow:hidden;"><p style="width:100%;height:200px;"></p></div>').appendTo(document.body);
    var $testContent = $testContainer.find('p:first');
    var w1 = $testContent[0].offsetWidth;
    $testContainer.css('overflow', 'scroll');
    var w2 = $testContent[0].offsetWidth;
    if (w1 === w2) { w2 = $testContainer[0].clientWidth; }
    $testContainer.remove();
    
    this._scrollbarWidth = (w1 - w2);
    
    return this._scrollbarWidth;
};

OriginDropDown.initialized = false;

OriginDropDown.instances = {};

OriginDropDown.init = function()
{
    if (OriginDropDown.initialized) { return; }
    OriginDropDown.initialized = true;
    
    $(document.body).on('click', function(evt)
    {
        // Hide our option drop down
        for(var id in OriginDropDown.instances) 
        {
            OriginDropDown.instances[id].hideOptionDropDown();
        }
    });
};

OriginDropDown.get = function(id)
{
    return OriginDropDown.instances[id];
};

// Expose to window
window.OriginDropDown = OriginDropDown;

})(jQuery);
