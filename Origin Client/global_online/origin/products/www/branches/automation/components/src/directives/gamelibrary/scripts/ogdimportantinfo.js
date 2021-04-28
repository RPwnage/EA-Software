/**
 * @file gamelibrary/ogdimportantinfo.js
 * @see https://confluence.ea.com/display/EBI/Managing+Game+Packart+and+OGD+Violators
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
        "youtube": "youtube",
        "windows": "windows",
        "apple": "apple",
        "newrelease": "newrelease",
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

    /* jshint ignore:start */
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

    /**
     * LiveCountdownEnumeration detemines the timer style and behavior for the title
     * @enum {string}
     */
    var LiveCountdownEnumeration = {
        "fixed-date": "fixed-date",
        "limited": "limited",
        "days-hours-minutes": "days-hours-minutes"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-gamelibrary-ogd-violator',
        READMORE_DIALOG_ID = 'ogd-violator-read-more-dialog',
        ENDDATE_INTERVAL_MS = 1000;

    function OriginGamelibraryOgdImportantInfoCtrl($scope, $state, UtilFactory, PurchaseFactory, GamesDataFactory, AuthFactory, DialogFactory, GameViolatorFactory) {
        /**
         * Stop propagation of the click event for link helpers and return false so the href does not react.
         * It's best to set the href to javascript:void(0) to prevent unintended default browser behavior
         * @param {Object} $event the click event
         * @return {Boolean}
         */
        function stopPropagation($event) {
            $event.stopPropagation();
            return false;
        }

        /**
         * Close the readmore dialog
         */
        function closeReadMoreDialog() {
            DialogFactory.close(READMORE_DIALOG_ID);
        }

        /**
         * In cases where an action is not possible from offline mode, tell the user to go online before proceeding
         * Because only one dialog can be open at a time, close the read more dialog in case the user is already in that modal
         * @return {Boolean}
         */
        function isClientOnline() {
            if(Origin.client.isEmbeddedBrowser() && !AuthFactory.isClientOnline()) {
                closeReadMoreDialog();

                DialogFactory.openAlert({
                    id: 'origin-violator-offline-action-not-allowed',
                    title: UtilFactory.getLocalizedStr($scope.offlinemodaltitle, CONTEXT_NAMESPACE, 'offlinemodaltitle'),
                    description: UtilFactory.getLocalizedStr($scope.offlinemodaldescription, CONTEXT_NAMESPACE, 'offlinemodaldescription'),
                    rejectText: UtilFactory.getLocalizedStr($scope.offlinemodaldismissbutton, CONTEXT_NAMESPACE, 'offlinemodaldismissbutton')
                });

                return false;
            }

            return true;
        }

        /**
         * Close the OGD window and return to the game library
         */
        function goToGameLibrary() {
            $state.go('app.game_gamelibrary');
        }

        /**
         * Remove entitlement to a vault game
         * @param {Object} $event the click event
         * @return {Boolean}
         */
        function removeVaultGame() {
            if(isClientOnline()) {
                GamesDataFactory.vaultRemove($scope.offerid)
                    .then(goToGameLibrary)
                    .catch(goToGameLibrary);
            }
        }

        /**
         * Hide a game from the user's library
         * @param {Object} $event the click event
         * @return {Boolean}
         */
        function hideGame() {
            if(isClientOnline()) {
                GamesDataFactory.hideGame($scope.offerid);
            }
        }

        /**
         * Renew Origin Access Membership
         * @param {Object} $event the click event
         * @return {Boolean}
         */
        function renewSubscription() {
            if(isClientOnline()) {
                PurchaseFactory.renewSubscription();
            }
        }

        /**
         * Go to the PDP
         * @param {Object} $event the click event
         * @return {Boolean}
         */
        function goToPdp() {
            if(isClientOnline()) {
                GamesDataFactory.buyNow($scope.offerid);
            }
        }

        /**
         * Bring the embedded client back online
         * @param {Object} $event the click event
         * @return {Boolean}
         */
        function goOnline() {
            Origin.client.onlineStatus.goOnline();
        }

        /**
         * Upgrade a vault enabled game
         * @param {Object} $event the click event
         * @return {Boolean}
         */
        function upgradeVaultGame() {
            if(isClientOnline()) {
                PurchaseFactory.entitleVaultGameFromLesserBaseGame($scope.offerid)
                    .then(goToGameLibrary)
                    .catch(goToGameLibrary);
            }
        }

        /**
         * Action to dismiss the violator message
         */
        function dismiss() {
            GameViolatorFactory.dismiss(
                $scope.offerid,
                $scope.priority,
                $scope.title,
                $scope.enddate,
                $scope.startdate,
                $scope.label
            );
        }

        /**
         * Decorate a user action to close the readmore dialog and then
         * call the desired action once the dialog has closed
         * @param  {Function} callback the action to call once the dialog has closed
         * @return {Function}
         */
        function closeDialogAndCall(callback) {
            /**
             * The ui event listener
             * @param  {Object} $event the angular event
             * @return {Boolean}
             */
            return function($event) {
                closeReadMoreDialog();

                window.requestAnimationFrame(callback);

                return stopPropagation($event);
            };
        }

        /**
         * Link Helpers. ng-clicks can be merchandised with the following functions
         */
        $scope.removeVaultGame = closeDialogAndCall(removeVaultGame);
        $scope.hideGame =  closeDialogAndCall(hideGame);
        $scope.renewSubscription =  closeDialogAndCall(renewSubscription);
        $scope.goToPdp =  closeDialogAndCall(goToPdp);
        $scope.goOnline = closeDialogAndCall(goOnline);
        $scope.upgradeVaultGame = closeDialogAndCall(upgradeVaultGame);
        $scope.dismiss = closeDialogAndCall(dismiss);

        /**
         * Get the Icon class name
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
         * Does the directive have a valid icon?
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
         * Has the user dismissed the violator?
         * @return {Boolean}
         */
        $scope.isDismissed = function() {
            return GameViolatorFactory.isDismissed(
                $scope.offerid,
                $scope.priority,
                $scope.title,
                $scope.enddate,
                $scope.startdate,
                $scope.label
            );
        };

        /**
         * Have the promoters made this component dismissible?
         *
         * @return {Boolean} true if the dismissible value is set
         */
        $scope.isDismissible = function() {
            return (String($scope.dismissible).toLowerCase() === BooleanEnumeration.true);
        };

        $scope.isWithinViewableDateRange = function() {
            return GameViolatorFactory.isWithinViewableDateRange($scope);
        };
    }

    function originGamelibraryOgdImportantInfo($interval, ComponentsConfigFactory, ViolatorTitleFactory, DateHelperFactory, GameViolatorFactory) {
        function originGamelibraryOgdImportantInfoLink(scope, elem) {
            var enddateIntervalHandle,
                parsedEnddate;

            /**
             * Append the compiled title string to the title content element
             */
            function appendTitle() {
                ViolatorTitleFactory.appendTitle(scope, elem, CONTEXT_NAMESPACE, '.origin-ogd-message-title-content');
            }

            /**
             * Check that the decalred enddate is not in the past
             * @return {Boolean} [description]
             */
            function isEnddateReached() {
                if(DateHelperFactory.isInThePast(parsedEnddate)) {
                    $interval.cancel(enddateIntervalHandle);
                    GameViolatorFactory.enddateReached(scope.offerid);
                }
            }

            if(scope.title) {
                appendTitle();
            }

            if(scope.enddate) {
                parsedEnddate = new Date(scope.enddate);
                enddateIntervalHandle = $interval(isEnddateReached, ENDDATE_INTERVAL_MS, 0, false);
            }

            scope.$on('$destroy', function() {
                $interval.cancel(enddateIntervalHandle);
            });
        }

        return {
            restrict: 'E',
            scope: {
                title: '@titleStr',
                icon: '@',
                iconcolor: '@',
                dismissible: '@',
                startdate: '@',
                enddate: '@',
                livecountdown: '@',
                offerid: '@',
                label: '@',
                priority: '@',
                timeremaining: '@',
                timerformat: '@',
                offlinemodaltitle: '@',
                offlinemodaldescription: '@',
                offlinemodaldismissbutton: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdimportantinfo.html'),
            controller: OriginGamelibraryOgdImportantInfoCtrl,
            link: originGamelibraryOgdImportantInfoLink
        };
    }

    function originGameViolatorProgrammed($interval, ComponentsConfigFactory, ViolatorTitleFactory, DateHelperFactory, GameViolatorFactory) {
        function originGameViolatorProgrammedLink(scope, elem) {
            var enddateIntervalHandle,
                parsedEnddate;

            /**
             * Append the compiled title string to the title content element
             */
            function appendTitle() {
                ViolatorTitleFactory.appendTitle(scope, elem, CONTEXT_NAMESPACE, '.origin-game-violator-text');
            }

            /**
             * Check that the decalred enddate is not in the past
             * @return {Boolean} [description]
             */
            function isEnddateReached() {
                if(DateHelperFactory.isInThePast(parsedEnddate)) {
                    $interval.cancel(enddateIntervalHandle);
                    GameViolatorFactory.enddateReached(scope.offerid);
                }
            }

            if(scope.title) {
                appendTitle();
            }

            if(scope.enddate) {
                parsedEnddate = new Date(scope.enddate);
                enddateIntervalHandle = $interval(isEnddateReached, ENDDATE_INTERVAL_MS, 0, false);
            }

            scope.$on('$destroy', function() {
                $interval.cancel(enddateIntervalHandle);
            });
        }

        return {
            restrict: 'E',
            scope: {
                title: '@titleStr',
                icon: '@',
                iconcolor: '@',
                dismissible: '@',
                startdate: '@',
                enddate: '@',
                livecountdown: '@',
                offerid: '@',
                label: '@',
                priority: '@',
                timeremaining: '@',
                timerformat: '@',
                offlinemodaltitle: '@',
                offlinemodaldescription: '@',
                offlinemodaldismissbutton: '@'
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
     * @param {LocalizedString} title-str the title (use the %enddate% template tag to denote a live countdown timer in the string)
     * @param {IconEnumeration} icon the icon to use
     * @param {IconColorEnumeration} iconcolor the icon color
     * @param {BooleanEnumeration} dismissible allow or disallow dismissial of the promotion
     * @param {DateTime} startdate the start date to show the promotion (UTC time)
     * @param {DateTime} enddate the end date to hide the promotion (UTC time)
     * @param {LiveCountdownEnumeration} livecountdown date time handler given a timeremaining or enddate - annotate with %time% for time remaining or %enddate% for a fixed future date
     * @param {PriorityEnumeration} priority A priority level for the message (critical messages will trump any automatically generated information)
     * @param {string} offerid the offerid to key on
     * @param {string} label the violator label hint in the case of automatic violators
     * @param {LocalizedString} offlinemodaltitle The title for the offline modal if a user takes an action that requires online services
     * @param {LocalizedString} offlinemodaldescription the description for the offline modal if a user takes an action that requires online services
     * @param {LocalizedString} offlinemodaldismissbutton the dismiss button text for the offline modal if a user takes an action that requires online services
     * @description
     *
     * A important placement that informs the user of game information in the OGD header
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-ogd>
     *             <origin-gamelibrary-ogd-important-info
     *                  title-str="Double XP weekend Active! Ends %enddate%."
     *                  icon="timer"
     *                  dismissible="true"
     *                  livecountdown="fixed-date"
     *                  startdate="2015-04-05T10:00:00Z"
     *                  enddate="2015-04-06T11:00:00Z"
     *                  priority="low"
     *                  offerid="OFB-EAST:192333"
     *                  label="justreleased"></origin-gamelibrary-ogd-important-info>
     *         </origin-gamelibrary-ogd>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryOgdImportantInfoCtrl', OriginGamelibraryOgdImportantInfoCtrl)
        .directive('originGamelibraryOgdImportantInfo', originGamelibraryOgdImportantInfo)
        .directive('originGameViolatorProgrammed', originGameViolatorProgrammed);
})();
