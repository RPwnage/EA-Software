'use explicit';

var helper = require("./helper.js")();

module.exports = function() {
  var buttonEnumerations = {
    ButtonTypeEnumeration : {
        "primary" : "primary",
        "secondary" : "secondary",
        "transparent" : "transparent"
    },

    SimpleButtonTypeEnumeration : {
      "primary" : "primary",
      "secondary" : "secondary",
    },

    ExtenedButtonTypeEnumeration : {
      "primary" : "primary",
      "secondary" : "secondary",
      "round" : "round",
      "transparent" : "transparent"
    },

    OptionalButtonTypeEnumeration : {
      "none" : "",
      "primary" : "primary",
      "secondary" : "secondary",
      "transparent" : "transparent"
    },

    ShortExtenedButtonTypeEnumeration: {
      "secondary" : "secondary",
      "transparent" : "transparent",
      "round" : "round"
    },

    DispositionEnumeration : {
      "hide": "hide",
      "logout": "logout",
      "externalurlSSO": "externalurlSSO"
    },

    ShortDispositionEnumeration : {
      "hide": "hide",
      "logout": "logout"
    },

    OTKIconEnumeration : {
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
        "offlineright": "otkicon-offlineright"
    },

    AnchorIconEnumeration : {
        "otkicon-person": "otkicon-person",
        "otkicon-multiplayer": "otkicon-multiplayer",
        "otkicon-controller": "otkicon-controller",
        "otkicon-trophy": "otkicon-trophy",
        "otkicon-cloud": "otkicon-cloud",
        "otkicon-ggg": "otkicon-ggg",
        "otkicon-download": "otkicon-download",
        "otkicon-progressiveinstall": "otkicon-progressiveinstall",
        "otkicon-chatbubble": "otkicon-chatbubble",
        "otkicon-twitch": "otkicon-twitch",
        "otkicon-timer": "otkicon-timer",
        "otkicon-offlinemode": "otkicon-offlinemode",
        "otkicon-modding": "otkicon-modding",
        "otkicon-leaderboard": "otkicon-leaderboard",
        "otkicon-app": "otkicon-app",
        "otkicon-splitscreen": "otkicon-splitscreen",
        "otkicon-dolby": "otkicon-dolby",
        "otkicon-touch": "otkicon-touch",
        "otkicon-crossplatform": "otkicon-crossplatform",
        "otkicon-4k": "otkicon-4k",
        "otkicon-directx": "otkicon-directx",
        "otkicon-refresh": "otkicon-refresh",
        "otkicon-firstperson": "otkicon-firstperson",
        "otkicon-thirdperson": "otkicon-thirdperson",
        "otkicon-challenge": "otkicon-challenge",
        "otkicon-downloadnegative": "otkicon-downloadnegative",
        "otkicon-mouse": "otkicon-mouse",
        "otkicon-coop": "otkicon-coop"
    },

    DetailItemIconEnumeration : {
            "otkicon-person": "otkicon-person",
            "otkicon-multiplayer": "otkicon-multiplayer",
            "otkicon-coop": "otkicon-coop",
            "otkicon-controller": "otkicon-controller",
            "otkicon-trophy": "otkicon-trophy",
            "otkicon-cloud": "otkicon-cloud",
            "otkicon-download": "otkicon-download",
            "otkicon-progressiveinstall": "otkicon-progressiveinstall",
            "otkicon-chatbubble": "otkicon-chatbubble",
            "otkicon-twitch": "otkicon-twitch",
            "otkicon-timer": "otkicon-timer",
            "otkicon-offlinemode": "otkicon-offlinemode",
            "otkicon-modding": "otkicon-modding",
            "otkicon-leaderboard": "otkicon-leaderboard",
            "otkicon-app": "otkicon-app",
            "otkicon-splitscreen": "otkicon-splitscreen",
            "otkicon-dolby": "otkicon-dolby",
            "otkicon-touch": "otkicon-touch",
            "otkicon-crossplatform": "otkicon-crossplatform",
            "otkicon-4k": "otkicon-4k",
            "otkicon-directx": "otkicon-directx",
            "otkicon-refresh": "otkicon-refresh",
            "otkicon-firstperson": "otkicon-firstperson",
            "otkicon-thirdperson": "otkicon-thirdperson",
            "otkicon-challenge": "otkicon-challenge",
            "otkicon-mouse": "otkicon-mouse",
            "otkicon-ggg": "otkicon-ggg"
    }

  };

  function findEnmeratioin(o) {
    for (var key in buttonEnumerations) {
      if (helper.areSameObject(o, buttonEnumerations[key])) {
        return key;
      }
    }
  }

  return {
    findEnmeratioin : findEnmeratioin
  }
}
