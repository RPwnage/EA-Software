/**
 * Created by handerson on 2015-04-21.
 * @file cta.js
 */
(function () {

    'use strict';

    function OriginOfflineCtaCtrl($scope, $sce, UtilFactory) {

        $scope.trustAsHtml = function (html) {
            return $sce.trustAsHtml(html);
        };

        function createHyperlink(url, string) {
            return '<a class="otka" href=\"' + url + '\">' + string + '</a>';
        }

        var CONTEXT_NAMESPACE = 'origin-offline-cta';

        $scope.titleLoc = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'titlestr');

        if ($scope.descriptionOverrideStr && $scope.descriptionOverrideStr.length) {
            // If there is a description override use it
            $scope.descriptionLoc = $scope.descriptionOverrideStr;
        }
        else {
            // Go with our standard text
            $scope.descriptionLoc = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'descriptionoverridestr')
                .replace('%notavailable%', $scope.notAvailableStr)
                .replace('%gamelibrary%', createHyperlink('game-library', UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'gamelibrarystr')));
        }
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originOfflineCta
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} titlestr the title of cta
     * @param {LocalizedString} notavailablestr "Your Friends and Conversations are not available in offline mode."
     * @param {LocalizedString} descriptionoverridestr (optional) "Go Online to reconnect Origin."
     * @param {LocalizedString} gamelibrarystr "game library"
     * @param {string} buttontheme - "light"|"primary"|"transparent" (optional parameter - defaults to "light")
     * @description
     *
     * offline -> cta
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-offline-cta
     *            titlestr="title"
     *            notavailablestr="Your Friends and Conversations are not available in offline mode."
     *         ></origin-offline-cta>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginOfflineCtaCtrl', OriginOfflineCtaCtrl)
        .directive('originOfflineCta', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginOfflineCtaCtrl',
                scope: {
                    'titleStr': '@titlestr',
                    'notAvailableStr': '@notavailablestr',
                    'descriptionOverrideStr': '@descriptionoverridestr',
                    'buttonTheme': '@buttontheme'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('offline/views/cta.html')
            };
        });

}());
