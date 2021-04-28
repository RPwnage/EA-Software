
/**
 * @file globalsitestripe/scripts/subscription.js
 */
(function(){
    'use strict';

    var priority = 0,
        CONTEXT_NAMESPACE = 'origin-global-sitestripe-subscription',
        SUBSCRIPTION_STATE_PENDINGEXPIRED = 'PENDINGEXPIRED',
        SUBSCRIPTION_STATE_EXPIRED = 'EXPIRED',
        LOCALSTORAGE_KEY = 'oax-notification:',
        USER_CANCELATION = 'UserCancellation';

    function OriginGlobalSitestripeSubscriptionCtrl($scope, $controller, UserObserverFactory, UtilFactory, $timeout, SubscriptionFactory, $element, ComponentsConfigHelper, NavigationFactory, LocalStorageFactory, AppCommFactory, GlobalSitestripeFactory) {
        var nextInterval = 0,
            timer = {};

        $scope.priority = _.isUndefined($scope.priority) ? priority : Number($scope.priority);

        UserObserverFactory
            .getObserver($scope, 'user');

        // determine days/hours/minutes remaining in subscription
        function getTimeRemaining() {
            var timeRemaning = {
                    days: 0,
                    hours: 0,
                    minutes: 0
                },
                difference = SubscriptionFactory.getTimeRemaining();

                //difference = new Date('Tue Mar 22 2016 09:29:12 GMT-0700 (Pacific Daylight Time)').valueOf() - new Date().valueOf();

            if(difference > 0) {
                timeRemaning.days = Math.floor(difference/1000/60/60/24);
                timeRemaning.hours = Math.floor(difference/1000/60/60%24);
                timeRemaning.minutes = Math.floor(difference/1000/60%60);
            }

            setMessage(timeRemaning);
        }

        function cancelTimer() {
            $timeout.cancel(timer);
        }

        // show the message and start the timer
        function beginTimer() {
            if(nextInterval > 0) {
                timer = $timeout(function(){
                    this.getTimeRemaining();
                }, nextInterval);
            }
        }

        function expiredWithinMonth() {
            return (new Date().valueOf() - new Date(SubscriptionFactory.getExpiration()).valueOf()) < (30*24*60*60*1000);
        }

        // set the reason for this message and CTA text/action
        function setReason() {
            if($scope.user.subscriptions.detailInfo.Subscription.status === SUBSCRIPTION_STATE_PENDINGEXPIRED && SubscriptionFactory.getCancelReason() !== USER_CANCELATION) { //unable to process payment
                $scope.reason = UtilFactory.getLocalizedStr($scope.reasonCouldNotProcess, CONTEXT_NAMESPACE, 'reason-could-not-process');
                $scope.cta = UtilFactory.getLocalizedStr($scope.ctaCouldNotProcess, CONTEXT_NAMESPACE, 'cta-could-not-process');
                $scope.btnAction = function() {
                    var url = ComponentsConfigHelper.getUrl('eaAccountsPayment');
                        url = url.replace("{locale}", Origin.locale.locale());
                        url = url.replace("{access_token}", "");

                    NavigationFactory.asyncOpenUrlWithEADPSSO(url);
                };
            } else if($scope.user.subscriptions.detailInfo.Subscription.status === SUBSCRIPTION_STATE_EXPIRED && expiredWithinMonth()) { // subscription expired
                $scope.reason = "";
                $scope.cta = UtilFactory.getLocalizedStr($scope.ctaRenewMembership, CONTEXT_NAMESPACE, 'cta-renew-membership');
                $scope.btnAction = function() {
                    AppCommFactory.events.fire('uiRouter:go', 'app.store.wrapper.origin-access');
                };
            } else {
                $scope.hideMessage();
                return;
            }

            beginTimer();
        }

        // set the main message
        function setMessage(timeRemaning) {
            if(timeRemaning.days > 0) {
                $scope.message = UtilFactory.getLocalizedStr($scope.messageExpireInDays, CONTEXT_NAMESPACE, 'message-expire-in-days', {'%time%': timeRemaning.days});
                nextInterval = 60*60*1000;
            } else if(timeRemaning.hours > 0) {
                $scope.message = UtilFactory.getLocalizedStr($scope.messageExpireInHours, CONTEXT_NAMESPACE, 'message-expire-in-hours', {'%time%': timeRemaning.hours});
                nextInterval = 60*60*1000;
            } else if(timeRemaning.minutes > 1) {
                $scope.message = UtilFactory.getLocalizedStr($scope.messageExpireInMinutes, CONTEXT_NAMESPACE, 'message-expire-in-minutes', {'%time%': timeRemaning.minutes});
                nextInterval = 60*1000;
            } else if(timeRemaning.minutes > 0) {
                $scope.message = UtilFactory.getLocalizedStr($scope.messageExpireInOneMinute, CONTEXT_NAMESPACE, 'message-expire-in-one-minute');
                nextInterval = 60*1000;
            } else {
                $scope.message = UtilFactory.getLocalizedStr($scope.messageExpiredHeader, CONTEXT_NAMESPACE, 'message-expired-header');
                nextInterval = 0;
            }

            setReason();
        }

        function cancelCountDown() {
            GlobalSitestripeFactory.hideStripe($scope.$id);
            cancelTimer();
        }

        // unregister the sitestripe and remove from DOM
        $scope.hideMessage = function() {
            LocalStorageFactory.set(LOCALSTORAGE_KEY + $scope.user.subscriptions.basicInfo.subscriptionId, false);
            cancelCountDown();
        };

        // check if the notification should display
        function checkNotificationDisplaySettings() {
            if (LocalStorageFactory.get(LOCALSTORAGE_KEY + $scope.user.subscriptions.basicInfo.subscriptionId) !== false) {
                getTimeRemaining();
                GlobalSitestripeFactory.showStripe($scope.$id, $scope.priority, $scope.hideMessage, $scope.message, $scope.reason, $scope.cta, $scope.btnAction);
            }
        }

        // watch the users subscription state
        $scope.$watch('user.subscriptions.detailInfo.Subscription.status', function() {
            var status = SubscriptionFactory.getStatus();
            if([SUBSCRIPTION_STATE_PENDINGEXPIRED, SUBSCRIPTION_STATE_EXPIRED].indexOf(status) !== -1) {
                checkNotificationDisplaySettings();
            } else {
                cancelCountDown();
            }
        });


        // clean up
        $scope.$on('$destroy', cancelCountDown);
    }

    function originGlobalSitestripeSubscription() {
        return {
            restrict: 'E',
            scope: {
                priority: '@',
                messageExpireInDays: '@',
                messageExpireInHours: '@',
                messageExpireInMinutes: '@',
                messageExpireInOneMinute: '@',
                messageExpiredHeader: '@',
                reasonCouldNotProcess: '@',
                ctaCouldNotProcess: '@',
                reasonRenewMembership: '@',
                ctaRenewMembership: '@'
            },
            controller: 'OriginGlobalSitestripeSubscriptionCtrl'
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originGlobalSitestripeSubscription
     * @restrict E
     * @element ANY
     * @scope
     * @description Origin Access global site notification stripe
     * @param {Number} priority the priority of the site stripe, controls the order which stripes appear
     * @param {LocalizedString} message-expire-in-days "Your Origin Access membership expires in %time% days."
     * @param {LocalizedString} message-expire-in-hours "Your Origin Access membership expires in %time% hours."
     * @param {LocalizedString} message-expire-in-minutes "Your Origin Access membership expires in %time% minutes."
     * @param {LocalizedString} message-expire-in-one-minute "Your Origin Access membership expires in less than one minute."
     * @param {LocalizedString} message-expired-header "Your Origin Access membership has expired."
     * @param {LocalizedString} reason-could-not-process "Your payment could not be processed. %cta%"
     * @param {LocalizedString} cta-could-not-process "Update payment"
     * @param {LocalizedString} reason-renew-membership "%cta%"
     * @param {LocalizedString} cta-renew-membership "Renew membership"
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-sitestripe />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginGlobalSitestripeSubscriptionCtrl', OriginGlobalSitestripeSubscriptionCtrl)
        .directive('originGlobalSitestripeSubscription', originGlobalSitestripeSubscription);
}());
