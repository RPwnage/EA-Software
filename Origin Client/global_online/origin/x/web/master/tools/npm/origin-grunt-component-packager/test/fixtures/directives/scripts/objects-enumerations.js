/**
 * ValueEnumeration - a list of key:Object[]s
 *
 * @enum {Object}
 * @see http://www.json.org
 */
var ValueEnumeration = {
    "enumerationkey": {
        "foo": "bar",
        "baz": "bat"
    }
};

function OriginHomeRecommendedActionTrialCtrl($scope) {
    $scope.home = ValueEnumeration['foo'];
}

/**
 * @ngdoc directive
 * @name origin-components.directives:originObjectsInEnumerations
 * @restrict E
 * @element ANY
 * @scope
 * @param {ValueEnumeration} value an enumeration
 */
angular.module('testbed')
    .controller('OriginHomeRecommendedActionTrialCtrl', OriginHomeRecommendedActionTrialCtrl);
