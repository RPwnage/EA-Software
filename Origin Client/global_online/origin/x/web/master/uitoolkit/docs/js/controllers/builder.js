angular.module('app')
    .controller('BuilderCtrl', function($scope) {
        $scope.createJSON = function() {
            var markup = "";
            if ($scope.html) {
                markup = $scope.html.replace(/"/g, '\\"');
                markup = markup.replace(/'/g, '\\"');
                markup = markup.replace(/\n/g, '\\n');
            }
            if (markup) {
                $scope.json = '{"html": "' + markup + '"}';
            }
        };
    });