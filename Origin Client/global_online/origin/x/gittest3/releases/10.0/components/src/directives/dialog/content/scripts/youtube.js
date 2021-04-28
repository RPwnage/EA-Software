/**
 * @file dialog/content/youtube.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-youtube';

    function OriginDialogContentYoutubeCtrl($scope, $sce, AppCommFactory, ComponentsUrlsFactory, UtilFactory) {

        $scope.trustedSrc = ComponentsUrlsFactory.getYouTubeEmbedUrl($scope.videoId);
        $scope.restrictedMessage = UtilFactory.getLocalizedStr($scope.restrictedMessage, CONTEXT_NAMESPACE, 'restrictedmessagestr');
        $scope.trustedSrc = $sce.trustAsResourceUrl($scope.trustedSrc);

        //we can revist this later, possibly how we show youtube videos in general
        function setVideo() {
            var video = $('#vidplayer').attr('src');
            $('#vidplayer').attr('src', '');
            $('#vidplayer').attr('src', video);
        }

        function onDestroy() {
            AppCommFactory.events.off('dialog:hidden', setVideo);
        }

        AppCommFactory.events.on('dialog:hidden', setVideo);
        $scope.$on('$destroy', onDestroy);

    }

    function originDialogContentYoutube(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                videoId: '@videoid',
                restrictedAge: '@restrictedage'
            },
            controller: 'OriginDialogContentYoutubeCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/youtube.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentYoutube
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} videoid the video id of the youtube video
     * @param {Number} restrictedage restricted age for a user
     * @description
     *
     * youtube video dialog
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-youtube
     *             videoid="hlEKa6tUPlU"
     *             restrictedage="18">
     *         </origin-dialog-content-youtube>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginDialogContentYoutubeCtrl', OriginDialogContentYoutubeCtrl)
        .directive('originDialogContentYoutube', originDialogContentYoutube);

}());