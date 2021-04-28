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
        "youtube": "youtube"
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
     * DismissibleEnumeration allow or diasallow dismissibility
     * @enum {string}
     */
    var DismissibleEnumeration = {
        "true": "true",
        "false": "false"
    };

    /**
     * LiveCountdownEnumeration allow or diasallow a live countdown
     * @enum {string}
     */
    var LiveCountdownEnumeration = {
        "true": "true",
        "false": "false"
    };

    /**
     * PriorityEnumeration set the priority this component should have against automatic violators
     * @enum {string}
     * @see https://confluence.ea.com/display/EBI/Managing+Game+Packart+and+OGD+Violators
     */
    var PriorityEnumeration = {
        "critical": "critical",
        "high": "high",
        "normal": "normal",
        "low": "low"
    };
}());
/* jshint ignore:end */

/**
 * @ngdoc directive
 * @name origin-components.directives:originGameViolatorProgrammed
 * @restrict E
 * @element ANY
 * @scope
 * @param {LocalizedString} title the title (use the %enddate% template tag to denote a live countdown timer in the string)
 * @param {IconEnumeration} icon the icon to use
 * @param {IconColorEnumeration} iconcolor the icon color
 * @param {DateTime} startdate the start date to show the promotion
 * @param {DateTime} enddate the end date to hide the promotion
 * @param {LiveCountdownEnumeration} livecountdown true to activate the live countdown to the enddate - annotate countdown placement with %enddate%
 * @param {PriorityEnumeration} priority A priority level for the message (critical messages will trump any automatically generated information)
 * @param {string} offerid the offerid to key on
 * @see  directives/gamelibrary/ogdimportantinfo
 * @description
 *
 * A programmed message that overlays the game packart
 *
 * @example
 * <example module="origin-components">
 *     <file name="index.html">
 *         <origin-game-violator>
 *             <origin-game-violator-programmed
 *                  title="Double XP weekend Active! Ends 9/28.&lt;a href=&quot;https://www.google.com&quot;&gt;View More Details&lt;/a&gt;"
 *                  icon="timer"
 *                  dismissible="true"
 *                  livecountdown="false"
 *                  startdate="2015-04-05T10:00:00Z"
 *                  enddate="2015-04-06T11:00:00Z"
 *                  priority="low"
 *                  offerid="OFB-EAST:192333"></origin-game-violator-programmed>
 *         </origin-game-violator>
 *     </file>
 * </example>
 */