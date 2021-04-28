/**
 * @file store/about/scripts/downloadoriginanchor.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */
    var AnchorIconEnumeration = {
        "otkicon-4k": "otkicon-4k",
        "otkicon-app": "otkicon-app",
        "otkicon-challenge": "otkicon-challenge",
        "otkicon-chatbubble": "otkicon-chatbubble",
        "otkicon-cloud": "otkicon-cloud",
        "otkicon-controller": "otkicon-controller",
        "otkicon-coop": "otkicon-coop",
        "otkicon-crossplatform": "otkicon-crossplatform",
        "icon-customerservice": "icon-customerservice",
        "otkicon-directx": "otkicon-directx",
        "otkicon-dolby": "otkicon-dolby",
        "otkicon-download": "otkicon-download",
        "otkicon-downloadnegative" :  "otkicon-downloadnegative",
        "otkicon-firstperson": "otkicon-firstperson",
        "otkicon-ggg": "otkicon-ggg",
        "otkicon-leaderboard": "otkicon-leaderboard",
        "otkicon-modding": "otkicon-modding",
        "otkicon-multiplayer": "otkicon-multiplayer",
        "otkicon-offlinemode": "otkicon-offlinemode",
        "otkicon-originlogo": "otkicon-originlogo",
        "otkicon-person": "otkicon-person",
        "otkicon-progressiveinstall": "otkicon-progressiveinstall",
        "otkicon-refresh": "otkicon-refresh",
        "otkicon-splitscreen": "otkicon-splitscreen",
        "otkicon-thirdperson": "otkicon-thirdperson",
        "otkicon-timer": "otkicon-timer",
        "otkicon-touch": "otkicon-touch",
        "otkicon-trophy": "otkicon-trophy",
        "otkicon-twitch": "otkicon-twitch"
    };
    /* jshint ignore:end */

    function originStoreAboutDownloadOriginAnchor(ComponentsConfigFactory) {

        function originStoreAboutDownloadOriginAnchorLink(scope, element, attributes, originStoreAboutAnchorCtrl) {
            originStoreAboutAnchorCtrl.listenToHashChangeEvents();
            originStoreAboutAnchorCtrl.registerAnchor('aboutDownloadOriginAnchor:added');
        }

        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                anchorName: '@',
                anchorIcon: '@'
            },
            controller: 'OriginStoreAboutAnchorCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/anchor.html'),
            link: originStoreAboutDownloadOriginAnchorLink,
            replace: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutDownloadOriginAnchor
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {AnchorIconEnumeration} anchor-icon icon for link (will appear in nav)
     * @description Creates a navigation point for deep-linking on a page, and emits its presence for inclusion on about nav.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-about-download-origin-anchor title-str="Friends Who Play" anchor-name="friends" anchor-icon="otkicon-timer"></origin-store-about-downloadorigin-anchor>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAboutDownloadOriginAnchor', originStoreAboutDownloadOriginAnchor);
}());
