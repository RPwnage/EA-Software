;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.utilities) { Origin.utilities = {}; }


// Add current platform information to views
Origin.utilities.currentPlatform = function()
{
      var userAgent = navigator.userAgent;

      if (userAgent.indexOf("Windows NT") !== -1)
      {
          return "PC";
      }
      else if (userAgent.indexOf("Macintosh") !== -1)
      {
          if(userAgent.indexOf("Origin") !== -1)
          {
              return "MAC-ORIGIN";
          }
          else
          {
              return "MAC";
          }
      }

      return null;
};

}(jQuery));