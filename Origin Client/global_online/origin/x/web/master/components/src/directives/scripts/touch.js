
/**
 * @file /scripts/touch.js
 */
(function(){
    'use strict';
    function originTouch(ComponentsConfigFactory, UtilFactory, $parse, $rootScope) {

        function originTouchLink(scope, elem, attrs) {

            /**
             * If the device is touch enabled attach as the event to the element.
             * The callback provide behaves the same ng-click would.
             */
            if(UtilFactory.isTouchEnabled()) {

                var fn = $parse(attrs.originTouch,/* interceptorFn */ null, /* expensiveChecks */ true);

                elem.on('click', function(event){
                    var callback = function() {
                        fn(scope, {$event:event});
                    };
                    if($rootScope.$$phase) {
                        scope.$evalAsync(callback);
                    } else {
                        scope.$apply(callback);
                    }
                });
            }
        }
        return {
            restrict: 'A',
            link: originTouchLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originOtouch
     * @restrict
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <div origin-touch="myCallback()> </div>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originTouch', originTouch);
}());
