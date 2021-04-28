(function () {

    'use strict';

    var CONTEXT_NAMESPACE = 'origin-achievements-self';

    /**
    * The controller
    */
    function OriginAchievementsSelfCtrl($scope, UtilFactory, AuthFactory, AchievementFactory, ObjectHelperFactory, ComponentsLogFactory) {

        var localize = UtilFactory.getLocalizedStrGenerator(CONTEXT_NAMESPACE);

        $scope.isLoading = true;

        /* Get Translated Strings */
        $scope.overallProgressLoc = localize($scope.overallProgressStr, 'overallprogress');
        $scope.achievementsCompletedLoc = localize($scope.achievementsCompletedStr, 'achievementscompleted');
        $scope.pointsCompletedLoc = localize($scope.pointsCompletedStr, 'pointscompleted');
        $scope.noGamesSelfTitleLoc = localize($scope.noGamesSelfTitleStr, 'nogamesselftitle');
        $scope.noGamesSelfTextLoc = localize($scope.noGamesSelfTextStr, 'nogamesselftext');

        /**
         * Update achievementSets
         * @method setAchievementsData
         */
        function setAchievementsData(achievementData) {
            $scope.nucleusId = Origin.user.userPid();
            $scope.personaId = Origin.user.personaId();
            $scope.achievementSets = achievementData.achievementSets;
            $scope.score = {
                grantedAchievements : achievementData.grantedAchievements(),
                totalAchievements : achievementData.totalAchievements(),
                grantedXp : achievementData.grantedXp(),
                totalXp : achievementData.totalXp()
            };
            $scope.isLoading = false;
            $scope.$digest();
        }

        /**
         * Get Achievement Portfolio from factory
         * @method getAchievementPortfolio
         */
        function getAchievementPortfolio() {
            AchievementFactory.getAchievementPortfolio()
                .then(setAchievementsData)
                .catch(function(error) {
                    ComponentsLogFactory.error('[Origin-Achievement-Self-Directive getAchievementportfolio Method] error', error);
                });
        }

        /**
         * Reset values on authChange
         * @method onAuthChange
         */
        function onAuthChange() {
            $scope.achievementSets = {};
            $scope.score = null;
            $scope.nucleusId = '';
            $scope.personaId = '';
        }

        /**
         * Unhook from auth factory events when directive is destroyed.
         * @method onDestroy
         */
        function onDestroy() {
            AuthFactory.events.off('myauth:change', onAuthChange);
            AuthFactory.events.off('myauth:ready', getAchievementPortfolio);
            AchievementFactory.events.off('achievements:achievementGranted', getAchievementPortfolio);
        }

        /* Bind to auth events */
        AuthFactory.events.on('myauth:change', onAuthChange);
        AuthFactory.events.on('myauth:ready', getAchievementPortfolio);

        //Update page from dirtybits response
        AchievementFactory.events.on('achievements:achievementGranted', getAchievementPortfolio);

        $scope.$on('$destroy', onDestroy);

        AuthFactory.waitForAuthReady()
            .then(getAchievementPortfolio)
            .catch(function(error) {
                ComponentsLogFactory.error('[Origin-Achievements-Otheruser-Directive AuthReady] error', error);
            });
    }

    /**
     * Achievements Self Directive
     */
    function originAchievementsSelf(ComponentsConfigFactory) {

        function originAchievementsSelfLink() {
            angular.element('html, body').animate({ scrollTop: 0 }, 'fast');
        }

        return {
            restrict: 'E',
            controller: 'OriginAchievementsSelfCtrl',
            scope: {
                overallProgressStr: '@overallprogress',
                achievementsCompletedStr: '@achievementscompleted',
                pointsCompletedStr: '@pointscompleted',
                noGamesSelfTitleStr: '@nogamesselftitle',
                noGamesSelfTextStr: '@nogamesselftext'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('achievements/views/achievements-self.html'),
            link: originAchievementsSelfLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originAchievementsSelf
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} overallprogress "Overall Achievement Progress"
     * @param {LocalizedString} achievementscompleted "Achievements Completed"
     * @param {LocalizedString} pointscompleted "Origin points earned"
     * @param {LocalizedString} nogamesselftitle "There's nowhere to go but up."
     * @param {LocalizedString} nogamesselftext "None of your current games offer achievements."
     * @description
     *
     * Overall Achievement Progress of the logged in user
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-achievements-self></origin-achievements-self>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginAchievementsSelfCtrl', OriginAchievementsSelfCtrl)
        .directive('originAchievementsSelf', originAchievementsSelf);

}());