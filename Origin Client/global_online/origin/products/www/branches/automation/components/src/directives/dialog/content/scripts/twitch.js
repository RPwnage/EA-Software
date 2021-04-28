/**
 * @file dialog/content/twitch.js
 */
(function() {
    'use strict';

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var CONTEXT_NAMESPACE = 'origin-dialog-content-youtube';

    function OriginDialogContentTwitchCtrl($scope, $sce, AppCommFactory, NavigationFactory, UrlHelper, UtilFactory, ScopeHelper) {
        $scope.trustedSrc = "";
        $scope.restrictedMessage = UtilFactory.getLocalizedStr($scope.restrictedMessage, CONTEXT_NAMESPACE, 'restrictedmessagestr');
        $scope.showPackart = $scope.hidePackart !== BooleanEnumeration.false;

        $scope.strings = {
            contentLink: UtilFactory.getLocalizedStr($scope.contentLink, CONTEXT_NAMESPACE, 'content-link'),
            contentLinkBtnText: UtilFactory.getLocalizedStr($scope.contentLinkBtnText, CONTEXT_NAMESPACE, 'content-link-btn-text'),
            contentText: UtilFactory.getLocalizedStr($scope.contentText, CONTEXT_NAMESPACE, 'content-text')
        };

        function isValidString(string) {
            return angular.isDefined(string) && _.isString(string) && string.length > 0;
        }

        if (isValidString($scope.channel)) {
            $scope.trustedSrc = UrlHelper.buildTwitchChannelEmbedUrl($scope.channel, false);
        } else if (isValidString($scope.video)) {
            $scope.trustedSrc = UrlHelper.buildTwitchVideoEmbedUrl($scope.video, false);
        } else if (isValidString($scope.clip)) {
            $scope.trustedSrc = UrlHelper.buildTwitchClipEmbedUrl($scope.clip, false);
        }

        $scope.trustedSrc = $sce.trustAsResourceUrl($scope.trustedSrc);

        //we can revist this later, possibly how we show videos in general
        function setVideo() {
            $('#vidplayer').attr('src', '');
        }

        function onDestroy() {
            AppCommFactory.events.off('dialog:hidden', setVideo);
        }

        AppCommFactory.events.on('dialog:hidden', setVideo);
        $scope.$on('$destroy', onDestroy);

        this.processContentLink = function(){
            if ($scope.contentLink) {
                $scope.contentLinkAbsoluteUrl = NavigationFactory.getAbsoluteUrl($scope.contentLink);
                $scope.isContentLinkExternal = UtilFactory.isAbsoluteUrl($scope.contentLink);
            }
        };

        this.onScopeResolved = function() {
            this.processContentLink();
            ScopeHelper.digestIfDigestNotInProgress($scope);
        };
    }

    function originDialogContentTwitch(ComponentsConfigFactory, DialogFactory, DirectiveScope, NavigationFactory) {

        function originDialogContentTwitchLink (scope, elem, attrs, OriginDialogContentYoutubeCtrl) {

            scope.closeDialog = function () {
                DialogFactory.close(scope.dialogId);
            };

            scope.$on('cta:clicked', scope.closeDialog);

            scope.openLink = function (href, $event) {
                $event.preventDefault();
                scope.closeDialog();
                NavigationFactory.openLink(href);
            };

            if (scope.ocdPath) {
                DirectiveScope.populateScopeWithOcdAndCatalog(scope, CONTEXT_NAMESPACE, scope.ocdPath)
                    .then(OriginDialogContentYoutubeCtrl.onScopeResolved);
            }else{
                OriginDialogContentYoutubeCtrl.processContentLink();
            }
        }

        return {
            restrict: 'E',
            scope: {
                channel: '@',
                video: '@',
                clip: '@',
                matureContent: '@',
                ocdPath: '@',
                contentLink: '@',
                contentLinkBtnText: '@',
                contentText: '@',
                hidePackart: '@'
            },
            controller: 'OriginDialogContentTwitchCtrl',
            link: originDialogContentTwitchLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/twitch.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentTwitch
     * @restrict E
     * @element ANY
     * @scope
     * @param {String} channel the twitch channel id ("gosutv_ow")
     * @param {String} video the twitch video id, no channel must be specified ("v92486492")
     * @param {String} clip the twitch video id, no channel or video must be specified ("eleaguetv/ZealousMosquitoPeteZarollTie")
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {OcdPath} ocd-path OCD path for pack art and CTA
     * @param {Url} content-link (Optional) URL that the CTA below the video will link to (overrides OCD path CTA)
     * @param {LocalizedString} content-link-btn-text (Optional) Content link CTA text
     * @param {LocalizedString} content-text (Optional) Text to display underneath the video for more information etc.
     * @param {BooleanEnumeration} hide-packart (Optional) Suppress pack art from showing when OCD path is provided
     * @description
     *
     * twitch video dialog
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-twitch
     *             channel="gosutv_ow"
     *             video="v92486492"
     *             clip="eleaguetv/ZealousMosquitoPeteZarollTie"
     *             mature-content="true">
     *         </origin-dialog-content-twitch>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginDialogContentTwitchCtrl', OriginDialogContentTwitchCtrl)
        .directive('originDialogContentTwitch', originDialogContentTwitch);

}());