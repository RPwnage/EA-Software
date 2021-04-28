/**
 * @file store/about/scripts/gggpolicy.js
 */
(function () {
    'use strict';

    var DEFAULT_DATE_FORMAT =  'LL';

    function OriginStoreAboutGggpolicyCtrl($scope) {
        $scope.dates = [];
        $scope.dateFormat = DEFAULT_DATE_FORMAT;
        
        // set the currently selected date to be the most current
        function setMostCurrentDate(date) {
            if(!$scope.selectedDate || moment(date).valueOf() > moment($scope.selectedDate).valueOf()) {
                return date;
            } else {
                return $scope.selectedDate;
            }
        }

        // sort the dates, most current ones on top
        function sortDates(a, b) {
            return moment(b).valueOf() - moment(a).valueOf();
        }

        /**
         *
         * @param {string} date - ISO-8601 date time string
         *
         * @description allow children policy item directive to register themselves using thier date
         */
        this.registerPolicyItem = function (date) {
            $scope.dates.push(date);
            $scope.selectedDate = setMostCurrentDate(date);

            if(_.size($scope.dates) > 1) {
                $scope.dates.sort(sortDates);
            }
        };

        this.getSelectedDate = function () {
            return $scope.selectedDate;
        };
    }

    function originStoreAboutGggpolicy(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                titleStr: '@'
            },
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/gggpolicy.html'),
            controller: 'OriginStoreAboutGggpolicyCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutGggpolicy
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedText} title-str - page title
     *
     * @description Container wrapper for GGG policy page
     *              Contains a list of policies indexd by date to select over
     *
     * @example
     * <origin-store-about-gggpolicy title-str="test">
     * </origin-store-about-gggpolicy>
     */

    angular.module('origin-components')
        .controller('OriginStoreAboutGggpolicyCtrl', OriginStoreAboutGggpolicyCtrl)
        .directive('originStoreAboutGggpolicy', originStoreAboutGggpolicy);
}());
