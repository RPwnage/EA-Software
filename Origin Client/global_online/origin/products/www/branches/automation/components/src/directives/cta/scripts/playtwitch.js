/**
 * @file cta/playtwitch.js
 */

(function() {
    'use strict';

    var CLS_ICON_PLAY = 'otkicon-play',
        DIALOG_ID = 'web-twitch-video';

    /**
     * A list of button type options
     * @readonly
     * @enum {string}
     */
    var ButtonTypeEnumeration = {
        "primary": "primary",
        "secondary": "secondary",
        "round": "round",
        "transparent": "transparent"
    };

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function OriginCtaPlayTwitchCtrl($scope, DialogFactory, ComponentsLogFactory) {
        $scope.iconClass = CLS_ICON_PLAY;
        $scope.type = $scope.type || ButtonTypeEnumeration.primary;

        $scope.onBtnClick = function($event) {
            // Emit an event that is picked up by OriginHomeDiscoveryTileProgrammableCtrl,
            // in order to record cta-click telemetry for the tile.
            $scope.$emit('cta:clicked', 'rich_media');
            
            $event.stopPropagation();
            DialogFactory.openDirective({
                id: DIALOG_ID,
                name: 'origin-dialog-content-twitch',
                size: 'large',
                data: {
                    'dialog-id': DIALOG_ID,
                    'channel': $scope.channel,
                    'video': $scope.video,
                    'clip': $scope.clip,
                    'mature-content': $scope.matureContent,
                    'ocd-path': $scope.ocdPath,
                    'content-text': $scope.contentText,
                    'content-link': $scope.contentLink,
                    'content-link-btn-text': $scope.contentLinkBtnText,
                    'hide-packart': $scope.hidePackart
                }
            });
            //load External url here with $scope.link
            ComponentsLogFactory.log('CTA: Playing Video');
        };

        // Attach and optional handler that can override the default behavior.
        var optionalHandler = $scope.handler();
        if (optionalHandler) {
            $scope.onBtnClick = optionalHandler;
        }
    }

    function originCtaPlayTwitch(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                channel: '@',
                video: '@',
                clip: '@',
                matureContent: '@',
                btnText: '@description',
                type: '@type',
                handler: '&handler',
                ocdPath: '@',
                contentLink: '@',
                contentLinkBtnText: '@',
                contentText: '@',
                hidePackart: '@'
            },
            controller: 'OriginCtaPlayTwitchCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaPlayTwitch
     * @restrict E
     * @element ANY
     * @scope
     * @param {String} channel the twitch channel id ("gosutv_ow")
     * @param {String} video the twitch video id, no channel must be specified ("v92486492")
     * @param {String} clip the twitch video id, no channel or video must be specified ("eleaguetv/ZealousMosquitoPeteZarollTie")
     * @param {BooleanEnumeration} mature-content Whether the video(s) defined within this component should be considered Mature
     * @param {LocalizedString} description the button text
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @param {void} [handler] function to execute when button is clicked.
     * @param {OcdPath} ocd-path (Optional) OCD path for pack art and CTA
     * @param {Url} content-link (Optional) URL that the CTA below the video will link to (overrides OCD path CTA)
     * @param {LocalizedString} content-link-btn-text (Optional) Content link CTA text
     * @param {LocalizedString} content-text (Optional) Text to display underneath the video for more information etc.
     * @param {BooleanEnumeration} hide-packart (Optional) Suppress pack art from showing when OCD path is provided

     * @description
     *
     * Play twitch channel/video/clip call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-play-twitch channel="gosutv_ow" video="v92486492" clip="eleaguetv/ZealousMosquitoPeteZarollTie" mature-content="true" description="Watch Hardline Video" type="primary"></origin-cta-play-twitch>
     *     </file>
     * </example>
     */

    angular.module('origin-components')
        .controller('OriginCtaPlayTwitchCtrl', OriginCtaPlayTwitchCtrl)
        .directive('originCtaPlayTwitch', originCtaPlayTwitch);
}());