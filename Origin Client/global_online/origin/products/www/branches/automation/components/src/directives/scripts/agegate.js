/**
 * @file agegate.js
 */

(function() {
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-agegate';
    var PASSED_EVENT = 'AGEGATE:PASSED';
    var FAILED_EVENT = 'AGEGATE:FAILED';

    function OriginAgegateCtrl($scope, UtilFactory, ComponentsLogFactory, AuthFactory, StoreAgeGateFactory, moment, SessionStorageFactory) {

        var isLoggedIn, monthsStr, i, thisYear;

        /* scope vars */
        $scope.restrictedAge = StoreAgeGateFactory.getMediaLocalAgeGateAge();
        $scope.months = [];
        $scope.days = [];
        $scope.years = [];
        $scope.dob = {};

        function refreshAgeData(loginObj) {

            /* auth */
            isLoggedIn = (loginObj && loginObj.isLoggedIn);
            var storedBirthday = SessionStorageFactory.get('dob');

            if (isLoggedIn) {
                $scope.underage = StoreAgeGateFactory.isUserUnderAgeForMediaLocale() ? 'yes' : 'no';
            } else if (storedBirthday) {
                $scope.underage = StoreAgeGateFactory.isUserUnderAgeForMediaLocale(storedBirthday) ? 'yes': 'no';
            } else {
                $scope.age = 0;
                $scope.underage = 'unknown';
            }

            ComponentsLogFactory.log("agegate: isLoggedIn:" + isLoggedIn);
            ComponentsLogFactory.log("agegate: sessionStorage.dob = " + storedBirthday);
            ComponentsLogFactory.log("agegate: age = " + $scope.age);
            ComponentsLogFactory.log("agegate: restrictedAge = " + $scope.restrictedAge);
            ComponentsLogFactory.log("agegate: underage = " + $scope.underage);

            if($scope.underage === 'no') {
                $scope.$emit(PASSED_EVENT, {});
            } else {
                $scope.$emit(FAILED_EVENT, {});
            }

            if (!$scope.$$phase && !$scope.$root.$$phase) {
                $scope.$digest();
            }
        }

        /* get localized strings */
        $scope.titleStr = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'title-str');
        $scope.descriptionStr = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'description-str');
        $scope.yearLabelStr = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'year-label-str');
        $scope.monthLabelStr = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'month-label-str');
        $scope.dayLabelStr = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'day-label-str');
        $scope.confirmStr = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'confirm-str');

        function updateRestrictedMessage(){
            $scope.restrictedMessage = UtilFactory.getLocalizedStr($scope.restrictedMessage, CONTEXT_NAMESPACE, 'restricted-message');
        }
        $scope.$watch('restrictedMessage', updateRestrictedMessage);

        $scope.getFormattedMonthString = function(){
            return moment($scope.dob.month, 'MM').format('MMMM');
        };

        /* build $scope.months from localized string */
        monthsStr = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'months-str');
        monthsStr = monthsStr.split(',');

        _.forEach(monthsStr, function(value, index) {
            var key = index + 1;
            key = key < 10 ? '0' + key : key;
            $scope.months.push({
                key: key,
                value: value
            });
        });

        /* build $scope.days */
        for (i = 1; i <= 31; i++) {
            $scope.days.push(i < 10 ? '0' + i : i);
        }

        /* build $scope.years */
        thisYear = new Date().getFullYear();
        for (i = thisYear; i > (thisYear - 100); i--) {
            $scope.years.push(i);
        }


        $scope.confirmAge = function() {
            /* update the session dob */
            SessionStorageFactory.set('dob', $scope.dob.year + "-" + $scope.dob.month + "-" + $scope.dob.day);
            /* refresh the age data */
            refreshAgeData();
        };

        /* TODO: This needs to move outside of the directive */
        function onAuthChange(loginObj) {
            /* remove dob from sessionStorage on auth:change */
            SessionStorageFactory.delete('dob');
            refreshAgeData(loginObj);
        }

        function onDestroy() {
            AuthFactory.events.off('myauth:change', onAuthChange);
            AuthFactory.events.off('myauth:ready', refreshAgeData);
        }

        /* Bind to auth events */
        AuthFactory.events.on('myauth:change', onAuthChange);
        AuthFactory.events.on('myauth:ready', refreshAgeData);

        $scope.$on('$destroy', onDestroy);

        function authReadyError(error) {
            ComponentsLogFactory.error('AGEGATE - authReadyError:', error);
        }

        /* Refresh Age Data */
        AuthFactory.waitForAuthReady()
            .then(refreshAgeData)
            .catch(authReadyError);

    }

    function originAgegate(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                matureContent: '@',
                restrictedMessage: '@'
            },
            controller: 'OriginAgegateCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/agegate.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originAgegate
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str Header title
     * @param {LocalizedString} description-str Header description
     * @param {LocalizedString} year-label-str "Year" label
     * @param {LocalizedString} month-label-str "Month" label
     * @param {LocalizedString} day-label-str "Day" label
     * @param {LocalizedString} confirm-str "Confirm" button text
     * @param {LocalizedString} months-str List of the months
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {LocalizedString} restricted-message the message to show the restricted user. This will override the dict value.
     * @description
     *
     * agegate directive
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-agegate
     *             mature-content="true"
     *             restricted-message="Nice try youngster!">
     *         </origin-agegate>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginAgegateCtrl', OriginAgegateCtrl)
        .directive('originAgegate', originAgegate);

}());
