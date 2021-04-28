( function() {
        'use strict';

        function stripe(str) {
            return str.replace(/[^a-z0-9]/ig, '');
        }

        function minispaTestdataCtrl($scope, $http, $compile, $element) {

            console.log('loading directives');

            $http.get('minispa_data/test-xml.json').success(function(data) {
                console.log('TR: data: ' + data.length);
                for (var i = 0; i < data.length; ++i) {

                    // create and append container to base
                    var testContainer = '<div class="resizable_container">';
                    testContainer += '<div class="container_header">';
                    testContainer += data[i].name;
                    testContainer += '</div>';
                    // generate id for inserting directive later
                    testContainer += '<div id="' + stripe(data[i].url) + '" class="container_body"></div>';
                    testContainer += '</div>';

                    $element.find('#test-data-container').append($compile(testContainer)($scope));

                    insertDirectives(data, i);
                }
            });

            function insertDirectives(data, i) {
                $http.get(data[i].url).then(function(msg) {
                    console.log(msg.config.url);
                    var items = $.grep(data, function(e) {
                        return e.url === msg.config.url;
                    });

                    // insert directive based on id
                    $element.find('#' + stripe(items[0].url)).append($compile(msg.data)($scope));
                });
            };

        }

        function minispaTestdata() {

            return {
                restrict : 'E',
                transclude : true,
                controller : 'minispaTestdataCtrl',
                templateUrl : 'minispa_directives/views/testdata.html'
            };

        }


        angular.module('testApp')
            .controller('minispaTestdataCtrl', minispaTestdataCtrl)
            .directive('minispaTestdata', minispaTestdata);

    }());
