/**
 * @file home/recommended/notloggedin.js
 */
(function() {
    'use strict';

    function OriginHomeRecommendedActionNotloggedinCtrl($scope, DialogFactory, ComponentsLogFactory) {

        /**
         *
         * TODO: Remove after first production run. It is a temporary function for first production run.
         *
         */

        $scope.onBtnClick = function() {
            ComponentsLogFactory.log('CTA: Going to PDP');
            DialogFactory.openAlert({
                id: 'web-going-to-pdp',
                title: 'Going to PDP',
                description: 'When the rest of OriginX is fully functional, you will be taken to the games pdp page.',
                rejectText: 'OK'
            });
        };

    }

    function originHomeRecommendedActionNotloggedin(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@image',
                title: '@title',
                subtitle: '@subtitle',
                description: '@description',
                subdescription: '@subdescription'
            },
            controller: 'OriginHomeRecommendedActionNotloggedinCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/notloggedin.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionNotloggedin
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl} image image of the tile
     * @param {LocalizedString} title title string
     * @param {LocalizedString} subtitle subtile on the tile
     * @param {LocalizedString} description description on the tile
     * @param {LocalizedString} subdescription sub description on the tile
     * @description
     *
     * not logged inrecommended next action
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-notloggedin image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/dai_intro_bg.png" titlestr="Discover. Play. Connect." descriptionstr="Origin is your PC Gaming platform. Play your favorite games, discover new content, and connect with friends. <a href='javascript:void(0);'>Learn More.</a>" subtitlestr="Featured" subdescriptionstr="<a href='javascript:void(0);'>Dragon Age: Inquisition</a>">
     *         </origin-home-recommended-action-notloggedin>
     *     </file>
     * </example>
     *
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionNotloggedinCtrl', OriginHomeRecommendedActionNotloggedinCtrl)
        .directive('originHomeRecommendedActionNotloggedin', originHomeRecommendedActionNotloggedin);
}());