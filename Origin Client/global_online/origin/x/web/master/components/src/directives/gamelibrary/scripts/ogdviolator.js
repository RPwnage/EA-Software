/**
 * @file gamelibrary/ogdviolator.js
 */
(function() {
    'use strict';

    function OriginGamelibraryOgdViolatorCtrl($scope, GamesDataFactory, UtilFactory, DateHelperFactory, moment) {
        var CONTEXT_NAMESPACE = 'origin-gamelibrary-ogd-violator',
            ViolatorMap;

        /**
         * Parse a title that uses an enddate placeholder into a string
         *
         * @param {Object} endDate Javscript Date
         * @param {string} violatorType the violator type enum
         * @return {string} the formatted string
         */
        function parseTitleWithEndDate(endDate, violatorType) {
            var formattedDate;

            if(!DateHelperFactory.isValidDate(endDate)) {
                return '';
            }

            formattedDate = moment(endDate).format('MM/DD/YYYY');

            return UtilFactory.getLocalizedStr(
                $scope[violatorType],
                CONTEXT_NAMESPACE,
                violatorType,
                {'%date%': formattedDate}
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
                {'%days%': countdownData.days},
                countdownData.days);
        }

        /**
         * Automatic Violator map
         * @type {Object}
         */
        ViolatorMap = {
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
                "icon": "download",
                "titleCallback": parseTitleWithEndDate
            },
            "preloadavailable": {
                "dismissible": "false",
                "icon": "download"
            },
            "releaseson": {
                "dismissible": "false",
                "icon": "download",
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
                "iconcolor": "yellow",
                "titleCallback": parseTitleWithDayCountdown
            },
            "gamesunset": {
                "dismissible": "false",
                "icon": "warning",
                "iconcolor": "red"
            }
        };

        /**
         * Parse the violator messaging. Allows for an optional callback in cases of date parsing.
         *
         * @param {string} violatorType the violator type name
         * @return {string} the localized string
         */
        function parseTitle(violatorType) {
            if(ViolatorMap[violatorType].titleCallback) {
                return ViolatorMap[violatorType].titleCallback.call(this, new Date($scope.enddate), violatorType);
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
            } else {
                return;
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
                return "false";
            }
        };

        function refresh() {
            $scope.$digest();
        }

        function destruct() {
            GamesDataFactory.events.off('games:update:' + $scope.offerId, refresh);
        }

        $scope.$on('$destroy', destruct);

        GamesDataFactory.events.on('games:update:' + $scope.offerId, refresh);
    }

    function originGamelibraryOgdViolator(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerid: '@offerid',
                violatortype: '@violatortype',
                enddate: '@enddate',
                billingfailed: '@billingfailed',
                billingpending: '@billingpending',
                preloadon: '@preloadon',
                preloadavailable: '@preloadavailable',
                releaseson: '@releaseson',
                needsrepair: '@needsrepair',
                updateavailable: '@updateavailable',
                gameexpires: '@gameexpires',
                trialnotactivated: '@trialnotactivated',
                trialnotexpired: '@trialnotexpired',
                trialexpired: '@trialexpired',
                gamedisabled: '@gamedisabled',
                downloadoverride: '@downloadoverride',
                newdlcavailable: '@newdlcavailable',
                newexpansionavailable: '@newexpansionavailable',
                newdlcreadyforinstall: '@newdlcreadyforinstall',
                newdlcinstalled: '@newdlcinstalled'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdviolator.html'),
            controller: OriginGamelibraryOgdViolatorCtrl
        };
    }

    function originGameViolatorAutomatic(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerid: '@offerid',
                violatortype: '@violatortype',
                enddate: '@enddate',
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
     * @param {string} offerid the offer id
     * @param {string} violatortype the violator id to build
     * @param {DateTime} enddate the end date for this violator
     * @param {LocalizedString} billingfailed Shown when user's preorder of game failed to bill
     * @param {LocalizedString} billingpending Shown after preorder date when user's payment for preorder game still pending
     * @param {LocalizedString} preloadon Preload on the street date
     * @param {LocalizedString} preloadavailable Preload is available to download
     * @param {LocalizedString} releaseson Releases on a particular catalog date
     * @param {LocalizedString} needsrepair in case of a CRC mismatch, inform the user the game needs repair
     * @param {LocalizedString} updateavailable There is a game update available
     * @param {LocalizedString} gameexpires Game expires on a date. Not for trial use.
     * @param {LocalizedString} trialnotactivated Only for limited trials
     * @param {LocalizedString} trialnotexpired Time left on trial
     * @param {LocalizedString} trialexpired Trial has expired
     * @param {LocalizedString} gamedisabled For games that are no longer available for download.
     * @param {LocalizedString} downloadoverride For 1102 only, if the user has set a download path override, note it in the violator so the user is aware
     * @param {LocalizedString} newdlcavailable (For base game tiles) New DLC is unowned and released within last 7 days
     * @param {LocalizedString} newexpansionavailable (For expansions) Expansion content is unowned and released within last 28 days
     * @param {LocalizedString} newdlcreadyforinstall New DLC or Expansion purchased within last 7 days that is not yet installed
     * @param {LocalizedString} newdlcinstalled New DLC installed for the game within the last 7 days.
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
     *                  enddate="12-03-15T23:25:01Z"
     *                  billingfailed": "Billing Failed"
     *                  billingpending="Billing Pending"
     *                  preloadon="Preload on %date%"
     *                  preloadavailable="Preload Available"
     *                  releaseson="Releases %date%"
     *                  needsrepair="Game needs repair"
     *                  updateavailable="Update Available"
     *                  gameexpires="Expires on %date%"
     *                  trialnotactivated="Game Time not activated"
     *                  trialnotexpired="%enddate% of Game Time left."
     *                  trialexpired="Game Time has ended"
     *                  gamedisabled="Game Disabled"
     *                  downloadoverride="Download Path Override"
     *                  newdlcavailable="New DLC Available"
     *                  newexpansionavailable="New Expansion Available"
     *                  newdlcreadyforinstall="New DLC/Expansion Ready for Install"
     *                  newdlcinstalled="New DLC/Expansion Installed"
     *                  gamepatched="Game Patched"
     *                  gamebeingsunset="{0} Game removed from vault today. | {1} Removed from vault in 1 day | ]1,+Inf] Removed from vault in %days% days"
     *                  gamesunset="Game removed from vault"
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
