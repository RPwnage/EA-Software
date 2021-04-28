/**
 * @file /policy.js
 */
(function () {
    'use strict';

    var DEFAULT_DATE_FORMAT =  'LL';

    function OriginPolicyCtrl($scope, formatDateFilter, $stateParams) {
        $scope.selectedPolicy = null;
        $scope.policies = {};
        $scope.dateFormat = DEFAULT_DATE_FORMAT;
        var firstPolicyRegistered;

        /**
         * @typedef policyItem
         * @type {object}
         * @property {string} date ISO-8601 date time string
         * @property {string} id Unique identifier string for the item. ex 'fifafanatics-07-26-16'
         * @property {string} name Localized name of the policy
         */
        /**
         * Allows children policy item directives to register themselves using thier date, id and name
         * @param {policyItem} policy Information about the policy
         */
        this.registerPolicyItem = function (policy) {
            firstPolicyRegistered = firstPolicyRegistered || policy;
            policy.date = formatDateFilter(policy.date, DEFAULT_DATE_FORMAT);
            $scope.policies[policy.id] = policy; // Index policies by id
            $scope.selectedPolicy = $scope.policies[$stateParams.id] || firstPolicyRegistered;
        };

        this.getSelectedPolicy = function () {
            return $scope.selectedPolicy;
        };
    }

    function originPolicy(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {},
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('/views/policy.html'),
            controller: 'OriginPolicyCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originPolicy
     * @restrict E
     * @element ANY
     * @scope
     * @description Container wrapper for Terms And Conditions policy page
     *              Contains a list of policies indexed by date to view
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *        <origin-policy>
     *          <origin-policyitem date="2016-02-01"
     *                             content="test"
     *                             title="Fifa Fanatics"
     *                             id="fifafanatics-2016-02-01"></origin-policyitem>
     *        </origin-policy>
     *     </file>
     * </example>
     */

    angular.module('origin-components')
        .controller('OriginPolicyCtrl', OriginPolicyCtrl)
        .directive('originPolicy', originPolicy);
}());
