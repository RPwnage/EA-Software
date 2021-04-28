( function() {
        'use strict';

        function minispaAutomationTestdataCtrl($scope, $http, $compile, $element) {

            this.render = function(source) {

                console.log('loading directives');

                var url = 'minispa_data/default.xml';
                if (source) {
                    url = source;
                }

                console.log('automation test data source: ' + url);

                $http.get(url).success(function(data) {
                    $element.find('#test-data-container').append($compile(data)($scope));
                }).error(function(data, status) {
                    console.error('$http.get()', url, status, data);
                });
            };

        }

        function minispaAutomationTestdata() {

            function minispaAutomationTestdataLink($scope, $element, args, ctrl) {

                ctrl.render($scope.source);

            }

            return {
                restrict : 'E',
                scope : {
                    source : '@'
                },
                transclude : true,
                controller : 'minispaAutomationTestdataCtrl',
                templateUrl : 'minispa_directives/views/automationtestdata.html',
                link : minispaAutomationTestdataLink
            };

        }


        angular.module('testApp')
            .controller('minispaAutomationTestdataCtrl', minispaAutomationTestdataCtrl)
            .directive('minispaAutomationTestdata', minispaAutomationTestdata);

    }());
