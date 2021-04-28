/**
 * @file cta/playvideo.js
 */

(function() {
    'use strict';

    var CLS_ICON_PLAY = 'otkicon-play';

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

    function OriginCtaPlayvideoCtrl($scope, DialogFactory, ComponentsLogFactory) {
        $scope.iconClass = CLS_ICON_PLAY;
        $scope.type = $scope.type || ButtonTypeEnumeration.primary;

        $scope.onBtnClick = function($event) {
            $event.stopPropagation();
            DialogFactory.openDirective({
                id: 'web-youtube-video',
                name: 'origin-dialog-content-youtube',
                size: 'large',
                data: {
                    videoid: $scope.videoId,
                    restrictedAge: $scope.restrictedAge
                }
            });
            //load External url here with $scope.link
            ComponentsLogFactory.log('CTA: Playing Video');
        };
    }

    function originCtaPlayvideo(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                videoId: '@videoid',
                restrictedAge: '@restrictedage',
                btnText: '@description',
                type: '@type'
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
     * @param {string} videoid the video id of the youtube video
     * @param {Number} restrictedage The Restricted Age of the youtube video for this promo (Optional).
     * @param {LocalizedString} description the button text
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @description
     *
     * Play video call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-playvideo videoid="hlEKa6tUPlU" restrictedage="18" description="Watch Hardline Video" type="primary"></origin-cta-playvideo>
     *     </file>
     * </example>
     */

    angular.module('origin-components')
        .controller('OriginCtaPlayvideoCtrl', OriginCtaPlayvideoCtrl)
        .directive('originCtaPlayvideo', originCtaPlayvideo);
}());