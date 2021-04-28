/**
 * @file home/getstarted/tile.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-get-started-tile';

    function OriginHomeGetStartedTileCtrl($scope, GetStartedFactory, ComponentsConfigHelper, NavigationFactory, DialogFactory, UtilFactory) {
        var settingsChangedHandle = null;

        $scope.shouldShow = GetStartedFactory.checkSetting($scope.tile.name);

        $scope.titleLoc = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'titlestr');
        $scope.msgLoc = UtilFactory.getLocalizedStr($scope.msgStr, CONTEXT_NAMESPACE, 'msgstr');
        $scope.btnLoc = UtilFactory.getLocalizedStr($scope.btnStr, CONTEXT_NAMESPACE, 'btnstr');



        function settingChanged(name) {
            if (name === $scope.tile.name) {
                $scope.shouldShow = false;
            }
        }

        function nonDirectiveSupportTextVisible() {
            return ($scope.tile.title.length > 0) ||
                ($scope.tile.msg.length > 0);
        }

        function updateSupportTextVisible() {
            $scope.supportTextVisible = nonDirectiveSupportTextVisible();
        }

        $scope.remindLater = function() {
            GetStartedFactory.dismiss($scope.tile.name, new Date().getTime());
        };

        $scope.onPrivacyClicked = function() {
            if (!!$scope.tile.msgHref) {
                Origin.windows.externalUrl($scope.tile.msgHref);
            }
        };

        $scope.onContactClicked = function() {
            DialogFactory.openPrompt({
                id: 'get-started-tile-contact-ea-id',
                name: 'get-started-tile-contactEA',
                title: $scope.titleLoc,
                description: $scope.msgLoc,
                acceptText: '',
                acceptEnabled: false,
                rejectText: $scope.btnLoc,
                defaultBtn: 'yes'
            });
        };

        $scope.openUrlSSO = function() {
            if ($scope.tile.href) {
                var url = ComponentsConfigHelper.getUrl($scope.tile.href);
                if (!Origin.client.isEmbeddedBrowser()){
                    url = url.replace("{locale}", Origin.locale.locale());
                    url = url.replace("{access_token}", "");
                }
                NavigationFactory.asyncOpenUrlWithEADPSSO(url);
            }
        };

        this.handleDismiss = function() {
            //Send telemetry when the tile's cta is clicked
            var source = Origin.client.isEmbeddedBrowser() ? 'client' : 'web',
                bucketTitle = _.get($scope, ['$parent', '$parent', '$parent', '$parent', 'titleLoc']) || '';

            Origin.telemetry.sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'message', source, 'success', {
                'type': 'in-game',
                'service': 'originx',
                'format': 'live_tile',
                'client_state': 'foreground',
                'msg_id': $scope.tile.ctid? $scope.tile.ctid: $scope.tile.image,
                'status': 'click',
                'content': $scope.msgLoc,
                'destination_name': bucketTitle
            });

            if ($scope.tile.externalLink === 'true') {
                // Dismiss it now
                $scope.openUrlSSO();
                GetStartedFactory.dismiss($scope.tile.name);
            }
            else {
                if ($scope.tile.tileAction) {
                    $scope.tile.tileAction();
                }
                return true;
            }
            return false;
        };

        this.factoryDismiss = function() {
            GetStartedFactory.dismiss($scope.tile.name);
        };

        updateSupportTextVisible();
        settingsChangedHandle = GetStartedFactory.events.on('getstarted:settingChanged', settingChanged);

        $scope.$on('$destroy', function() {
            if(settingsChangedHandle) {
                settingsChangedHandle.detach();
            }
        });
    }

    function originHomeGetStartedTile($compile, $timeout, ComponentsConfigFactory, HtmlTransformer) {

        function originHomeGetStartedTileLink(scope, elem, attrs, ctrl) {
            var tileDescription = _.get(scope, 'tile.msg');
            if (tileDescription) {
                var transformedHtml = HtmlTransformer.addOtkClasses(tileDescription);
                elem.find('.origin-tile-support-text-paragraph').append($compile('<span>'+transformedHtml+'</span>')(scope));
            }

            scope.dismiss = function () {
                if (ctrl.handleDismiss()) {
                    // Show confirmation before dismissing
                    elem[0].classList.add('confirmed');
                    $timeout(function () {
                        // Dismiss after 4 seconds
                        ctrl.factoryDismiss();
                    }, 4000, false);
                }
            };

        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                tile: '=',
                btnStr: '@btnstr',
                titleStr: '@titlestr',
                msgStr: '@msgstr'

            },
            controller: 'OriginHomeGetStartedTileCtrl',
            link: originHomeGetStartedTileLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/getstarted/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeGetStartedTile
     * @restrict E
     * @element ANY
     * @scope
     * @param {object} tile the data needed for the tile to display properly
     * @param {LocalizedString} titlestr the title text for the dialog
     * @param {LocalizedText} msgstr the subtitle text for the dialog
     * @param {LocalizedString} btnstr the link text for the dialog
     * @description
     *
     * get started
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *        <origin-getstarted-tile></origin-getstarted-tile>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeGetStartedTileCtrl', OriginHomeGetStartedTileCtrl)
        .directive('originHomeGetStartedTile', originHomeGetStartedTile);
}());
