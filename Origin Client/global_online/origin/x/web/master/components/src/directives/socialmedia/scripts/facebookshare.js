/**
 * @file socialmedia/scripts/facebookshare.js
 */
(function() {
    'use strict';

    function OriginSocialmediaFacebookshareCtrl($scope, AppCommFactory, $window, $timeout) {

        $scope.isLoading = true;

        function onData(data) {
            $scope.$evalAsync(function() {
                $scope.url = data.url;
                $scope.isLoading = false;
            });
        }

        $scope.update = function() {
            if ($scope.isLoading)
            {
                $timeout(function() {
                    $scope.update();
                },500);
            }
            else
            {
                $window.FB.XFBML.parse();
            }
        };

        function onDestroy() {
            AppCommFactory.events.off('seo:ready', onData);
        }

        /* TODO: This will come from a SEO component in the future */
        var tempData = {
            url: "https://www.origin.com/en-us/store/buy/battlefront",
        };
        /* TODO: Bind onData() to seo:ready.. just for testing.. */
        AppCommFactory.events.on('seo:ready', onData);
        /* TODO: Fire myself for now.. */
        AppCommFactory.events.fire('seo:ready', tempData);

        $scope.$on('$destroy', onDestroy);
    }

    function originSocialmediaFacebookshareLink(scope) {
        scope.update();
    }

    function originSocialmediaFacebookshare(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {},
            link: originSocialmediaFacebookshareLink,
            controller: 'OriginSocialmediaFacebookshareCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/socialmedia/views/facebookshare.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialmediaFacebookshare
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Social Media directive to hold buttons
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-socialmedia-facebookshare></origin-socialmedia-facebookshare>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginSocialmediaFacebookshareCtrl', OriginSocialmediaFacebookshareCtrl)
        .directive('originSocialmediaFacebookshare', originSocialmediaFacebookshare);
}());