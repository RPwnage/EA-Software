;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.model) { Origin.model = {}; }


// Add current platform information to views
Origin.model.currentPlatform = function()
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

}(Zepto));