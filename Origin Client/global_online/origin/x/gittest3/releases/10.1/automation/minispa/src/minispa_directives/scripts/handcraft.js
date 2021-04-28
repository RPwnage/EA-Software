( function() {
        'use strict';

        function minispaHandcraftCtrl($scope, $http, $compile, $element) {

            $scope.compileStr = function(directiveStr) {
                var testContainer = '<div class="resizable_container">';
                testContainer += '<div class="container_body">';
                testContainer += directiveStr;
                testContainer += '</div>';
                testContainer += '</div>';
                $element.find('#hand-craft-container').append($compile(testContainer)($scope));
            };

            $scope.compileUrl = function(directiveUrl) {
                $http.get(directiveUrl).success(function(data) {
                    console.log(directiveUrl);
                    var testContainer = '<div class="resizable_container">';
                    testContainer += '<div class="container_body">';
                    testContainer += data;
                    testContainer += '</div>';
                    testContainer += '</div>';
                    $element.find('#hand-craft-container').append($compile(testContainer)($scope));
                }).error(function(data, status) {
                    console.error('$http.get()', directiveUrl, status, data);
                });
            };

        }

        function minispaHandcraft() {

            return {
                restrict : 'E',
                transclude : true,
                controller : 'minispaHandcraftCtrl',
                templateUrl : 'minispa_directives/views/handcraft.html'
            };

        }


        angular.module('testApp')
            .controller('minispaHandcraftCtrl', minispaHandcraftCtrl)
            .directive('minispaHandcraft', minispaHandcraft);

    }());
