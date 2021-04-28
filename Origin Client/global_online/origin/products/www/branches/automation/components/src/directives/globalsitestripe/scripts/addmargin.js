
/**
 * @file globalsitestripe/scripts/addmargin.js
 */
(function(){
    'use strict';

    function originAddSitestripeMargin(GlobalSitestripeFactory, ScreenFactory, ComponentsConfigHelper, $parse) {
        function originAddSitestripeMarginLink(scope, elem, attrs) {
            var shouldOffsetHeight = attrs.originAddSitestripeMarginOffsetHeight === 'true';
            var removeStripeMargin;
            var margin = 0;

            function registerCallback(callback) {
                if (!angular.isDefined(ComponentsConfigHelper.getOIGContextConfig())) {
                    onDestroy();
                    removeStripeMargin = GlobalSitestripeFactory.addStripeMargin(callback);
                }
            }

            function offsetHeight() {
                if (margin > 0) {
                    var body = angular.element('body');
                    var offsetPercent = (margin / body.height()) * 100;
                    body.css('height', (100 - offsetPercent).toString() + '%');
                } else {
                    angular.element('body').css('height', '100%');
                }
            }

            if (shouldOffsetHeight) {
                scope.$watch(function() { 
                        return $(document).height();
                }, offsetHeight);
            }

            scope.defaultSetMargin = function(newMargin) {
                margin = newMargin;

                if (shouldOffsetHeight) {
                    offsetHeight();
                }

                if (!attrs.originAddSitestripeMargin || (attrs.originAddSitestripeMargin === 'mobile' && ScreenFactory.isSmall())) {
                    elem.css('margin-top', margin + 'px');
                } else {
                    elem.css('margin-top', '0');
                }
            };

            // Determine what to call when fixing the margin.
            // defaultSetMargin simply adds the margin. For more complicated behaviors looks for
            // a "set-margin" attribute that describes a function.
            if (attrs.hasOwnProperty("setMargin")) {
                if (scope.hasOwnProperty(attrs.setMargin)) {
                    var externalCallback = $parse(attrs.setMargin)(scope);
                    registerCallback(externalCallback);
                } else {
                    // If the function was requested listen to scope to see if its added.
                    // Commonnly executes when the requested function is in the link function
                    // of the parent directive.
                    scope.$watch(attrs.setMargin, function() {
                        var externalCallback = $parse(attrs.setMargin)(scope);
                        registerCallback(externalCallback);
                    });
                }
            } else {
                registerCallback(scope.defaultSetMargin);
            }

            function onDestroy() {
                if (removeStripeMargin) {
                    removeStripeMargin();
                }
            }

            scope.$on('$destroy', onDestroy);
        }
        return {
            restrict: 'A',
            link: originAddSitestripeMarginLink
        };
    }
     /**
     * @ngdoc directive
     * @name origin-components.directives:originAddSitestripeMargin
     * @restrict A
     * @description
     * Global site stripe margin add/remove interface. Optional set-margin parameter to override default
     * behavior which will add a margin.
     *
     * @example
     * <example module="origin-components" origin-add-sitestripe-margin set-margin="function">
     * </example>
     */
    angular.module('origin-components')
        .directive('originAddSitestripeMargin', originAddSitestripeMargin);
}());
