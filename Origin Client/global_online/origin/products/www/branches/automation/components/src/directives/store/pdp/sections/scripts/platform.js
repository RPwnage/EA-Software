/**
 * @file store/pdp/sections/script/platform.js
 */
(function () {
    'use strict';


    function originStorePdpSectionsPlatform(ComponentsConfigFactory) {

        function originStorePdpSectionsPlatformLink(scope) {

            function getDownloadablePlatform(platformData, platform) {
                if(angular.isDefined(platformData.downloadStartDate)) {
                    return platform;
                }
                return  undefined;
            }

            var stopWatchingPlatforms = scope.$watch('platforms', function(newValue) {
                if (newValue) {
                    stopWatchingPlatforms();

                    if (_.isObject(newValue)) {
                        scope.platformArray = _.map(newValue, getDownloadablePlatform);
                    } else {
                        scope.platformArray = [newValue];
                    }
                }
            });
        }

        return {
            restrict: 'E',
            scope: {
                platformText: '@',
                platforms: '='
            },
            link: originStorePdpSectionsPlatformLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/sections/views/platform.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSectionsPlatform
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} platform-text (optional) platform label
     *
     * @example
     *
     * <origin-store-pdp-sections-platform platform="" platform-text=""></origin-store-pdp-sections-platform>
     *
     */
    angular.module('origin-components')
        .directive('originStorePdpSectionsPlatform', originStorePdpSectionsPlatform);
}());
