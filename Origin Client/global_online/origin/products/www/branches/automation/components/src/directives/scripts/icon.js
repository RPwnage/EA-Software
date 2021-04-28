
/**
 * @file /scripts/icon.js
 */
(function(){
    'use strict';


    /**
     * A list of icons that can be applied
     * @readonly
     * @enum {string}
     */
    var IconEnumeration = {
        "language": "otkicon-language",
        "newrelease": "otkicon-newrelease",
        "video": "otkicon-video",
        "preorder": "otkicon-preorder",
        "earlyaccess": "otkicon-earlyaccess",
        "expiring": "otkicon-expiring",
        "expired": "otkicon-expired",
        "coop": "otkicon-coop",
        "ggg": "otkicon-ggg",
        "progressiveinstall": "otkicon-progressiveinstall",
        "twitch": "otkicon-twitch",
        "gametime": "otkicon-gametime",
        "offlinemode": "otkicon-offlinemode",
        "modding": "otkicon-modding",
        "leaderboard": "otkicon-leaderboard",
        "app": "otkicon-app",
        "splitscreen": "otkicon-splitscreen",
        "dolby": "otkicon-dolby",
        "thirdperson": "otkicon-thirdperson",
        "touch": "otkicon-touch",
        "crossplatform": "otkicon-crossplatform",
        "fourK": "otkicon-4k",
        "directx": "otkicon-directx",
        "firstperson": "otkicon-firstperson",
        "challenge": "otkicon-challenge",
        "circle": "otkicon-play-with-circle",
        "noCircle": "otkicon-warning-no-circle",
        "maximize": "otkicon-maximize",
        "multiplayer": "otkicon-multiplayer",
        "help": "otkicon-help",
        "info": "otkicon-info",
        "newIcon": "otkicon-new",
        "filter": "otkicon-filter",
        "timer": "otkicon-timer",
        "article": "otkicon-article",
        "sortup": "otkicon-sortup",
        "sortdown": "otkicon-sortdown",
        "lockclosed": "otkicon-lockclosed",
        "lockopen": "otkicon-lockopen",
        "origintext": "otkicon-origintext",
        "pin": "otkicon-pin",
        "battery": "otkicon-battery",
        "batterycharged": "otkicon-batterycharged",
        "incomingcall": "otkicon-incomingcall",
        "speakermuted": "otkicon-speakermuted",
        "speakinglevels": "otkicon-speakinglevels",
        "broadcasting": "otkicon-broadcasting",
        "speaker": "otkicon-speaker",
        "accesstext": "otkicon-accesstext",
        "youtube": "otkicon-youtube",
        "settings": "otkicon-settings",
        "close": "otkicon-close",
        "rightarrow": "otkicon-rightarrow",
        "leftarrow": "otkicon-leftarrow",
        "add": "otkicon-add",
        "search": "otkicon-search",
        "originlogo": "otkicon-originlogo",
        "facebook": "otkicon-facebook",
        "refresh": "otkicon-refresh",
        "downarrow": "otkicon-downarrow",
        "menu": "otkicon-menu",
        "twitter": "otkicon-twitter",
        "checkcircle": "otkicon-checkcircle",
        "check": "otkicon-check",
        "home": "otkicon-home",
        "store": "otkicon-store",
        "gamelibrary": "otkicon-gamelibrary",
        "profile": "otkicon-profile",
        "download": "otkicon-download",
        "downloadnegative": "otkicon-downloadnegative",
        "person": "otkicon-person",
        "leftarrowcircle": "otkicon-leftarrowcircle",
        "rightarrowcircle": "otkicon-rightarrowcircle",
        "expandarrow": "otkicon-expandarrow",
        "addcontact": "otkicon-addcontact",
        "bar": "otkicon-bar",
        "trophy": "otkicon-trophy",
        "pause": "otkicon-pause",
        "microphone": "otkicon-microphone",
        "chatbubble": "otkicon-chatbubble",
        "closecircle": "otkicon-closecircle",
        "play": "otkicon-play",
        "pausecircle": "otkicon-pausecircle",
        "join": "otkicon-join",
        "dlc": "otkicon-dlc",
        "windows": "otkicon-windows",
        "apple": "otkicon-apple",
        "key": "otkicon-key",
        "save": "otkicon-save",
        "cloud": "otkicon-cloud",
        "star": "otkicon-star",
        "warning": "otkicon-warning",
        "controller": "otkicon-controller",
        "access": "otkicon-access",
        "sort": "otkicon-sort",
        "callactive": "otkicon-callactive",
        "callinactive": "otkicon-callinactive",
        "addgroup": "otkicon-addgroup",
        "mute": "otkicon-mute",
        "learnmore": "otkicon-learnmore",
        "offlineleft": "otkicon-offlineleft",
        "offlineright": "otkicon-offlineright",
        "64bit": "otkicon-64bit"
    };


    function originIcon() {

        function originIconLink(scope/*, element, attrs, ctrl*/) {
            scope.iconClass = IconEnumeration[scope.icon];
        }

        return {
            restrict: 'E',
            template: '<i class="otkicon {{ iconClass }}"></i>',
            link: originIconLink,
            scope: {
                icon: '@'
            }
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originIcon
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {IconEnumeration} icon  type of icon to show
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-icon><origin-icon>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originIcon', originIcon);
}());
