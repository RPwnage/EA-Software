/* jshint ignore:start */
(function(){

    'use strict';

    /**
     * IconEnumeration list of optional icon types
     * @enum {string}
     */
    var IconEnumeration = {
        "dlc": "dlc",
        "settings": "settings",
        "originlogo": "originlogo",
        "store": "store",
        "profile": "profile",
        "download": "download",
        "trophy": "trophy",
        "play": "play",
        "star": "star",
        "controller": "controller",
        "learnmore": "learnmore",
        "help": "help",
        "info": "info",
        "new": "new",
        "warning": "warning",
        "timer": "timer",
        "pin": "pin",
        "youtube": "youtube",
        "windows": "windows",
        "apple": "apple",
        "64bit": "64bit"
    };

    /**
     * IconColorEnumeration list of optional icon colors
     * @enum {string}
     */
    var IconColorEnumeration = {
        "white": "white",
        "yellow": "yellow",
        "red": "red"
    };

    /**
     * BooleanEnumeration
     * @enum {string}
     */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    /**
     * PriorityEnumeration set the priority this component should have against automatic violators
     * @enum {string}
     */
    var PriorityEnumeration = {
        "critical": "critical",
        "high": "high",
        "normal": "normal",
        "low": "low"
    };

    /* jshint ignore:start */
    /**
     * LiveCountdownEnumeration detemines the timer style and behavior for the title
     * @enum {string}
     */
    var LiveCountdownEnumeration = {
        "fixed-date": "fixed-date",
        "limited": "limited",
        "days-hours-minutes": "days-hours-minutes"
    };
}());
/* jshint ignore:end */

/**
 * @ngdoc directive
 * @name origin-components.directives:originGameViolatorProgrammed
 * @restrict E
 * @element ANY
 * @scope
 * @param {LocalizedString} title-str the title (use the %enddate% template tag to denote a live countdown timer in the string)
 * @param {IconEnumeration} icon the icon to use
 * @param {IconColorEnumeration} iconcolor the icon color
 * @param {BooleanEnumeration} dismissible allow or disallow dismissial of the promotion
 * @param {DateTime} startdate the start date to show the promotion (UTC time)
 * @param {DateTime} enddate the end date to hide the promotion (UTC time)
 * @param {LiveCountdownEnumeration} livecountdown date time handler given a timeremaining or enddate - annotate with %time% for time remaining or %enddate% for a fixed future date
 * @param {PriorityEnumeration} priority A priority level for the message (critical messages will trump any automatically generated information)
 * @param {OfferId} offerid the offerid to key on
 * @param {string} label the automatic violator hint
 *
 * @description
 *
 * A programmed game packart violator
 *
 * @example
 * <example module="origin-components">
 *     <file name="index.html">
 *         <origin-game-violator-programmed
 *              title-str="Double XP weekend Active! Ends %enddate%."
 *              icon="timer"
 *              dismissible="true"
 *              livecountdown="fixed-date"
 *              startdate="2015-04-05T10:00:00Z"
 *              enddate="2015-04-06T11:00:00Z"
 *              priority="low"
 *              offerid="OFB-EAST:192333"
 *              label="doublexp"></origin-game-violator-programmed>
 *     </file>
 * </example>
 */