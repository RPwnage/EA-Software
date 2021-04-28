
/**
 * @file /scripts/touch.js
 */
(function(){
    'use strict';
    function originTouch(UtilFactory, $parse) {

        function originTouchLink(scope, elem, attrs) {
            var fn = $parse(attrs.originTouch,/* interceptorFn */ null, /* expensiveChecks */ true);

            function handleEvent(event) {

                scope.$evalAsync(function() {
                    fn(scope, {$event:event});
                });
            }

            function onDestroy() {
                elem.off('click', handleEvent);
            }

            /**
             * If the device is touch enabled attach as the event to the element.
             * The callback provide behaves the same ng-click would.
             */
            if(UtilFactory.isTouchEnabled()) {
                elem.on('click', handleEvent);
                scope.$on('$destroy', onDestroy);
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
