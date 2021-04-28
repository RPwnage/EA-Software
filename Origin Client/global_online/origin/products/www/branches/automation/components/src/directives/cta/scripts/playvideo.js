/**
 * @file cta/playvideo.js
 */

(function() {
    'use strict';

    var CLS_ICON_PLAY = 'otkicon-play',
        DIALOG_ID = 'web-youtube-video';

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

    function OriginCtaPlayvideoCtrl($scope, DialogFactory, ComponentsLogFactory) {
        $scope.iconClass = CLS_ICON_PLAY;
        $scope.type = $scope.type || ButtonTypeEnumeration.primary;

        $scope.onBtnClick = function($event) {
            // Emit an event that is picked up by OriginHomeDiscoveryTileProgrammableCtrl,
            // in order to record cta-click telemetry for the tile.
            $scope.$emit('cta:clicked', 'rich_media');
            
            $event.stopPropagation();
            DialogFactory.openDirective({
                id: DIALOG_ID,
                name: 'origin-dialog-content-youtube',
                size: 'large',
                data: {
                    'dialog-id': DIALOG_ID,
                    'video-id': $scope.videoId,
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

    function originCtaPlayvideo(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                videoId: '@',
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
            controller: 'OriginCtaPlayvideoCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaPlayvideo
     * @restrict E
     * @element ANY
     * @scope
     * @param {Video} video-id the video id of the youtube video
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
     * Play video call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-playvideo video-id="hlEKa6tUPlU" mature-content="true" description="Watch Hardline Video" type="primary"></origin-cta-playvideo>
     *     </file>
     * </example>
     */

    angular.module('origin-components')
        .controller('OriginCtaPlayvideoCtrl', OriginCtaPlayvideoCtrl)
        .directive('originCtaPlayvideo', originCtaPlayvideo);
}());