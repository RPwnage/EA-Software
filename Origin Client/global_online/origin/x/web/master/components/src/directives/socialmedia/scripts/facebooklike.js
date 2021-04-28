/**
 * @file socialmedia/scripts/facebooklike.js
 */
(function() {
    'use strict';

    function OriginSocialmediaFacebooklikeCtrl($scope, AppCommFactory, $window, $timeout) {

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

    function originSocialmediaFacebooklikeLink(scope) {
        scope.update();
    }

    function originSocialmediaFacebooklike(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {},
            link: originSocialmediaFacebooklikeLink,
            controller: 'OriginSocialmediaFacebooklikeCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/socialmedia/views/facebooklike.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialmediaFacebooklike
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
     *         <origin-socialmedia-facebooklike></origin-socialmedia-facebooklike>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginSocialmediaFacebooklikeCtrl', OriginSocialmediaFacebooklikeCtrl)
        .directive('originSocialmediaFacebooklike', originSocialmediaFacebooklike);
}());