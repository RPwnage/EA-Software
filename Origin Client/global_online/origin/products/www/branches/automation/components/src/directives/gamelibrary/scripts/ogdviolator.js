/**
 * @file gamelibrary/ogdviolator.js
 */
(function() {
    'use strict';

    function OriginGamelibraryOgdViolatorCtrl($scope, $sce, GamesDataFactory, UtilFactory, DateHelperFactory) {
        var CONTEXT_NAMESPACE = 'origin-gamelibrary-ogd-violator',
            ViolatorMap;

        /**
         * Parse a title that uses a time remaining placeholder into a string
         *
         * @param {Object} timeremaining seconds remainign int a trial
         * @param {string} violatorType the violator type enum
         * @return {string} the formatted string
         */
        function parseTitleWithTrialTimeRemaining(violatorType) {
            var hours = Math.ceil(($scope.timeremaining/60)/60);

            return UtilFactory.getLocalizedStr(
                $scope[violatorType],
                CONTEXT_NAMESPACE,
                violatorType,
                {'%hours%': hours}
            );
        }

        /**
         * Parse a title that uses an enddate placeholder into a string
         *
         * @param {Object} endDate Javscript Date
         * @param {string} violatorType the violator type enum
         * @return {string} the formatted string
         */
        function parseTitleWithEndDate(endDate, violatorType) {
            return UtilFactory.getLocalizedStr(
                $scope[violatorType],
                CONTEXT_NAMESPACE,
                violatorType,
                {'%date%': DateHelperFactory.formatDateWithTimezone(endDate)}
            );
        }

        /**
         * Parse a title with a days placeholder into a string
         *
         * @param {Object} endDate Javscript Date
         * @param {string} violatorType the violator type enum
         * @return {string} the fomatted string
         */
        function parseTitleWithDayCountdown(endDate, violatorType) {
            var countdownData = DateHelperFactory.getCountdownData(endDate);

            if(!countdownData) {
                return '';
            }

            return UtilFactory.getLocalizedStr(
                $scope[violatorType],
                CONTEXT_NAMESPACE,
                violatorType,
                {
                    '%days%': countdownData.days,
                },
                countdownData.days);
        }

        /**
         * Automatic Violator map
         * @type {Object}
         */
        ViolatorMap = {
            "preorderfailed": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "red"
            },
            "preorderfailedogd": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "red"
            },
            "preloadon": {
                "dismissible": "false",
                "icon": "download",
                "titleCallback": parseTitleWithEndDate
            },
            "preloadonwin": {
                "dismissible": "false",
                "icon": "download",
                "titleCallback": parseTitleWithEndDate
            },
            "preloadonmac": {
                "dismissible": "false",
                "icon": "download",
                "titleCallback": parseTitleWithEndDate
            },
            "preloadavailable": {
                "dismissible": "false",
                "icon": "download"
            },
            "justreleased": {
                "dismissible": "true",
                "icon": "newrelease"
            },
            "platformwin": {
                "dismissible": "false",
                "icon": "windows"
            },
            "platformmac": {
                "dismissible": "false",
                "icon": "apple"
            },
            "platformwinmac": {
                "dismissible": "false",
                "icon": "info"
            },
            "wrongarchitecture": {
                "dismissible": "false",
                "icon": "64bit"
            },
            "releasesonwin": {
                "dismissible": "false",
                "icon": "info",
                "titleCallback": parseTitleWithEndDate
            },
            "releasesonmac": {
                "dismissible": "false",
                "icon": "info",
                "titleCallback": parseTitleWithEndDate
            },
            "releaseson": {
                "dismissible": "false",
                "icon": "info",
                "titleCallback": parseTitleWithEndDate
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
                "icon": "timer",
                "titleCallback": parseTitleWithEndDate
            },
            "legacytrialnotactivated": {
                "dismissible": "false",
                "icon": "info",
                "timeRemainingCallback": parseTitleWithTrialTimeRemaining,
                "livecountdown": "limited"
            },
            "legacytrialnotexpired": {
                "dismissible": "false",
                "icon": "timer",
                "titleCallback": parseTitleWithEndDate,
                "livecountdown": "fixed-date"
            },
            "legacytrialexpired": {
                "dismissible": "false",
                "icon": "timer"
            },
            "oatrialnotactivated": {
                "dismissible": "false",
                "icon": "info",
                "timeRemainingCallback": parseTitleWithTrialTimeRemaining
            },
            "oatrialnotexpiredgametile": {
                "dismissible": "false",
                "icon": "timer",
                "livecountdown": "days-hours-minutes"
            },
            "oatrialnotexpired": {
                "dismissible": "false",
                "icon": "timer",
                "livecountdown": "days-hours-minutes"
            },
            "oatrialnotexpiredreleased": {
                "dismissible": "false",
                "icon": "timer",
                "livecountdown": "days-hours-minutes"
            },
            "oatrialexpiredgametile": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "red"
            },
            "oatrialexpired": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "red",
            },
            "oatrialexpiredhidden": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "red",
            },
            "oatrialexpiredreleased": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "red",
            },
            "oatrialexpiredhiddenreleased": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "red",
            },
            "oaexpiredgametile": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "red"
            },
            "oaexpired": {
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
            "newdlcexpansionreadyforinstall": {
                "dismissible": "true",
                "icon": "download"
            },
            "newdlcinstalled": {
                "dismissible": "true",
                "icon": "newrelease"
            },
            "expired": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "red"
            },
            "expireson": {
                "dismissible": "false",
                "icon": "timer",
                "titleCallback": parseTitleWithEndDate
            },
            "offlineplayexpiringgametile": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "yellow",
                "titleCallback": parseTitleWithDayCountdown,
                "livecountdown": "fixed-date"
            },
            "offlineplayexpiring": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "yellow",
                "titleCallback": parseTitleWithDayCountdown,
                "livecountdown": "fixed-date"
            },
            "offlineplayexpiredgametile": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "red"
            },
            "offlineplayexpired": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "red"
            },
            "offlinemodegametile": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "yellow"
            },
            "offlinemode": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "yellow"
            },
            "preannouncementdisplaydate": {
                "dismissible": "false",
                "icon": "info",
                "passTitle": "true"
            },
            "preloaded": {
                "dismissible": "false",
                "icon": "info"
            },
            "oagameexpiringgametile": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "yellow",
                "livecountdown": "days-hours-minutes"
            },
            "oagameexpiring": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "yellow",
                "livecountdown": "days-hours-minutes"
            },
            "oagameexpiredgametile": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "red",
            },
            "oagameexpired": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "red",
            },
            "oaplatformwin": {
                "dismissible": "false",
                "icon": "windows",
            },
            "oagameupgradenoticegametile": {
                "dismissible": "false",
                "icon": "download",
                "iconcolor": "white"
            },
            "oagameupgradenotice": {
                "dismissible": "false",
                "icon": "download",
                "iconcolor": "white"
            }
        };

        /* jshint ignore:start */
        /**
         * TimerTypeEnumeration
         * @enum {string}
         */
        var TimerTypeEnumeration = {
            "trial": "trial",
            "precise": "precise"
        };
        /* jshint ignore:end */


        /**
         * Parse the violator messaging. Allows for an optional callback in cases of date parsing.
         *
         * @param {string} violatorType the violator type name
         * @return {string} the localized string
         */
        function parseTitle(violatorType) {
            if(_.has(ViolatorMap[violatorType],'passTitle') && ViolatorMap[violatorType].passTitle === 'true') {
                return $scope.enddate;
            } else if(ViolatorMap[violatorType].titleCallback) {
                return ViolatorMap[violatorType].titleCallback.call(this, new Date($scope.enddate), violatorType);
            } else if(ViolatorMap[violatorType].timeRemainingCallback) {
                return ViolatorMap[violatorType].timeRemainingCallback.call(this, violatorType);
            } else {
                return UtilFactory.getLocalizedStr($scope[violatorType], CONTEXT_NAMESPACE, violatorType);
            }
        }

        /**
         * Get the title for the violator
         * @return {string} the title string
         */
        $scope.getTitle = function() {
            var type = String($scope.violatortype);

            if(ViolatorMap[type]) {
                return parseTitle(type);
            }
        };

        /**
         * Get the icon name if applicable
         * @return {string} the icon enumeration to use
         */
        $scope.getIcon = function() {
            var type = String($scope.violatortype);

            if(ViolatorMap[type] && ViolatorMap[type].icon) {
                return ViolatorMap[type].icon;
            } else {
                return;
            }
        };

        /**
         * Get the dismissibility flag if applicable
         * @return {string} the dismissibilty flag to use
         */
        $scope.getDismissibility = function() {
            var type = String($scope.violatortype);

            if(ViolatorMap[type] && ViolatorMap[type].dismissible) {
                return ViolatorMap[type].dismissible;
            } else {
                return "false";
            }
        };

        /**
         * Get the icon color class if applicable
         * @return {string} the icon color enumeration to use
         */
        $scope.getIconColor = function() {
            var type = String($scope.violatortype);

            if(ViolatorMap[type] && ViolatorMap[type].iconcolor) {
                return ViolatorMap[type].iconcolor;
            } else {
                return;
            }
        };

        /**
         * Get the live countdown to the enddate if applicable
         * @return {string} the live countdown enumeration to use
         */
        $scope.getLiveCountdown = function() {
            var type = String($scope.violatortype);

            if(ViolatorMap[type] && ViolatorMap[type].livecountdown) {
                return ViolatorMap[type].livecountdown;
            } else {
                return "";
            }
        };
    }

    function originGamelibraryOgdViolator(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                violatortype: '@',
                priority: '@',
                enddate: '@',
                preorderfailed: '@',
                preorderfailedogd: '@',
                timeremaining: '@',
                timerformat: '@',
                preloadon: '@',
                preloadonwin: '@',
                preloadonmac: '@',
                preloadavailable: '@',
                justreleased: '@',
                releaseson: '@',
                releasesonwin: '@',
                releasesonmac: '@',
                platformwin: '@',
                platformmac: '@',
                platformwinmac: '@',
                updateavailable: '@',
                wrongarchitecture: '@',
                gameexpires: '@',
                legacytrialnotactivated: '@',
                legacytrialnotexpired: '@',
                legacytrialexpired: '@',
                downloadoverride: '@',
                newdlcavailable: '@',
                newexpansionavailable: '@',
                newdlcreadyforinstall: '@',
                newdlcexpansionreadyforinstall: '@',
                newdlcinstalled: '@',
                expired: '@',
                expireson: '@',
                offlineplayexpiringgametile: '@',
                offlineplayexpiring: '@',
                offlineplayexpiredgametile: '@',
                offlineplayexpired: '@',
                oatrialexpired: '@',
                oatrialexpiredhidden: '@',
                oatrialexpiredreleased: '@',
                oatrialexpiredhiddenreleased: '@',
                oaexpiredgametile: '@',
                oaexpired: '@',
                oatrialnotexpired: '@',
                oatrialnotexpiredreleased: '@',
                oatrialnotactivated: '@',
                offlinemode: '@',
                preloaded: '@',
                oagameexpiringgametile: '@',
                oagameexpiring: '@',
                oagameexpiredgametile: '@',
                oagameexpired: '@',
                oaplatformwin: '@',
                oagameupgradenoticegametile : '@',
                oagameupgradenotice: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdviolator.html'),
            controller: OriginGamelibraryOgdViolatorCtrl
        };
    }

    function originGameViolatorAutomatic(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                violatortype: '@',
                priority: '@',
                enddate: '@',
                timeremaining: '@',
                timerformat: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/automatic.html'),
            controller: OriginGamelibraryOgdViolatorCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdViolator
     * @restrict E
     * @element ANY
     * @scope
     * @param {OfferId} offerid the offer id
     * @param {ViolatorMap} violatortype the violator id to build
     * @param {string} priority the priority level
     * @param {DateTime} enddate the end date for this violator (UTC time)
     * @param {Number} timeremaining the time remaining or this violator in seconds
     * @param {TimerTypeEnumeration} timerformat the timer format used
     * @param {LocalizedString} preorderfailed Shown when user's account is still in preorder state but the release date has passed (game tile)
     * @param {LocalizedString} preorderfailedogd Shown when user's account is still in preorder state but the release date has passed (ogd)
     * @param {LocalizedString} preloadon Preload on the street date
     * @param {LocalizedString} preloadonwin Preload on street date for the windows platform
     * @param {LocalizedString} preloadonmac Preload on the street date for the mac platform
     * @param {LocalizedString} preloadavailable Preload is available to download
     * @param {LocalizedString} justreleased Entitlement was just released
     * @param {LocalizedString} releaseson Releases on a particular catalog date
     * @param {LocalizedString} releasesonwin Releases on a particular catalog date for PC only (only on invalid web platforms)
     * @param {LocalizedString} releasesonmac Releases on a particular catalog date for Mac only (only on invalid web platforms)
     * @param {LocalizedString} releasesonwinmac Releases on a particular catalog date for PC and Mac (only on invalid web platforms)
     * @param {LocalizedString} platformwin Shown if the platform is Win Only (only on invalid web platforms)
     * @param {LocalizedString} platformmac Shown if the platform is Mac Only (only on invalid web platforms)
     * @param {LocalizedString} platformwinmac Shown if the game is a hybrid game and in a browser context
     * @param {LocalizedString} wrongarchitecture Only available on 64-bit systems.
     * @param {LocalizedString} updateavailable There is a game update available
     * @param {LocalizedString} gameexpires Game expires on a date. Not for trial use.
     * @param {LocalizedString} legacytrialnotactivated Only for limited trials
     * @param {LocalizedString} legacytrialnotexpired Time left on trial
     * @param {LocalizedString} legacytrialexpired Trial has expired
     * @param {LocalizedString} downloadoverride For 1102 only, if the user has set a download path override, note it in the violator so the user is aware
     * @param {LocalizedString} newdlcavailable (For base game tiles) New DLC is unowned and released within last 7 days
     * @param {LocalizedString} newexpansionavailable (For expansions) Expansion content is unowned and released within last 28 days
     * @param {LocalizedString} newdlcreadyforinstall New Addon content purchased within last 7 days that is not yet installed
     * @param {LocalizedString} newdlcexpansionreadyforinstall New Expansion purchased within last 7 days that is not yet installed
     * @param {LocalizedString} newdlcinstalled New DLC installed for the game within the last 7 days.
     * @param {LocalizedString} expired Game is expired message when the game has been removed from the catalog
     * @param {LocalizedString} expireson Game Expires on: %date%  when the game is being removed from the catalog
     * @param {LocalizedString} offlineplayexpiringgametile Offline play of subs games expires in X time.
     * @param {LocalizedString} offlineplayexpiring Offline play of subs games expires in X time.
     * @param {LocalizedString} offlineplayexpiredgametile Offline play of subs games has expired (game tile).
     * @param {LocalizedString} offlineplayexpired Offline play of subs games has expired (ogd).
     * @param {LocalizedString} oatrialnotactivated Time limited OA Early access and normal trial not yet activated
     * @param {LocalizedString} oatrialnotexpiredgametile Time limited OA Early access and normal trial in progress use %time% for time left expressions.
     * @param {LocalizedString} oatrialnotexpired Time limited OA Early access and normal trial in progress use %time% for time left expressions.
     * @param {LocalizedString} oatrialnotexpiredreleased Time limited OA Early access and normal trial in progress but already released. Use %time% for time left expressions.
     * @param {LocalizedString} daystemplate Pluralized template for displaying the number of days
     * @param {LocalizedString} hourstemplate Pluralized template for displaying the number of hours
     * @param {LocalizedString} minutestemplate Pluralized template for displaying the number of minutes
     * @param {LocalizedString} oatrialexpiredgametile Time limited OA Early access and normal trial expired (game tile)
     * @param {LocalizedString} oatrialexpired Time limited OA Early access and normal trial expired and not released(ogd)
     * @param {LocalizedString} oatrialexpiredhidden Time limited OA Early access and normal trial expired and not released (for hidden games in the ogd)
     * @param {LocalizedString} oatrialexpiredreleased Time limited OA Early access and normal trial expired and released (ogd)
     * @param {LocalizedString} oatrialexpiredhiddenreleased Time limited OA Early access and normal trial expired and released (for hidden games in the ogd)
     * @param {LocalizedString} oaexpiredgametile Origin Access has expired on a subscription game (game tile)
     * @param {LocalizedString} oaexpired Origin Access has expired on a subscription game (ogd)
     * @param {LocalizedString} offlinemodegametile Connect to the internet to play this trial. Go Online. (game tile)
     * @param {LocalizedString} offlinemode Connect to the internet to play this trial. Go Online. (ogd)
     * @param {LocalizedString} preloaded The game has been preloaded by the user, but is not yet released
     * @param {LocalizedString} oagameexpiringgametile the game is expiring in the next 90 days - use %time% to place a timer (game tile)
     * @param {LocalizedString} oagameexpiring the game is expiring in the next 90 days - use '%time%' to place a timer (ogd)
     * @param {LocalizedString} oagameexpiredgametile the game is no longer available in the vault (sunset, game tile)
     * @param {LocalizedString} oagameexpired the game is no longer available in the vault (sunset, ogd)
     * @param {LocalizedString} oaplatformwin available on origin access for windows only.
     * @param {LocalizedString} oagameupgradenoticegametile the upgrade the game from the owned edition to the vault edition if applicable (game tile)
     * @param {LocalizedString} oagameupgradenotice the upgrade the game from the owned edition to the vault edition if applicable (ogd)
     * @param {LocalizedString} offlinemodaltitle The title for the offline modal if a user takes an action that requires online services
     * @param {LocalizedString} offlinemodaldescription the description for the offline modal if a user takes an action that requires online services
     * @param {LocalizedString} offlinemodaldismissbutton the dismiss button text for the offline modal if a user takes an action that requires online services
     *
     * @description
     *
     * The textual violator for the owned game details flyout
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-ogd>
     *             <origin-gamelibrary-ogd-violator
     *                  offerid="OFB-EAST:192333"
     *                  violatortype="trialnotexpired"
     *                  priority="normal"
     *                  enddate="12-03-15T23:25:01Z"
     *                  timeremaining="29898938"
     *                  preorderfailed="Preorder Failed"
     *                  preorderfailedogd="Pre-order failed - Your order couldn't be processed. <a href=\"https://help.ea.com/en-us/help/account/origin-order-history-and-status-guide/\" class=\"otka external-link\">Learn more</a>"
     *                  preloadon="Preload on %date%"
     *                  preloadonwin="Windows Preload Date: %date%"
     *                  preloadonmac="Mac Preload Date: %date%"
     *                  preloadavailable="Preload Available"
     *                  justreleased="Just Released"
     *                  plaformwin="Only Available On Windows"
     *                  platformmac="Only Available on Mac"
     *                  platformwinmac="Available on mac and windows"
     *                  wrongarchitecture="Only available on 64-bit systems."
     *                  releaseson="Releases %date%"
     *                  releasedatewin="Windows release date: %date%"
     *                  releasedatemac="Mac Release Date: %date%"
     *                  updateavailable="Update Available"
     *                  gameexpires="Expires on %date%"
     *                  legacytrialnotactivated="Game Time not activated"
     *                  legacytrialnotexpired="%enddate% of Game Time left."
     *                  legacytrialexpired="Game Time has ended"
     *                  downloadoverride="Download Path Override"
     *                  newdlcavailable="New DLC Available"
     *                  newexpansionavailable="New Expansion Available"
     *                  newdlcreadyforinstall="New Addon Ready for Install"
     *                  newdlcexpansionreadyforinstall="New Expansion Ready for Install"
     *                  newdlcinstalled="New DLC/Expansion Installed"
     *                  expired="Game is Expired"
     *                  expireson="Game Expires on: %date%"
     *                  offlineplayexpiringgametile="Online connection required in %enddate% to verify Origin Access membership. Go online and verify."
     *                  offlineplayexpiring="Online connection required in %enddate% to verify Origin Access membership. <a href=\"javascript:void(0);\" ng-click=\"goOnline($event)\" class=\"otka\">Go online and verify.</a>"
     *                  offlineplayexpiredgametile="Offline play expired."
     *                  offlineplayexpired="Offline play expired. <a href=\"javascript:void(0);\" ng-click=\"goOnline($event)\" class=\"otka\">Go online</a> to verify your Origin Access membership and continue playing"
     *                  oatrialnotactivated="Play up to %hours% hours."
     *                  oatrialnotexpiredgametile="Time left in Trial: %time%."
     *                  oatrialnotexpired="Time left in Trial: %time%. <a href=\"javascript:void(0);\" ng-click=\"goToPdp($event)\" class=\"otka\">Pre-order game</a>."
     *                  oatrialnotexpiredreleased="Trial time left: %time%. <a href=\"javascript:void(0);\" ng-click=\"goToPdp($event)\" class=\"otka\">Purchase game</a>."
     *                  daystemplate="{1} 1 day | [0,+Inf] %days% days"
     *                  hourstemplate="{1} 1 hour | [0,+Inf] %hours% hours"
     *                  minutestemplate="{1} 1 minute | [0,+Inf] %minutes% minutes"
     *                  oatrialexpiredgametile="You've already completed your trial."
     *                  oatrialexpired="You've already completed your trial. <br/><a href=\"javascript:%preorder%\" class=\"otka\">Pre-order full game</a> or <a href=\"javascript:%hide%\" class=\"otka\">hide in library</a>."
     *                  oatrialexpiredhidden="You've already completed your trial. <br/><a href=\"javascript:%preorder%\" class=\"otka\">Pre-order full game</a>."
     *                  oatrialexpiredreleased="You've already completed your trial. <br/><a href=\"javascript:%preorder%\" class=\"otka\">Purchase full game</a> or <a href=\"javascript:%hide%\" class=\"otka\">hide in library</a>."
     *                  oatrialexpiredhiddenreleased="You've already completed your trial. <br/><a href=\"javascript:%preorder%\" class=\"otka\">Purchase full game</a>."
     *                  oaexpiredgametile="Your Origin Access membership has expired."
     *                  oaexpired="Your Origin Access membership has expired. <br/><a href=\"javascript:void(0);\" ng-click=\"renewSubscription($event)\" class=\"otka\">Renew membership</a> or <a href=\"javascript:void(0);\" ng-click=\"goToPdp($event)\" class=\"otka\">buy the game</a> to continue playing."
     *                  offlinemodegametile="Connect to the internet to play this trial. Go Online."
     *                  offlinemode="Connect to the internet to play this trial. Go Online."
     *                  preloaded="Preloaded"
     *                  oagameexpiringgametile="This game is leaving the vault in %time%."
     *                  oagameexpiring="This game is leaving the vault in %time%. <a href=\"javascript:void(0);\" ng-click=\"removeVaultGame($event)\" class=\"otka\">Remove from library</a>."
     *                  oagameexpiredgametile="No longer included with Origin Access."
     *                  oagameexpired="No longer included with Origin Access. <a href=\"javascript:void(0);\" ng-click=\"goToPdp()\">Buy the game</a> or <a href=\"javascript:void(0);\" ng-click=\"removeVaultGame()\">remove from library.</a>"
     *                  oaplatformwin="Available on Origin Access for Windows only. <a href=\"javascript:void(0);\" ng-click=\"goToPdp($event)\" class=\"otka\">Buy game for Mac.</a>"
     *                  oagameupgradenoticegametile="Upgrade your game at no extra cost."
     *                  oagameupgradenotice="Upgrade your game at no extra cost. <a href=\"javascript:void(0);\" ng-click=\"upgradeVaultGame($event)\" class=\"otka\">Upgrade Now</a>."
     *                  offlinemodaltitle="Oops, you're offline!"
     *                  offlinemodaldescription="You'll have to go online to perform this action."
     *                  offlinemodaldismissbutton="OK"
     *                 ></origin-gamelibrary-ogd-violator>
     *         </origin-gamelibrary-ogd>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryOgdViolatorCtrl', OriginGamelibraryOgdViolatorCtrl)
        .directive('originGamelibraryOgdViolator', originGamelibraryOgdViolator)
        .directive('originGameViolatorAutomatic', originGameViolatorAutomatic);
})();
