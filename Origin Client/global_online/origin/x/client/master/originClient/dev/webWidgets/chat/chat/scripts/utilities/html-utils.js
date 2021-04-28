;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.utilities) { Origin.utilities = {}; }

// Escapes an HTML string to plain text
var escapeHtml = function (html) {    
    var chr = { '<': '&lt;', '>': '&gt;', '&': '&amp;' };
    html = html.replace(/[<>&]/g, function (a) { return chr[a]; });
    return html;    
};

// Must have at least two periods, such as www.ea.com
// URLs such as "amazon.com" (without www.) will need to have a http:// protocol appended in order to be linkified
var validHostRegex = /^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9])\.){2,}([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\-]*[A-Za-z0-9])$/;

// Special case to allow "ea.com" and "origin.com" as linkable URLs
var eaOriginUrlsRegex = /(ea.com|origin.com)$/;


var linkifyHtml = function (urlText) {
    console.log("linkify: " + urlText);
    var result = urlText;
    var parsed = URI.parse(urlText);
    
    var href = "";

    // The protocol is specified in the urlText
    if (!!parsed.protocol) {
        href = urlText;
    }
    else {
        // Else, this URI is specified without a protocol. Check that the path contains a valid hostname, according to our definition, above.
        if (!!parsed.path) {        
            var host = ( parsed.path.indexOf("/") > 0 ? parsed.path.slice(0, parsed.path.indexOf("/")) : parsed.path );
            if (validHostRegex.test(host) || eaOriginUrlsRegex.test(host)) {
                // Add http:// on to the link
                href = "http://" + urlText;
            }        
        }        
    }
    
    if (href.length > 0)
    {
        result = '<a class="origin-allow-copy-link user-provided" href="' + href + '">' + urlText + '</a>';
    }   
    return result;    
};

// Takes a tagname as a parameter, and unescapes just that tag
var unescapeTag = function(html, tagname) {

    var replaceOpenTag = function(match, attrs) {    
        var attributeRegex = /(\S+)=["']?((?:.(?!["']?\s+(?:\S+)=|["']))+.)["']?/g;       
        var attrArray = attrs.match(attributeRegex);
        
        var attrString = "";
        if (!!attrArray) {
            for(var i=0; i<attrArray.length; i++) {
                var attribute = attrArray[i];
                
                // Do not allow the "style" or "class" attribute on any unescaped tag
                if ( (attribute.toLowerCase().indexOf("style") < 0) && (attribute.toLowerCase().indexOf("class") < 0)) {
                    attrString += " " + attribute;
                }
            }
        }
        
        console.log("attrString: " + attrString);
        return "<" + tagname + attrString + ">";
    }
    
    var result = html;
    var openTagRegex = new RegExp("&lt;"+tagname+"(.*?)&gt;", "g");
    var closeTagRegex = new RegExp("&lt;\/"+tagname+"&gt;", "g");
    result = result.replace(openTagRegex, replaceOpenTag);
    result = result.replace(closeTagRegex, "</"+tagname+">");
    
    return result;    
};

// Function that escapes an HTML string, and then linkifies it
Origin.utilities.escapeAndLinkifyHtml = function (html) {

    var result = URI.withinString(escapeHtml(html), function(url) {
        var linkified = linkifyHtml(url);
        return linkified;
    }, {
        ignoreHtml: false,        
        // valid "scheme://" or "www.", or ending with "origin.com" or "ea.com"
        start: /\b(?:(https?:\/\/)|www\.|[A-Za-z0-9]*\.?origin\.com|[A-Za-z0-9]*\.?ea\.com)/gi,
        // trim trailing punctuation captured by end RegExp: add encoded symbols
        trim: /([`!()\[\]{};,'"<>]|&amp;|&quot;|&gt;|&lt;)+?.*$/
    });
    
    // Restore HTML text for marquees only, 'cause they are fun!
    result = unescapeTag(result, "marquee");
    
    return result;
};

}(jQuery));