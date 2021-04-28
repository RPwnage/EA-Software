/**
 * @file agegate.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-agegate';

    function OriginAgegateCtrl($scope, UtilFactory, ComponentsLogFactory, AppCommFactory, $window) {

        var isLoggedIn, monthsStr, i, thisYear;

        /* scrope vars */
        $scope.restrictedAge = parseInt($scope.restrictedAge, 10);
        $scope.months = [];
        $scope.days = [];
        $scope.years = [];

        /* ctrl vars */
        this.dob = {};

        function getAge(birthdayStr) {
            var today = new Date();
            var birthDate = new Date(birthdayStr);
            var age = today.getFullYear() - birthDate.getFullYear();
            var m = today.getMonth() - birthDate.getMonth();
            if (m < 0 || (m === 0 && today.getDate() < birthDate.getDate()))
            {
                age--;
            }
            return age;
        }

        function refreshAgeData() {
            /* auth */
            isLoggedIn = (Origin && Origin.auth && Origin.auth.isLoggedIn());

            if (isLoggedIn)
            {
                $scope.age = getAge(Origin.user.dob());
                $scope.underage = ($scope.age < $scope.restrictedAge) ? 'yes' : 'no';
            }
            else if ($window.sessionStorage.getItem('dob'))
            {
                $scope.age = getAge($window.sessionStorage.getItem('dob'));
                $scope.underage = ($scope.age < $scope.restrictedAge) ? 'yes' : 'no';
            }
            else
            {
                $scope.age = 0;
                $scope.underage = 'unknown';
            }

            ComponentsLogFactory.log("agegate: isLoggedIn:" + isLoggedIn);
            ComponentsLogFactory.log("agegate: sessionStorage.dob = " + $window.sessionStorage.getItem('dob'));
            ComponentsLogFactory.log("agegate: age = " + $scope.age);
            ComponentsLogFactory.log("agegate: restrictedAge = " + $scope.restrictedAge);
            ComponentsLogFactory.log("agegate: underage = " + $scope.underage);
        }

        /* get localized strings */
        $scope.restrictedMessage = UtilFactory.getLocalizedStr($scope.restrictedMessage, CONTEXT_NAMESPACE, 'restrictedmessagestr');
        $scope.titleStr = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'titlestr');
        $scope.descriptionStr = UtilFactory.getLocalizedStr($scope.descriptionStr, CONTEXT_NAMESPACE, 'descriptionstr');
        $scope.yearLabelStr = UtilFactory.getLocalizedStr($scope.yearLabelStr, CONTEXT_NAMESPACE, 'yearlabelstr');
        $scope.monthLabelStr = UtilFactory.getLocalizedStr($scope.monthLabelStr, CONTEXT_NAMESPACE, 'monthlabelstr');
        $scope.dayLabelStr = UtilFactory.getLocalizedStr($scope.dayLabelStr, CONTEXT_NAMESPACE, 'daylabelstr');
        $scope.confirmStr = UtilFactory.getLocalizedStr($scope.confirmStr, CONTEXT_NAMESPACE, 'confirmstr');

        /* build $scope.months from localized string */
        monthsStr = UtilFactory.getLocalizedStr($scope.monthsStr, CONTEXT_NAMESPACE, 'monthsstr');
        monthsStr = monthsStr.split(",");

        $.each(monthsStr, function(index, value)
        {
            var key = index + 1;
            key = key < 10 ? "0" + key : key;
            $scope.months.push({key: key, value: value});
        });

        /* build $scope.days */
        for(i = 1; i <= 31; i++)
        {
            $scope.days.push(i < 10 ? "0"+i : i);
        }

        /* build $scope.years */
        thisYear = new Date().getFullYear();
        for(i = thisYear; i > (thisYear-100); i--)
        {
            $scope.years.push(i);
        }

        this.confirmAge = function() {
            /* update the session dob */
            $window.sessionStorage.setItem('dob', this.dob.year + "-" + this.dob.month + "-" + this.dob.day);
            /* refresh the age data */
            refreshAgeData();
        };

        /* TODO: This needs to move outside of the directive */
        function onAuthChange() {
            /* remove dob from sessionStorage on auth:change */
            $window.sessionStorage.removeItem('dob');
            refreshAgeData();
        }

        function onDestroy() {
            AppCommFactory.events.off('auth:change', onAuthChange);
            AppCommFactory.events.off('auth:ready', refreshAgeData);
        }

        /* Bind to auth events */
        AppCommFactory.events.on('auth:change', onAuthChange);
        AppCommFactory.events.on('auth:ready', refreshAgeData);

        $scope.$on('$destroy', onDestroy);

        /* Refresh Age Data */
        refreshAgeData();
    }

    function originAgegate(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                restrictedAge: '@restrictedage',
                restrictedMessage: '@restrictedmessage'
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
     * @param {string} restrictedage the restricted age of the content
     * @param {string} restrictedmessage the message to show the ristricted user. This will override the dict value.
     * @description
     *
     * agegate directive
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-agegate
     *             restrictedage="18"
     *             restrictedmessage="Nice try youngster!">
     *         </origin-agegate>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginAgegateCtrl', OriginAgegateCtrl)
        .directive('originAgegate', originAgegate);

}());