/**
 * @file socialmedia/scripts/twitter.js
 */
(function() {
    'use strict';

    function OriginSocialmediaTwitterCtrl($scope, AppCommFactory, $window, $timeout) {

        $scope.isLoading = true;

        function onData(data) {
            $scope.$evalAsync(function() {
                $scope.href = data.href;
                $scope.url = data.url;
                $scope.via = data.via;
                $scope.hashtags = data.hashtags;
                $scope.text = data.text;
                $scope.lang = data.lang;
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
                /* This uses the twitter api to scans the dom for twitter buttons and turns them info iframes */
                $window.twttr.widgets.load();
            }
        };

        function onDestroy() {
            AppCommFactory.events.off('seo:ready', onData);
        }

        /* TODO: This will come from a SEO component in the future */
        var tempData = {
            href: "https://twitter.com/share",
            url: "https://www.origin.com/en-us/store/buy/battlefront",
            via: "OriginInsider",
            hashtags: "OriginStore",
            text: "Check out Star Wars&trade; Battlefront&trade; Deluxe Edition",
            lang: "en"
        };
        /* TODO: Bind onData() to seo:ready.. just for testing.. */
        AppCommFactory.events.on('seo:ready', onData);
        /* TODO: Fire myself for now.. */
        AppCommFactory.events.fire('seo:ready', tempData);

        $scope.$on('$destroy', onDestroy);
    }

    function originSocialmediaTwitterLink(scope) {
        scope.update();
    }

    function originSocialmediaTwitter(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {},
            link: originSocialmediaTwitterLink,
            controller: 'OriginSocialmediaTwitterCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/socialmedia/views/twitter.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialmediaTwitter
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
     *         <origin-socialmedia-twitter></origin-socialmedia-twitter>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginSocialmediaTwitterCtrl', OriginSocialmediaTwitterCtrl)
        .directive('originSocialmediaTwitter', originSocialmediaTwitter);
}());