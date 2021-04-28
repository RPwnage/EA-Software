;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }

var ImageProcessing = function()
{
    var imageWidth = imageProcessingJsHelper.scaledWidth;
    var imageHeight = imageProcessingJsHelper.scaledHeight;
    var imageScaledRatio = imageProcessingJsHelper.scaledRatio();
    var initialCropboxWidth = 231;
    var initialCropboxHeight = 326;
    
    $('#imgCropbox').attr('src', imageProcessingJsHelper.image);
    $('#imgCropbox').css({width: imageWidth + 'px', height: imageHeight + 'px'});
    $('#imgPreview').attr('src', imageProcessingJsHelper.image);
    
    var cropScaleRatio = Math.min(imageWidth / initialCropboxWidth, imageHeight / initialCropboxHeight);    
    initialCropboxHeight = initialCropboxHeight * cropScaleRatio;
    initialCropboxWidth = initialCropboxWidth * cropScaleRatio;

    var initialCropboxX = imageWidth / 2 - initialCropboxWidth / 2;
    var initialCropboxY = imageHeight / 2 - initialCropboxHeight / 2;
    
    var jcrop_api;
    var bounds, boundx, boundy;
    
    boundx = imageWidth;
    boundy = imageHeight;
    // Scale the minCropSize based on the image scale size
    // But never allow it to be smaller than 15px
    var minCropSize = Math.floor(15/imageScaledRatio);
    if (minCropSize < 15)
    {
        minCropSize = 15;
    }
    
    $('#imgCropbox').Jcrop({
        minSize: [minCropSize,minCropSize],
        onChange: showimgPreview,
        onSelect: showimgPreview,
        setSelect: [initialCropboxX, initialCropboxY, initialCropboxX + initialCropboxWidth, initialCropboxY + initialCropboxHeight],
        allowSelect: false,
        aspectRatio: 231/326
    },function(){
        jcrop_api = this;
        bounds = jcrop_api.getBounds();
        boundx = bounds[0];
        boundy = bounds[1];

        imageProcessingJsHelper.cropImageLoadComplete();
    });
    
    function showimgPreview(coords)
    {
        if (parseInt(coords.w) > 0)
        {
            imageProcessingJsHelper.setCroppedRect(coords.x, coords.y, coords.w, coords.h);
            
            var rx = 231 / coords.w;
            var ry = 326 / coords.h;
            
            $('#imgPreview').css({
                width: Math.round(rx * boundx) + 'px',
                height: Math.round(ry * boundy) + 'px',
                marginLeft: '-' + Math.round(rx * coords.x) + 'px',
                marginTop: '-' + Math.round(ry * coords.y) + 'px'
            });
        }
    };
}


// Expose to the world
Origin.views.imageProcessing = new ImageProcessing();

}(jQuery));
