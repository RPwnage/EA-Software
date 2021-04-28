;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.utilities) { Origin.utilities = {}; }


Origin.utilities.hasRealName = function(contact) {
    var hasRealName = ( (!!contact.realName) && (  (!!contact.realName.firstName) ||  (!!contact.realName.lastName) ));
    return hasRealName;
}

Origin.utilities.getRealName = function(contact) {
    var name = "";
    if (!!contact.realName) {
        if (!!contact.realName.firstName) name = contact.realName.firstName;
        if (!!contact.realName.lastName) name = name + (name.length > 0 ? " " : "") + contact.realName.lastName;
    } 
    return name;
}


}(jQuery));