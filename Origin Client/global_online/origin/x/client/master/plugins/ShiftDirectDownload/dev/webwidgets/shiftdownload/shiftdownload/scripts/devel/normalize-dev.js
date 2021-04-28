;(function($, undefined){
"use strict";

if (window.dcmtDownload) { return; }
if (window.dateFormat) { return; }

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
 * CLASS: DcmtDownloadProxy
 */
var DcmtDownloadProxy = function()
{
    this.downloadProgress = new Connector;
    this.started = new Connector;
    this.finished = new Connector;
    this.error = new Connector;
    this.canceled = new Connector;
    this.deliveryRequested = new Connector;
    
    this._totalBytes = 10000000;
    this._maxBps = 1000000;
    this._maxTimeRemaining = 60*60*24;
    this._timeUntilComplete = 5000;
    this._downloadInterval = 0;
    this._buildId = '8b4c11-9325bc-abcde8';
    
    this.fileName = function()
    {
        return 'Production 2 DIP4_0.zip';
    };
    
    this.cancel = function()
    {
        console.log('Cancel clicked');
        this.canceled.trigger(this._buildId);
    };
    
    this.buildId = function()
    {
        return this._buildId;
    };
    
    // This emulates state transitions
    // 1. Initial state (generating download URL)
    // 2. Delivery requested
    // 3. Download started
    // 4. Download progressed (multiple)
    // 5. Download complete
    window.setTimeout($.proxy(function()
    {
        this.deliveryRequested.trigger(this._buildId);
        
        window.setTimeout($.proxy(function()
        {
            this.started.trigger(this._buildId);
            
            this._downloadInterval = window.setInterval($.proxy(function()
            {
                var bytesDownloaded = Math.random() * this._totalBytes;
                var totalBytes = this._totalBytes;
                var bps = Math.random() * this._maxBps;
                var timeRemaining = Math.random() * this._maxTimeRemaining;
                
                this.downloadProgress.trigger(bytesDownloaded, totalBytes, bps, timeRemaining);
            }, this), 1000);
            
            window.setTimeout($.proxy(function()
            {
                window.clearInterval(this._downloadInterval);
                
                var bytesDownloaded = this._totalBytes;
                var totalBytes = this._totalBytes;
                var bps = -1;
                var timeRemaining = 0;
                
                this.downloadProgress.trigger(bytesDownloaded, totalBytes, bps, timeRemaining);
                this.finished.trigger(this._buildId);
            }, this), this._timeUntilComplete);
        }, this), 3000);
    }, this), 2000);
}

window.dcmtDownload = new DcmtDownloadProxy;

/**
 * CLASS: DateFormatter
 * Helper for formatting localized dates and times
 */
var DateFormatter = function()
{
};

var HOUR_IN_SECONDS = 60 * 60;
var DAY_IN_SECONDS = HOUR_IN_SECONDS * 24;


DateFormatter.prototype.formatShortInterval = function(seconds)
{
    var hour = seconds / 3600;
    var minute = (seconds % 3600) / 60;
    var second = seconds % 60;
    
    return Math.round(hour)+':'+Math.round(minute)+':'+Math.round(second);
};

// Formats the passed interval in seconds. The result is in the form "1 day", "2 hours", "less than an hour", etc
DateFormatter.prototype.formatInterval = function(seconds)
{
    if (seconds > DAY_IN_SECONDS)
    {
        var days = seconds / DAY_IN_SECONDS;
        var daysRounded = Math.floor(days);
        return daysRounded + ' days'; // + this.formatInterval(days - daysRounded)
    }
    else if (seconds === DAY_IN_SECONDS)
    {
        return '1 day';
    }
    else if (seconds > HOUR_IN_SECONDS)
    {
        return Math.ceil(seconds / HOUR_IN_SECONDS) + ' hours';
    }
    else if (seconds === HOUR_IN_SECONDS)
    {
        return '1 hour';
    }
    else
    {
        return 'less than an hour';
    }
};

// Formats the date component of the passed Date object as a long localized string. This is typically includes the full month name eg October 12, 2012.
DateFormatter.prototype.formatLongDate = function(date)
{
    return date.format('mmmm dS, yyyy');
};

// Formats the passed Date object as a localized datetime string using the long date format.
DateFormatter.prototype.formatLongDateTime = function(date)
{
    return date.format('mmmm dS, yyyy h:MM tt');
};

// Formats the date component of the passed Date object as a short localized string. This is typically a short numerical representation eg 12/10/2012.
DateFormatter.prototype.formatShortDate = function(date)
{
    return date.format('mm/d/yyyy');
};

// Formats the passed Date object as a localized datetime string using the short date format.
DateFormatter.prototype.formatShortDateTime = function(date)
{
    return date.format('mm/d/yyyy h:MM tt');
};

// Formats the time component of the passed Date object as a localized string
DateFormatter.prototype.formatTime = function(date)
{
    if (includeSeconds)
    {
        return date.format('h:MM:ss tt');
    }
    else
    {
        return date.format('h:MM tt');
    }
};

// Formats the passed Date object as a localized datetime string using the long date format and includes the day of the week.
DateFormatter.prototype.formatLongDateTimeWithWeekday = function(date)
{
    return date.format('dddd mmmm dS, yyyy h:MM tt');
};

// Expose to the world
window.dateFormat = new DateFormatter();

})(jQuery);
