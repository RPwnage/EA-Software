
/**
 * @file globalsitestripe/scripts/update.js
 */
(function(){
    'use strict';

    var priority = 1000;
    var countdownThreshold = 2 * 60, // 2 minutes
        countdownIntervalMinutes = 1,
        countdownIntervalMinuteMilliseconds = countdownIntervalMinutes * 60000,
        countdownIntervalSecondMilliseconds = countdownIntervalMinutes * 1000;
    var CONTEXT_NAMESPACE = 'origin-global-sitestripe-update';
    var LOCALSTORAGE_KEY = 'gss-upd:';

    function OriginGlobalSitestripeUpdateCtrl($scope, $interval, GlobalSitestripeFactory, EventsHelperFactory, UtilFactory, DateHelperFactory, NavigationFactory, LocalStorageFactory, UpdateAppFactory) {
        var releaseNotesUrl;
        var localize = UtilFactory.getLocalizedStrGenerator(CONTEXT_NAMESPACE),
            clientUpdateStripeShowing = false;
        $scope.priority = _.isUndefined($scope.priority) ? priority : Number($scope.priority);

        /**
         * Sitestripe dismissed
         */
        $scope.hideMessage = function() {
            LocalStorageFactory.set($scope.localStoragekey, false);
            GlobalSitestripeFactory.hideStripe($scope.$id);
            clientUpdateStripeShowing = false;
        };

        /**
         * Restart client
         */
        $scope.restart = function() {
            Origin.client.user.requestRestart();
        };

        /**
         * Open release notes
         */
        $scope.releaseNotes = function() {
            if (releaseNotesUrl) {
                NavigationFactory.asyncOpenUrlWithEADPSSO(releaseNotesUrl);
            }
        };

        /**
         * Generates a local storage key based on the current state of the controller
         * @param {string} reason - the reason string to encode, parameterized because $scope.reason has dynamic timer
         */
        function generateLocalStorageKey(reason) {
            function getIfDefined (string) {
                return !_.isUndefined(string) ? string : "";
            }

            $scope.localStoragekey = LOCALSTORAGE_KEY.concat(getIfDefined($scope.message))
                .concat(getIfDefined(reason))
                .concat(getIfDefined($scope.ctaRestart))
                .concat(getIfDefined($scope.ctaReleaseNotes))
                .concat(getIfDefined(releaseNotesUrl));
        }

        /**
         * Add the stripe to the GlobalSitestripeFactory to be drawn
         */
        function showStripe(customCTACallback) {
            if (LocalStorageFactory.get($scope.localStoragekey) !== false) {
                GlobalSitestripeFactory.showStripe($scope.$id, $scope.priority, $scope.hideMessage, $scope.message, $scope.reason, $scope.cta, customCTACallback ? customCTACallback: $scope.restart, $scope.icon, $scope.cta2, $scope.releaseNotes);
            }
        }

        /**
         * Cancel the countdown interval
         */
        function cancelCountdown() {
            if (!!$scope.updatePromise) {
                $interval.cancel($scope.updatePromise);
                $scope.updatePromise = undefined;
            }
        }

        /**
         * Start the countdown interval
         */
        function startCountdown(callback, interval) {
            cancelCountdown();
            $scope.updatePromise = $interval(callback, interval);
        }

        /**
         * Update local variables based on this update
         * @param {object} update - the update information
            udpate.releaseNotesUrl - url to open release notes
         */
        function setUpdate(update) {
            if (update.hasOwnProperty('releaseNotesUrl')) {
                releaseNotesUrl = update.releaseNotesUrl;
            }
        }

        /**
         * Show stripe for a new (optional) version of the Origin client
         * @param {object} update - the update information
            update.releaseNotesUrl - url to open release notes
         */
        function showPendingUpdateStripe(update) {
            cancelCountdown();
            setUpdate(update);

            $scope.icon = "passive";
            $scope.message = UtilFactory.getLocalizedStr($scope.pendingUpdateMessage, CONTEXT_NAMESPACE, 'pending-message');
            $scope.reason = UtilFactory.getLocalizedStr($scope.pendingUpdateReason, CONTEXT_NAMESPACE, 'pending-reason');
            $scope.cta = UtilFactory.getLocalizedStr($scope.pendingRestartCta, CONTEXT_NAMESPACE, 'pending-restart-cta');
            $scope.cta2 = UtilFactory.getLocalizedStr($scope.pendingReleaseNotesCta, CONTEXT_NAMESPACE, 'pending-release-notes-cta');
            generateLocalStorageKey($scope.reason);
            showStripe();
            clientUpdateStripeShowing = true;
        }

        /**
         * Show stripe for an imminent mandatory udpate with a countdown for when users are kicked offline
         * @param {object} update - the update information
            update.releaseNotesUrl - url to open release notes
            update.countdown - "hh:mm:ss" time format to countdown
         */
        function showPendingUpdateCountdownStripe(update) {
            function showCountdownStripe(countdown) {
                $scope.icon = "warning";
                $scope.message = UtilFactory.getLocalizedStr($scope.mandatoryMessage, CONTEXT_NAMESPACE, 'mandatory-message');

                if (countdown.hours > 0 || countdown.minutes > 0) {
                    $scope.reason = localize($scope.mandatoryReason, 'mandatory-reason', { '%hours%': countdown.hours, '%minutes%': countdown.minutes });
                } else {
                    $scope.reason = localize($scope.mandatoryReasonSeconds, 'mandatory-reason-seconds', { '%seconds%': countdown.seconds });
                }

                $scope.cta = UtilFactory.getLocalizedStr($scope.mandatoryRestartCta, CONTEXT_NAMESPACE, 'mandatory-restart-cta');
                $scope.cta2 = UtilFactory.getLocalizedStr($scope.mandatoryReleaseNotesCta, CONTEXT_NAMESPACE, 'mandatory-release-notes-cta');
                generateLocalStorageKey($scope.mandatoryReason);
                showStripe();
                clientUpdateStripeShowing = true;

            }

            function updateCountdown() {
                var countdown = DateHelperFactory.getCountdownData($scope.targetDate, new Date());
                showCountdownStripe(countdown);

                if (countdown.totalSeconds > 0) {
                    // In the last 2 minutes use a second interval for accuracy
                    if (countdown.totalSeconds < countdownThreshold) {
                        startCountdown(updateCountdown, countdownIntervalSecondMilliseconds);
                    }
                } else {
                    cancelCountdown();
                }
            }

            if (update.hasOwnProperty('countdown')) {
                setUpdate(update);

                var hms = update.countdown.split(':');
                $scope.targetDate = new Date();
                $scope.targetDate.setHours($scope.targetDate.getHours() + parseInt(hms[0]));
                $scope.targetDate.setMinutes($scope.targetDate.getMinutes() + parseInt(hms[1]));
                $scope.targetDate.setSeconds($scope.targetDate.getSeconds() + parseInt(hms[2]));

                startCountdown(updateCountdown, countdownIntervalMinuteMilliseconds);
                updateCountdown();
            }
        }

        /**
         * Show stripe for an update with an activation date in the past
         * @param {object} update - the update information
            update.releaseNotesUrl - url to open release notes
         */
        function showPendingUpdateKickedOfflineStripe(update) {
            cancelCountdown();
            setUpdate(update);

            $scope.icon = "warning";
            $scope.message = UtilFactory.getLocalizedStr($scope.mandatoryOfflineMessage, CONTEXT_NAMESPACE, 'offline-message');
            $scope.reason = UtilFactory.getLocalizedStr($scope.mandatoryOfflineReason, CONTEXT_NAMESPACE, 'offline-reason');
            $scope.cta = UtilFactory.getLocalizedStr($scope.offlineRestartCta, CONTEXT_NAMESPACE, 'offline-restart-cta');
            $scope.cta2 = UtilFactory.getLocalizedStr($scope.offlineReleaseNotesCta, CONTEXT_NAMESPACE, 'offline-release-notes-cta');
            generateLocalStorageKey($scope.reason);
            showStripe();
            clientUpdateStripeShowing = true;

        }

        /**
         * reloads the entire SPA but leaves user at current route
         */
        function reloadSPAAtCurrentRoute() {
            window.location.reload();
        }

        /**
         * show stripe to update the SPA
         */
        function showNewSPAAvailable() {
            if(!clientUpdateStripeShowing) {
                cancelCountdown();

                $scope.icon = 'warning';
                $scope.message = UtilFactory.getLocalizedStr($scope.mandatoryOfflineMessage, CONTEXT_NAMESPACE, 'spa-restart-message');
                $scope.reason = UtilFactory.getLocalizedStr($scope.mandatoryOfflineReason, CONTEXT_NAMESPACE, 'spa-restart-reason');
                $scope.cta = UtilFactory.getLocalizedStr($scope.offlineRestartCta, CONTEXT_NAMESPACE, 'spa-restart-cta');
                $scope.cta2 = '';
                generateLocalStorageKey($scope.reason);
                showStripe(reloadSPAAtCurrentRoute);                
            }
        }



        var handles = [
            Origin.events.on(Origin.events.CLIENT_NAV_SHOW_PENDING_UPDATE_STRIPE, showPendingUpdateStripe),
            Origin.events.on(Origin.events.CLIENT_NAV_SHOW_PENDING_UPDATE_COUNTDOWN_STRIPE, showPendingUpdateCountdownStripe),
            Origin.events.on(Origin.events.CLIENT_NAV_SHOW_PENDING_UPDATE_KICKED_OFFLINE_STRIPE, showPendingUpdateKickedOfflineStripe),
            UpdateAppFactory.events.on('showNewSPAAvailable', showNewSPAAvailable)
        ];

        /**
        * Called when $scope is destroyed, release event handlers
        * @method onDestroy
        */
        function onDestroy() {
            EventsHelperFactory.detachAll(handles);
        }

        $scope.$on('$destroy', onDestroy);
    }

    function originGlobalSitestripeUpdate() {
        return {
            restrict: 'E',
            scope: {
                priority: '@',
                pendingMessage: '@',
                pendingReason: '@',
                pendingRestartCta: '@',
                pendingReleaseNotesCta: '@',
                mandatoryMessage: '@',
                mandatoryReason: '@',
                mandatoryReasonSeconds: '@',
                mandatoryRestartCta: '@',
                mandatoryReleaseNotesCta: '@',
                offlineMessage: '@',
                offlineReason: '@',
                offlineRestartCta: '@',
                offlineReleaseNotesCta: '@',
                spaRestartMessage: '@',
                spaRestartReason: '@',
                spaRestartCta: '@'
            },
            controller: 'OriginGlobalSitestripeUpdateCtrl'
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originGlobalSitestripeUpdate
     * @restrict E
     * @element ANY
     * @scope
     * @description TRUSTe preferences site stripe
     * @param {Number} priority the priority of the site stripe, controls the order which stripes appear
     * @param {LocalizedString} pending-message message for a non mandatory update
     * @param {LocalizedString} pending-reason reason for a non mandatory update
     * @param {LocalizedString} pending-restart-cta restart cta text
     * @param {LocalizedString} pending-release-notes-cta release notes cta text
     * @param {LocalizedString} mandatory-message mandatory update message
     * @param {LocalizedString} mandatory-reason mandatory update reason, include %hours% and %minutes% for timer
     * @param {LocalizedString} mandatory-reason-seconds mandatory update reason, include %seconds% for timer
     * @param {LocalizedString} mandatory-restart-cta restart cta text
     * @param {LocalizedString} mandatory-release-notes-cta release notes cta text
     * @param {LocalizedString} offline-message offline because of mandatory update message
     * @param {LocalizedString} offline-reason offline because of mandatory update reason
     * @param {LocalizedString} offline-restart-cta restart cta text
     * @param {LocalizedString} offline-release-notes-cta release notes cta text
     * @param {LocalizedString} spa-restart-message new spa available message (New SPA avaiable)
     * @param {LocalizedString} spa-restart-reason new spa available reason (Press %cta% to reload)
     * @param {LocalizedString} spa-restart-cta restart cta text (is placed where %cta token is)
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-global-sitestripe-update />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGlobalSitestripeUpdateCtrl', OriginGlobalSitestripeUpdateCtrl)
        .directive('originGlobalSitestripeUpdate', originGlobalSitestripeUpdate);
}());
