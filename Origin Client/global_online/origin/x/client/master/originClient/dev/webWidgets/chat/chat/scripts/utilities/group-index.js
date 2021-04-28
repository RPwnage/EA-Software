;(function(){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.utilities) { Origin.utilities = {}; }

// Given the group GUID return the index of the group
Origin.utilities.getGroupIndex = function (guid) {
    var index = 0;
    $.each(originSocial.groupList.groups, function(i, group)
    {
        if (group.groupGuid === guid) {
            index = i;
            return false; //break
        }
    });
    return index;
}


}());