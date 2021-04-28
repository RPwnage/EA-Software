;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }


var Debug = function()
{
        // TODO REMOVE
        // TODO REMOVE
        var curEnt = null;
        var num = entitlementManager.topLevelEntitlements.length;
        //var num = 1;
        for(var i = 0; i < num; i++)
        {
            curEnt = entitlementManager.topLevelEntitlements[i];
            curEnt.startDownload();
            for(var iAddon = 0; iAddon < curEnt.addons.length; iAddon++)
            {
                curEnt.addons[iAddon].startDownload();
            }
            
            for(var iEx = 0; iEx < curEnt.expansions.length; iEx++)
            {
                curEnt.expansions[iEx].startDownload();
            }
        }
	
	$("body").css({
		width: 'initial',
		'min-height': 'initial',
		'max-height': 'initial'
	});
};

// Expose to the world
window.origindebug = new Debug();

})(jQuery);