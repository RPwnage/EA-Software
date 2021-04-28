;(function($){
"use strict";

/**
 * CLASS: Connector
 */
var Connector = function()
{
    this.connections = [];    
};
Connector.prototype.connect = function(callback)
{
    this.connections.push(callback);
};
Connector.prototype.disconnect = function(callback)
{
    this.connections = this.connections.filter(function(other)
    {
        return callback !== other;
    });
};
Connector.prototype.trigger = function()
{
    var args = Array.prototype.slice.call(arguments);
    this.connections.forEach(function(callback)
    {
        callback.apply(null, args);
    });
};

/**
 * CLASS: ImageProcessingJsHelper
 */
var ImageProcessingJsHelper = function()
{
    this._image = 'imageprocessing/images/default-boxart.jpg';
    this._scaledWidth = 0;
    this._scaledHeight = 0;
    this._maxHeight = 326;
    this._scaledRatio = 1;
};
ImageProcessingJsHelper.prototype.__defineGetter__('image', function()
{
    return this._image;
});
ImageProcessingJsHelper.prototype.__defineGetter__('scaledWidth', function()
{
    return this._scaledWidth;
});
ImageProcessingJsHelper.prototype.__defineGetter__('scaledHeight', function()
{
    return this._scaledHeight;
});
ImageProcessingJsHelper.prototype.__defineGetter__('maxHeight', function()
{
    return this._maxHeight;
});

ImageProcessingJsHelper.prototype.__defineGetter__('scaledRatio', function()
{
    return this._scaledRatio;
});

ImageProcessingJsHelper.prototype.setCroppedRect = function(x, y, width, height)
{
    console.log('x = '+x+' y = '+y+' w = '+width+' h = '+height);
};

window.imageProcessingJsHelper = new ImageProcessingJsHelper;

})(jQuery);