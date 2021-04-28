;(function($, undefined){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }
if (!Origin.model) { Origin.model = {}; }
if (!Origin.util) { Origin.util = {}; }

var constants = function()
{
	this.DEFAULT_MYGAMES_SORTING_DIRECTION = 1;
	this.DEFAULT_MYGAMES_SORT_BY = 'title';
    
    this.DEFAULT_SOFTWARE_BUILD_SORTING_DIRECTION = 1;
    this.DEFAULT_SOFTWARE_BUILD_SORT_BY = 'buildLiveDate';
    
    this.DEFAULT_IMAGE_PATH = './shiftofferlist/images/common/default-boxart.jpg';
    this.MAX_BUILDS_TO_SHOW = 50;
};

Origin.model.constants = new constants();

})(jQuery);
