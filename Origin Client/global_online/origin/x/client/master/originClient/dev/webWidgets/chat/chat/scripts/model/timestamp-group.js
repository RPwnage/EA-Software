(function($){
"use strict";

var TimestampGroup = function(initialEvent)
{
    var lastTime, timestampThresholdMs;

    // Consider events more than timestampThresholdMs apart as different 
    // timestamp groups
    timestampThresholdMs = 60 * 1000;

    lastTime = initialEvent.time.getTime();

    // Returns false if this event is part of a different timestamp group
    this.continueWith = function(newEvent)
    {
        var newTime = newEvent.time.getTime();

        if ((newTime - lastTime) > timestampThresholdMs)
        {
            // The event is too old
            return false;
        }
        
        // Keep the dialogue alive
        lastTime = newTime;
        return true;
    };
};

if (!window.Origin) { window.Origin = {}; }
if (!window.Origin.model) { window.Origin.model = {}; }
window.Origin.model.TimestampGroup = TimestampGroup;

}(Zepto));
