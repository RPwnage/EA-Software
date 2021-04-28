/**
 * @file home/newuser/tile.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-newuser-tile';

    function OriginHomeNewuserTileCtrl($scope, SettingsFactory, LocFactory, UtilFactory, ComponentsLogFactory) {
        function checkSeenSetting(settingsData) {
             /* jshint camelcase: false */
            $scope.show = !settingsData.new_user_home_seen;
        }

        function retrieveSettingError(error) {
            ComponentsLogFactory.error('[origin-home-newuser-tile]:retrieveSettings FAILED:', error.message);
            $scope.show = true;
        }

        $scope.dismiss = function() {
            $scope.class = 'origin-home-newuser-isclosed';
            SettingsFactory.setGeneralSetting('new_user_home_seen', true);
        };

        $scope.title = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'title');
        $scope.subtitle = UtilFactory.getLocalizedStr($scope.subtitleStr, CONTEXT_NAMESPACE, 'subtitle');
        $scope.linkcta = UtilFactory.getLocalizedStr($scope.linkctaStr, CONTEXT_NAMESPACE, 'linkcta');
        $scope.show = false;

        SettingsFactory.getGeneralSettings()
            .then(checkSeenSetting)
            .catch(retrieveSettingError);
    }

    function originHomeNewuserTile(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                titleStr: '@title',
                subtitleStr: '@subtitle',
                linkctaStr: '@linkcta'
            },
            controller: 'OriginHomeNewuserTileCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/newuser/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeNewuserTile
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title the title text
     * @param {LocalizedText} subtitle the subtitle text
     * @param {LocalizedString} linkcta the link text
     * @description
     *
     * new user
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-newuser-tile></origin-home-newuser-tile>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeNewuserTileCtrl', OriginHomeNewuserTileCtrl)
        .directive('originHomeNewuserTile', originHomeNewuserTile);
}());