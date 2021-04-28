
/** 
 * @file store/pdp/scripts/keybeat.js
 */ 
(function(){
    'use strict';

    function originStorePdpKeybeatCtrl($scope) {
        /**
         * Checks whether the keybeat is visible or not
         * @return {boolean}
         */
        $scope.isKeybeatVisible = function() {
            return ($scope.message !== '');
        };
    }

    function originStorePdpKeybeat(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                message: '@',
                icon: '@'
            },
            controller: 'originStorePdpKeybeatCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/keybeat.html'),
            replace: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpKeybeat
     * @restrict E
     * @element ANY
     * @scope
     * @description Merchandized keybeat icon and message
     * @param {string} icon icon to use
     * @param {LocalizedString} message text message
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-pdp-keybeat icon="originlogo" message="Own <a href=&quot;#&quot; class=&quot;otka&quot;>Premium Membership</a>? You're already good to go and will get early access." />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originStorePdpKeybeatCtrl', originStorePdpKeybeatCtrl)
        .directive('originStorePdpKeybeat', originStorePdpKeybeat);
}()); 
