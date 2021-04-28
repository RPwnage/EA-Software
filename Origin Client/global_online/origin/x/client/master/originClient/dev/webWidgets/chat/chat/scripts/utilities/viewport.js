;(function(){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.utilities) { Origin.utilities = {}; }

Origin.utilities.isElementAboveViewport = function(el) {
    var rect = el.getBoundingClientRect();
    return (rect.bottom > 0);
};    

Origin.utilities.isElementInViewport = function(el) {
    var rect = el.getBoundingClientRect();
    return (rect.bottom > 0 && rect.bottom <= (window.innerHeight || document.documentElement.clientHeight));
};    

Origin.utilities.isElementBelowViewport = function(el) {
    var rect = el.getBoundingClientRect();
    return (rect.bottom > (window.innerHeight || document.documentElement.clientHeight));
};    
    
}());