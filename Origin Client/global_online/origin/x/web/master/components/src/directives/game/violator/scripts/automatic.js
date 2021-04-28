/* jshint ignore:start */
(function(){

    'use strict';

    /**
     * Violator Types
     * @enum {Object}
     */
    var ViolatorTypeEnumeration = {
        "billingfailed": {
            "dismissible": "false",
            "icon": "warning",
            "iconcolor": "red"
        },
        "billingpending": {
            "dismissible": "false",
            "icon": "info"
        },
        "preloadon": {
            "dismissible": "false",
            "icon": "download"
        },
        "preloadavailable": {
            "dismissible": "false",
            "icon": "download"
        },
        "releaseson": {
            "dismissible": "false",
            "icon": "download"
        },
        "needsrepair": {
            "dismissible": "false",
            "icon": "warning",
            "iconcolor": "yellow"
        },
        "updateavailable": {
            "dismissible": "false",
            "icon": "download"
        },
        "gameexpires": {
            "dismissible": "false",
            "icon": "timer"
        },
        "trialnotactivated": {
            "dismissible": "false",
            "icon": "info"
        },
        "trialnotexpired": {
            "dismissible": "false",
            "icon": "timer",
            "livecountdown": "true"
        },
        "trialexpired": {
            "dismissible": "false",
            "icon": "timer"
        },
        "gamedisabled": {
            "dismissible": "false",
            "icon": "warning",
            "iconcolor": "red"
        },
        "downloadoverride": {
            "dismissible": "false",
            "icon": "info",
            "iconcolor": "red"
        },
        "newdlcavailable": {
            "dismissible": "true",
            "icon": "dlc"
        },
        "newexpansionavailable": {
            "dismissible": "true",
            "icon": "dlc"
        },
        "newdlcreadyforinstall": {
            "dismissible": "true",
            "icon": "download"
        },
        "newdlcinstalled": {
            "dismissible": "true",
            "icon": "new"
        },
        "gamepatched": {
            "dismissible": "true",
            "icon": "new"
        },
        "gamebeingsunset": {
            "dismissible": "false",
            "icon": "warning",
            "iconcolor": "yellow"
        },
        "gamesunset": {
            "dismissible": "false",
            "icon": "warning",
            "iconcolor": "red"
        }
    };
}());
/* jshint ignore:end */

/**
 * @ngdoc directive
 * @name origin-components.directives:originGameViolatorAutomatic
 * @restrict E
 * @element ANY
 * @scope
 * @param {string} offerid the offer id
 * @param {ViolatorTypeEnumeration} violatortype the violator to build
 * @param {DateTime} enddate the end date for this violator
 * @see  directives/gamelibrary/ogd-violator.js
 *
 * @description
 *
 * The textual violator for game tiles (Translations are inherited from origin-gamelibrary-ogd-violator
 * until these violator messages diverge significantly)
 *
 * @example
 * <example module="origin-components">
 *     <file name="index.html">
 *         <origin-gamelibrary>
 *             <origin-game-violator-automatic
 *                 offerid="OFB-EAST:192333"
 *                 violatortype="trialnotexpired"
 *                 enddate="12-03-15T23:25:01Z"></origin-game-violator-automatic>
 *         </origin-gamelibrary>
 *     </file>
 * </example>
 */