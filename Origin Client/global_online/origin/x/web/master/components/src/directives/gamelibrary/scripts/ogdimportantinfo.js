/**
 * @file gamelibrary/ogdimportantinfo.js
 */
(function() {
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

    function OriginGamelibraryOgdImportantInfoCtrl($scope, UtilFactory, Md5Factory) {
        /**
         * Get the dismissibility key name
         *
         * @return {String} the md5 hash for this message
         */
        function getKey() {
            return "pi" + Md5Factory.md5([
                $scope.offerid,
                $scope.title,
                $scope.startdate,
                $scope.enddate,
                $scope.priority || PriorityEnumeration.low
            ]);
        }

        /**
         * Create a live countdown element
         *
         * @return html tag to include for the countdown timer
         */
        function getEndDateElement() {
            return '<span class="important-info-timer"></span>';
        }

        /**
         * Get the Icon class name
         *
         * @return {string} the classname
         */
        $scope.getIconClass = function() {
            if ($scope.icon && IconEnumeration[$scope.icon] && $scope.iconcolor && IconColorEnumeration[$scope.iconcolor]) {
                return 'otkicon-' + $scope.icon + ' ' + $scope.iconcolor;
            } else if ($scope.icon && IconEnumeration[$scope.icon]) {
                return 'otkicon-' + $scope.icon;
            } else {
                return '';
            }
        };

        /**
         * Does the directive have  valid icon?
         *
         * @return {Boolean} true if the icon attribute is valid
         */
        $scope.hasIcon = function() {
            if ($scope.icon && IconEnumeration[$scope.icon]) {
                return true;
            } else {
                return false;
            }
        };

        /**
         * Detect if this component within the campaign's date range(s)
         *
         * @return {Boolean} true if withing the start and end dates
         */
        $scope.isWithinViewableDateRange = function() {
            var startDate,
                endDate,
                currentDate = new Date();

            if ($scope.startdate) {
                startDate = new Date($scope.startdate);
            }

            if ($scope.enddate) {
                endDate = new Date($scope.enddate);
            }

            if (startDate && currentDate < startDate) {
                 return false;
            } else if (endDate && currentDate > endDate) {
                return false;
            } else {
                return true;
            }
        };

        /**
         * Has the user dimissed this message in the past?
         *
         * @return {Boolean} true if the user has dismissed the component
         */
        $scope.isDismissed = function() {
            var key = getKey();
            return (localStorage[key]) ? true : false;
        };

        /**
         * Action to dismiss the message
         */
        $scope.dismiss = function() {
            var key = getKey();
            localStorage[key] = '1';
            $scope.$evalAsync();
        };

        /**
         * Have the promoters made this component dismissible?
         *
         * @return {Boolean} true if the dismissible value is set
         */
        $scope.isDismissible = function() {
            return (String($scope.dismissible).toLowerCase() === DismissibleEnumeration["true"]);
        };

        /**
         * Get the title
         *
         * @return {string} the formatted title
         */
        $scope.getTitle = function() {
            if ($scope.livecountdown === LiveCountdownEnumeration['true']) {
                var token = {};
                token['%enddate%'] = getEndDateElement();
                return UtilFactory.getLocalizedStr($scope.title, null, 'title', token);
            } else {
                return $scope.title;
            }
        };
    }

    function originGamelibraryOgdImportantInfo(ComponentsConfigFactory, $compile, $timeout) {
        function originGamelibraryOgdImportantInfoLink(scope, elem) {
            function appendDateCountdown() {
                var title = $compile('<origin-game-violator-timer enddate="' + scope.enddate + '"></origin-game-violator-timer>')(scope);
                elem.find('.important-info-timer').append(title);
            }

            $timeout(appendDateCountdown, 0);
        }

        return {
            restrict: 'E',
            scope: {
                title: '@title',
                icon: '@icon',
                iconcolor: '@iconcolor',
                dismissible: '@dismissible',
                startdate: '@startdate',
                enddate: '@enddate',
                livecountdown: '@livecountdown',
                offerid: '@offerid',
                priority: '@priority'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdimportantinfo.html'),
            controller: OriginGamelibraryOgdImportantInfoCtrl,
            link: originGamelibraryOgdImportantInfoLink
        };
    }

    function originGameViolatorProgrammed(ComponentsConfigFactory, $compile, $timeout) {
        function originGameViolatorProgrammedLink(scope, elem) {
            function appendDateCountdown() {
                var title = $compile('<origin-game-violator-timer enddate="' + scope.enddate + '"></origin-game-violator-timer>')(scope);
                elem.find('.important-info-timer').append(title);
            }

            $timeout(appendDateCountdown, 0);
        }

        return {
            restrict: 'E',
            scope: {
                title: '@title',
                icon: '@icon',
                iconcolor: '@iconcolor',
                dismissible: '@dismissible',
                startdate: '@startdate',
                enddate: '@enddate',
                livecountdown: '@livecountdown',
                offerid: '@offerid',
                priority: '@priority'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('game/violator/views/programmed.html'),
            controller: OriginGamelibraryOgdImportantInfoCtrl,
            link: originGameViolatorProgrammedLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdImportantInfo
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title the title (use the %enddate% template tag to denote a live countdown timer in the string)
     * @param {IconEnumeration} icon the icon to use
     * @param {IconColorEnumeration} iconcolor the icon color
     * @param {DismissibleEnumeration} dismissible allow or disallow dismissial of the promotion
     * @param {DateTime} startdate the start date to show the promotion
     * @param {DateTime} enddate the end date to hide the promotion
     * @param {LiveCountdownEnumeration} livecountdown true to activate the live countdown to the enddate - annotate countdown placement with %enddate%
     * @param {PriorityEnumeration} priority A priority level for the message (critical messages will trump any automatically generated information)
     * @param {string} offerid the offerid to key on
     * @description
     *
     * A important placement that informs the user of game information in the OGD header
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-ogd>
     *             <origin-gamelibrary-ogd-important-info
     *                  title="Double XP weekend Active! Ends 9/28.&lt;a href=&quot;https://www.google.com&quot;&gt;View More Details&lt;/a&gt;"
     *                  icon="timer"
     *                  dismissible="true"
     *                  livecountdown="false"
     *                  startdate="2015-04-05T10:00:00Z"
     *                  enddate="2015-04-06T11:00:00Z"
     *                  priority="low"
     *                  offerid="OFB-EAST:192333"></origin-gamelibrary-ogd-important-info>
     *         </origin-gamelibrary-ogd>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryOgdImportantInfoCtrl', OriginGamelibraryOgdImportantInfoCtrl)
        .directive('originGamelibraryOgdImportantInfo', originGamelibraryOgdImportantInfo)
        .directive('originGameViolatorProgrammed', originGameViolatorProgrammed);
})();
