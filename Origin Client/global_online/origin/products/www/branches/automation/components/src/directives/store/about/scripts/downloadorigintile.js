/**
 * @file store/about/scripts/downloadorigintile.js
 */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-about-downloadorigintile';

    /* jshint ignore:start */
    var PlatformEnumeration = {
        "windows": "windows",
        "mac": "mac"
    };
    /* jshint ignore:end */

    /* Directive declaration*/
    function originStoreAboutDownloadorigintile(ComponentsConfigFactory, UtilFactory, BeaconFactory) {

        /* Directive Link */
        function originStoreAboutDownloadorigintileLink(scope){
            scope.platformTitle = UtilFactory.getLocalizedStr(scope.titleStr, CONTEXT_NAMESPACE, scope.platform + 'title');
            scope.platformDownloadText = UtilFactory.getLocalizedStr(scope.downloadButtonText, CONTEXT_NAMESPACE, 'download-button-text');
            scope.showDownloadCta = true;

            function handleBeaconResponse(data) {
                scope.showDownloadCta = data;

                if((Origin.utils.os() === Origin.utils.OS_WINDOWS && scope.platform === 'windows') || (Origin.utils.os() === Origin.utils.OS_MAC && scope.platform === 'mac')) {
                    scope.ctaColor = 'primary';
                } else {
                    scope.ctaColor = 'light';
                }
            }

            BeaconFactory
                .isInstallable()
                .then(handleBeaconResponse);
        }

        return {
            restrict: 'E',
            scope: {
                platform: '@',
                titleStr: '@',
                systemRequirements: '@',
                downloadButtonText: '@',
                downloadUrl: '@'
            },
            replace: true,
            link: originStoreAboutDownloadorigintileLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/downloadorigintile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutDownloadorigintile
     * @restrict E
     * @element ANY
     * @scope
     * @param {PlatformEnumeration} platform desc
     * @param {LocalizedString} title-str (optional) defaults to Windows or Mac
     * @param {LocalizedText} system-requirements min system requirements (HTML)
     * @param {Url} download-url download url
     * @param {LocalizedString} download-button-text (optional) defaults to Download
     * @param {LocalizedString} mactitle Mac title*update in defaults
     * @param {LocalizedString} windowstitle Windows title *update in defaults
     *
     * @description Creates a box witha download link to download origin for a specific platform
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-about-downloadorigintile title-str="Windows" platform="windows" system-requirement="" download-button-text="Download" donwload-url="someurl"></origin-store-about-downloadorigintile>
     *     </file>
     * </example>
     */

    angular.module('origin-components')
        .directive('originStoreAboutDownloadorigintile', originStoreAboutDownloadorigintile);
}());
