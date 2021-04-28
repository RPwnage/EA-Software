(function () {
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-achievement-details';

    /**
    * Achievement Details Ctrl
    */
    function OriginAchievementDetailsCtrl($scope, AuthFactory, UserDataFactory, AchievementFactory, EventsHelperFactory,
        ComponentsLogFactory, AppCommFactory, UtilFactory, NavigationFactory) {

        var localize = UtilFactory.getLocalizedStrGenerator(CONTEXT_NAMESPACE);

        $scope.isLoading = true;

        /* Get Translated Strings */
        $scope.achievementsCompletedLoc = localize($scope.achievementsCompletedStr, 'achievementscompleted');
        $scope.pointsCompletedLoc = localize($scope.pointsCompletedStr, 'pointscompleted');

        /**
         * Set AchievementSet data
         * @method setAchievementData
         */
        function setAchievementData(data) {
            $scope.baseGameData = data.baseGame;
            $scope.score = {
                grantedAchievements: data.grantedAchievements(),
                totalAchievements: data.totalAchievements(),
                grantedXp: data.grantedXp(),
                totalXp: data.totalXp()
            };

            if (Object.keys(data.expansions).length) { // cover the use case for SDK expansions
                $scope.expansionsData = data.expansions;
            }
            $scope.isLoading = false;
            $scope.$digest();
        }

        /**
         * Get Achievement Set data
         * @method getAchievementSet
         */
        function getAchievementSet() {
            AchievementFactory.getAchievementSet($scope.personaId, $scope.achievementSetId)
                .then(setAchievementData)
                .catch(function(error) {
                    ComponentsLogFactory.error('[Origin-Achievement-Details-Directive getAchievementset Method] error', error);
                });
        }

        /**
         * Verify that this achievement set is part of the user's portfolio and that the user has not opted out of
         * sharing achievements in their privacy settings.
         * @method checkOwnershipAndPrivacy
         */
        function checkOwnershipAndPrivacy(achievementData) {
            if(!achievementData.hidden) {
                var achSets = achievementData.achievementSets;
                for (var achSet in achSets) {
                    if (achSets.hasOwnProperty(achSet)) {
                        if (_.includes(achSets[achSet].compatibleIds, $scope.achievementSetId)) {
                            // Achievements are not private, and the achievement set is in the user's portfolio, so get the set.
                            getAchievementSet();
                            return;
                        }
                    }
                }
            }

            // Either the achievements are private or the user does not own a game in the achievement set.  In either case,
            // we redirect to the achievements summary page.  If it is a privacy issue, the summary page will display the 
            // privacy message.
            var isSelf = Number($scope.personaId) === Number(Origin.user.personaId());

            if (isSelf) {
                NavigationFactory.goToState('app.profile.topic', {
                    topic: 'achievements'
                });
            } else {
                NavigationFactory.goUserProfile($scope.nucleusId,'achievements');
            }
        }

        /**
         * Get Achievement Portfolio from factory
         * @method getAchievementPortfolio
         */
        function getAchievementPortfolio() {
            AchievementFactory.getAchievementPortfolio($scope.personaId)
                .then(checkOwnershipAndPrivacy)
                .catch(function(error) {
                    ComponentsLogFactory.error('[Origin-Achievements-Details-Directive getAchievementPortfolio] error', error);
                });
        }

        /**
         * Set the persona id for the selected user
         * @method setPersonaId
         */
        function setPersonaId(personaId) {
            $scope.personaId = personaId;
        }

        /**
         * Get the persona id for selected user
         * @method getPersonaId
         */
        function getPersonaId(){
            if (Number($scope.nucleusId) !== Number(Origin.user.userPid())) { // friend/stranger profile
                UserDataFactory.getPersonaId($scope.nucleusId)
                    .then(setPersonaId)
                    .then(getAchievementPortfolio)
                    .catch(function(error) {
                        ComponentsLogFactory.error('[Origin-Achievements-Details-Directive getPersonaId] error', error);
                    });
            } else { // own profile
                $scope.personaId = Origin.user.personaId();
                getAchievementPortfolio();
            }
        }

        AuthFactory.waitForAuthReady()
            .then(getPersonaId);

        /**
         * Unhook from achievement factory events when directive is destroyed.
         * @method onDestroy
         */
        function onDestroy() {
            AchievementFactory.events.off('achievements:achievementGranted', getAchievementPortfolio);
        }

        //Update page from dirtybits response
        AchievementFactory.events.on('achievements:achievementGranted', getAchievementPortfolio);

        $scope.$on('$destroy', onDestroy);
    }

    /**
     * Achievement Details Directive
     */
    function originAchievementDetails(ComponentsConfigFactory) {


        return {
            restrict: 'E',
            controller: 'OriginAchievementDetailsCtrl',
            scope: {
                achievementSetId: '@achievementsetid',
                nucleusId: '@nucleusid',
                isOgd: '=isogd',
                achievementsCompletedStr: '@achievementscompleted',
                pointsCompletedStr: '@pointscompleted'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('achievements/views/achievement-details.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originAchievementDetails
     * @restrict E
     * @scope
     * @param {string} achievementsetid - achievement set id for the achievements displayed on this page
     * @param {string} nucleusid - nucleus id of the selected user
     * @param {LocalizedString} achievementscompleted "Achievements Completed"
     * @param {LocalizedString} pointscompleted "Origin points earned"
     * @param {BooleanEnumeration} isogd - set true if its OGD achievements tab or false if its profile achievements tab
     * @description
     *
     * Achievement details page - list of achievements for one achievement set
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-achievement-details achievementsetid="123456" nucleusid="123456" isogd="true"></origin-achievement-details>
     *     </file>
     * </example>
     *
     */
    // declaration
    angular.module('origin-components')
        .controller('OriginAchievementDetailsCtrl', OriginAchievementDetailsCtrl)
        .directive('originAchievementDetails', originAchievementDetails);
}());