;(function($){
"use strict";

var StandardProgressBar = function($elm)
{
    var progressHtml = '<span class="overlay"></span><progress min="0" max="100"></progress>';
    var fillHtml = '<span class="fill"><span></span></span>';
    var labelHtml = '<span class="label"></span>';
    var id;
    var $progress;
    
    if (!$elm)
    {
        id = StandardProgressBar.genId();
        $elm = $('<div data-id ="' + id + '" class="origin-progressbar">' + progressHtml + fillHtml + labelHtml + '</div>');
        $progress = $elm.find('> progress');
        // Need a way to get element or append to a specified parent
    }
    else
    {
        if (!(id = $elm.attr('id')))
        {
            id = StandardProgressBar.genId();
        }
        $elm.attr('data-id', id);
        
        $progress = $elm.find('> progress');
        
        if (!$elm.hasClass('origin-progressbar'))
        {
            $elm.addClass('origin-progressbar');
        }
        if ($progress.length === 0)
        {
            $progress = $(progressHtml).appendTo($elm);
        }
        if ($elm.find('> span.fill').length === 0)
        {
            $progress.after(fillHtml);
        }
        if ($elm.find('> span.label').length === 0)
        {
            $elm.append(labelHtml);
        }
    }
    
    var isIntermediate = $elm.hasClass('indeterminate');
    var val = (!isIntermediate)? (($progress[0])? Number($progress[0].value) : 0) : 100;
    
    this.id = id;
    
    StandardProgressBar.instances[id] = this;
    
    this.setProgress(val/100, true);
};
StandardProgressBar.counter = 0;
StandardProgressBar.instances = {};
StandardProgressBar.genId = function()
{
    return 'progressbar-' + ++StandardProgressBar.counter;
};
StandardProgressBar.get = function(id)
{
    return StandardProgressBar.instances[id];
};
StandardProgressBar.prototype.get$Elm = function()
{
    return $('.origin-progressbar[data-id="' + this.id + '"]');
};
StandardProgressBar.prototype.setLabel = function(text)
{
    return this.get$Elm().find('> .label').text(text);
};
StandardProgressBar.prototype.setProgress = function(progress, init)
{
    var progress = Math.floor(progress * 100);
    var $elm = this.get$Elm();
    
    if(progress >= 100)
    {
        this.setComplete();
    }
    else
    {
        if (!init)
        {
            $elm.removeClass('indeterminate')
                .removeClass('completed')
                .find('> progress').attr('value', progress);
        }
        $elm.find('> .fill > span').width(progress + '%');
    }

    return $elm;
};
StandardProgressBar.prototype.progress = function()
{
    return this.get$Elm().find('> progress').attr('value');
};
StandardProgressBar.prototype.setIndeterminate = function()
{
    var $elm = this.get$Elm().addClass('indeterminate');
    $elm.removeClass('completed');
    $elm.find('> progress').removeAttr('value');
    $elm.find('> .fill > span').width('100%');
    return $elm;
};
StandardProgressBar.prototype.setComplete = function()
{
    var $elm = this.get$Elm().addClass('completed');
    $elm.removeClass('indeterminate');
    $elm.find('> progress').removeAttr('value');
    $elm.find('> .fill > span').width('0%');
    return $elm;
};
StandardProgressBar.prototype.pause = function()
{
    return this.get$Elm().addClass('paused');
};
StandardProgressBar.prototype.unpause = function()
{
    return this.get$Elm().removeClass('paused');
};
StandardProgressBar.prototype.show = function()
{
    return this.get$Elm().show();
};
StandardProgressBar.prototype.hide = function() 
{
    return this.get$Elm().hide();
};

// Expose to window
window.StandardProgressBar = StandardProgressBar;

})(jQuery);
