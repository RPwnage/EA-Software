;(function($){
"use strict";

/* The EA CDN frequently contains missing files and it can take us a while to 
 * get a 404 back indicating they don't exist. The shows up as a delay to the 
 * user while we fall back to the next image.
 *
 * Ideally all of this would be handled by the CDN giving cacheable 404s but
 * it's unlikely to happen
 */

if (!window.Origin) { window.Origin = {}; }

// Cache for 24 hours
var BAD_IMAGE_CACHE_MILLISECONDS = 24 * 60 * 60 * 1000;

function cacheKeyForUrl(url)
{
    return "badimage-" + url;
}

function isDataUri(url)
{
    return url.indexOf("data:") === 0;
}
    
// Stub Web Storage implementation
// This only implements the parts of Web Storage we use
var TemporaryStorage = function()
{
    this._items = {};
};

TemporaryStorage.prototype.setItem = function(key, value)
{
    this._items[key] = value;
};

TemporaryStorage.prototype.getItem = function(key)
{
    if (this._items.hasOwnProperty(key))
    {
        return this._items[key];
    }

    return null;
};

TemporaryStorage.prototype.removeItem = function(key)
{
    delete this._items[key];
};

var BadImageCache = function()
{
    onlineStatus.onlineStateChanged.connect(BadImageCache.prototype.selectStorageBackend);
    this.selectStorageBackend();
};

BadImageCache.prototype.selectStorageBackend = function()
{
    if (onlineStatus.onlineState)
    {
        // Use real session storage
        // If we fail to load a URL while online we should remember it 
        this.storage = window.sessionStorage;
    }
    else
    {
        // Use temporary storage
        // This is so we don't remember bad URLs when we come online
        // Create a new one every time we go offline as we could've cached new images while online
        this.storage = new TemporaryStorage;
    }
};

BadImageCache.prototype.shouldTryUrl = function(url)
{
    if (isDataUri(url))
    {
        // Always try data URIs
        return true;
    }

    var cacheKey = cacheKeyForUrl(url);
    var lastCheckTime = this.storage.getItem(cacheKey);

    if (lastCheckTime === null)
    {
        // We don't know about this URL
        return true;
    }
    else if ((Date.now() - parseInt(lastCheckTime, 10)) > BAD_IMAGE_CACHE_MILLISECONDS)
    {
        // This was bad but our cache time has expired
        // Try again
        this.storage.removeItem(cacheKey);
        return true;
    }

    // Bad!
    return false;
};

BadImageCache.prototype.urlFailed = function(url)
{
    if (isDataUri(url))
    {
        // Ignore data: URIs
        return;
    }

    var cacheKey = cacheKeyForUrl(url);
    this.storage.setItem(cacheKey, Date.now().toString());
};

BadImageCache.prototype.urlSucceeded = function(url)
{
    if (isDataUri(url))
    {
        // Ignore data: URIs
        return;
    }

    var cacheKey = cacheKeyForUrl(url);
    this.storage.removeItem(cacheKey);
};

Origin.BadImageCache = new BadImageCache();

})(jQuery);
