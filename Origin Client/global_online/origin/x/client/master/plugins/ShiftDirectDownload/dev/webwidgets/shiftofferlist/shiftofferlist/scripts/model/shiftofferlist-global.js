;(function($, undefined){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }
if (!Origin.model) { Origin.model = {}; }
if (!Origin.util) { Origin.util = {}; }

Origin.util.badImageRender = function()
{	
	$('img.boxart').error(function()
    {
		$(this).attr('src', Origin.model.constants.DEFAULT_IMAGE_PATH);
	});
};

$(window.document).ready(function() 
{
    if(window.originCommon) 
    {
        window.originCommon.reloadConfig();
        Origin.views.myGamesOverrides.init();
        Origin.views.navbar.currentTab = "my-games";
    }
});

})(jQuery);