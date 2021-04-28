;(function($, undefined){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }
if (!Origin.model) { Origin.model = {}; }
if (!Origin.util) { Origin.util = {}; }

// Add current platform information to views
Origin.views.currentPlatform = function()
{
      var userAgent = navigator.userAgent;

      if (userAgent.indexOf("Windows NT") !== -1)
      {
          return "PC";
      }
      else if (userAgent.indexOf("Macintosh") !== -1)
      {
          return "MAC";
      }

      return null;
};

})(jQuery);