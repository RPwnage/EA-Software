(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-achievements-otheruser';

    /**
    * The controller
    */
    function OriginAchievementsOtheruserCtrl($scope, UtilFactory, AppCommFactory, UserDataFactory, AchievementFactory, AuthFactory, ComponentsLogFactory) {

        var localize = UtilFactory.getLocalizedStrGenerator(CONTEXT_NAMESPACE);

        $scope.isLoading = true;
        $scope.retry = false;

        /* Get Translated Strings */
        $scope.overallProgressLoc = localize($scope.overallProgressStr, 'overallprogress');
        $scope.achievementsCompletedLoc = localize($scope.achievementsCompletedStr, 'achievementscompleted');
        $scope.pointsCompletedLoc = localize($scope.pointsCompletedStr, 'pointscompleted');
        $scope.noPeekingLoc = localize($scope.noPeekingStr, 'nopeeking');
        $scope.noGamesOtherTitleLoc = localize($scope.noGamesOtherTitleStr, 'nogamesothertitle');
        $scope.retryErrorTitleLoc = localize($scope.retryErrorTitleStr, 'retryerrortitle');
        $scope.retryErrorDescLoc = localize($scope.retryErrorDescStr, 'retryerrordesc');
        $scope.retryErrorCTALoc = localize($scope.retryErrorCTAStr, 'retryerrorcta');

        /**
         * Set Users privacy and achievementSet
         * @method setUserPortfolio
         */
        function setUserPortfolio(achievementData, userData) {
            $scope.isPrivate = achievementData.hidden;
            if(achievementData.hidden) {
                $scope.achievementsPrivateLoc = UtilFactory.getLocalizedStr($scope.achievementsPrivateStr, CONTEXT_NAMESPACE, 'achievementsprivate', {
                    '%OriginId%': userData[0].EAID
                });
            }
            else {
                $scope.noGamesOtherTextLoc = UtilFactory.getLocalizedStr($scope.noGamesOtherTextStr, CONTEXT_NAMESPACE, 'nogamesothertext', {
                    '%OriginId%': userData[0].EAID
                });
                $scope.achievementSets = achievementData.achievementSets; // set achievementSets for public profile
                $scope.score = {
                    grantedAchievements : achievementData.grantedAchievements(),
                    totalAchievements : achievementData.totalAchievements(),
                    grantedXp : achievementData.grantedXp(),
                    totalXp : achievementData.totalXp()
                };
            }
            $scope.isLoading = false;
            $scope.$digest();
        }

        /**
         * Reload achievements route
         * @method reloadCurrentRoute
         */
        $scope.reloadCurrentRoute = function() {
            AppCommFactory.events.fire('origin-route:reloadcurrentRoute');
        };

        /**
         * Show the retry error message
         * @method displayRetryMessage
         */
        function displayRetryMessage(error) {
            $scope.isLoading = false;
            $scope.retry = true;
            ComponentsLogFactory.error('[Origin-Achievements-Otheruser-Directive displayRetryMessage Method] error', error);
        }

        /**
         * Get User Portfolio from achievement factory and atom service
         * @method getUserPortfolio
         */
        function getUserPortfolio() {
            Promise.all([
                    AchievementFactory.getAchievementPortfolio($scope.personaId),
                    Origin.atom.atomUserInfoByUserIds([$scope.nucleusId])
                ])
                .then(_.spread(setUserPortfolio))
                .catch(displayRetryMessage);
        }

        /**
         * Set Persona ID
         * @method setPersonaId
         */
        function setPersonaId(personaId) {
            $scope.personaId = personaId;
        }

        /**
         * Get Persona Id from userdata factory
         * @method getPersonaId
         */
        function getPersonaId() {
            UserDataFactory.getPersonaId($scope.nucleusId)
                .then(setPersonaId)
                .then(getUserPortfolio)
                .catch(function(error) {
                    ComponentsLogFactory.error('[Origin-Achievements-Otheruser-Directive getPersonaId] error', error);
                });
        }

        /**
         * Reset values on authChange
         * @method onAuthChange
         */
        function onAuthChange() {
            $scope.achievementSets = {};
            $scope.score = null;
            $scope.personaId = '';
        }

        /**
         * Unhook from auth factory events when directive is destroyed.
         * @method onDestroy
         */
        function onDestroy() {
            AuthFactory.events.off('myauth:change', onAuthChange);
            AuthFactory.events.off('myauth:ready', getPersonaId);
        }

        /* Bind to auth events */
        AuthFactory.events.on('myauth:change', onAuthChange);
        AuthFactory.events.on('myauth:ready', getPersonaId);

        $scope.$on('$destroy', onDestroy);

        $scope.loading = true;
        AuthFactory.waitForAuthReady()
            .then(getPersonaId)
            .catch(function(error) {
                ComponentsLogFactory.error('[Origin-Achievements-Otheruser-Directive AuthReady] error', error);
            });
    }

    /**
     * Achievements Other User Directive
     */
    function OriginAchievementsOtheruser(ComponentsConfigFactory) {

        function OriginAchievementsOtheruserLink() {
            angular.element('html, body').animate({ scrollTop: 0 }, 'fast');
        }

        return {
            restrict: 'E',
            controller: 'OriginAchievementsOtheruserCtrl',
            scope: {
                overallProgressStr: '@overallprogress',
                achievementsCompletedStr: '@achievementscompleted',
                pointsCompletedStr: '@pointscompleted',
                noPeekingStr: '@nopeeking',
                achievementsPrivateStr: '@achievementsprivate',
                noGamesOtherTitleStr: '@nogamesothertitle',
                noGamesOtherTextStr: '@nogamesothertext',
                nucleusId: '@nucleusid',
                retryErrorTitleStr: '@retryerrortitle',
                retryErrorDescStr: '@retryerrordesc',
                retryErrorCTALoc: '@retryerrorcta'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('achievements/views/achievements-otheruser.html'),
            link: OriginAchievementsOtheruserLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originAchievementsOtheruser
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} overallprogress "Overall Achievement Progress"
     * @param {LocalizedString} achievementscompleted "Achievements Completed"
     * @param {LocalizedString} pointscompleted "Origin points earned"
     * @param {LocalizedString} nopeeking "No peeking."
     * @param {LocalizedString} achievementsprivate "\"%OriginId%\" set their achievements to private."
     * @param {LocalizedString} nogamesothertitle "Nothing to see here."
     * @param {LocalizedString} nogamesothertext "\"%OriginId%\" doesn't currently have any games that offer achievements."
     * @param {LocalizedString} retryerrortitle "Well, that didn't go as planned."
     * @param {LocalizedString} retryerrordesc "Origin encountered an issue loading this page. Please try again later"
     * @param {LocalizedString} retryerrorcta "Try Again"
     * @param {string} nucleusid - Nucleus Id of the selected friend/stranger
     * @description
     *
     * Overall Achievement Progress of another user
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-achievements-otheruser nucleusid="123456"></origin-achievements-otheruser>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginAchievementsOtheruserCtrl', OriginAchievementsOtheruserCtrl)
        .directive('originAchievementsOtheruser', OriginAchievementsOtheruser);
}());