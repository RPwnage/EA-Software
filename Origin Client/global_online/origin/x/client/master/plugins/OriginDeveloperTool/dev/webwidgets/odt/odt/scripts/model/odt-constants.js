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
    
    this.DEFAULT_IMAGE_PATH = './odt/images/common/default-boxart.jpg';
    
    this.DEFAULT_STORE_LOCALE = 'en-ie/';
    
    this.DUMMY_ENTITLEMENT = {
        productId : '',
        contentId : '',
        expansionId : '',
        masterTitleId : '',
        boxartUrls : [],
        title : '',
        packageType : '',
        itemSubType : '',
        expansions : [],
        addons : [],
        testCode : null,
        releaseStatus : 'AVAILABLE',
        twitchBlacklisted : false,
        platformsSupported : [],
        localesSupported : []
    }
};

Origin.model.constants = new constants();

})(jQuery);