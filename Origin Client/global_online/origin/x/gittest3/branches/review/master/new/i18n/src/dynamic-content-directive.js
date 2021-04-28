(function() {
    'use strict';

    angular.module('origin-i18n')
        .directive('dynamicContent', function($parse, $compile) {
            return {
                restrict: 'A',
                link: function(scope, element) {
                    var contentKey = element.attr('dynamic-content');
                    scope.$watch(contentKey, function() {
                        element.html($parse(contentKey)(scope));
                        $compile(element.contents())(scope);
                    }, true);
                }
            };
        });
}());
