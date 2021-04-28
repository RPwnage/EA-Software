(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-achievement-game-details';

    /**
    * Achievement Details Ctrl
    */
    function OriginAchievementGameDetailsCtrl($scope, UtilFactory, AchievementFactory, ObjectHelperFactory) {
        // set the initial limit for achievements list
        $scope.limit = 12;

        /* Get Translated Strings */
        $scope.achievementLoc = UtilFactory.getLocalizedStr($scope.achievementStr, CONTEXT_NAMESPACE, 'achievement');
        $scope.pointsLoc = UtilFactory.getLocalizedStr($scope.pointsStr, CONTEXT_NAMESPACE, 'points');
        $scope.viewAllLoc = UtilFactory.getLocalizedStr($scope.viewAllStr, CONTEXT_NAMESPACE, 'viewall');

        $scope.showAllTiles = function() {
            $scope.limit = $scope.gameAchievements.length;
        };

        /**
         * Sets the achievement data for the directive
         * @param achievementSet the data to set
         */
        function setAchievementData(achievementSet) {
            $scope.gameAchievements = ObjectHelperFactory.toArray(achievementSet.achievements);
        }

        /**
         * Callback for a dirtybits achievement event. This should only be hooked up if the achievements page is for the current user
         */
        function onAchievementSetUpdated(updateInfo) {
            //$scope.achievementSet() represents the initial achievement set used to initialize the directive
            //$scope.gameAchievements represents the current data of the directive (just the achievement list, slightly reformatted)
            //Since the achievementset id doesn't change, we can always use $scope.achievementSet().id to id the new data
            //Result is saved into $scope.gameAchievements while $scope.achievementSet() remains intact
            if (updateInfo.achievementSet.baseGame.id === $scope.achievementSet().id) {
                setAchievementData(updateInfo.achievementSet.baseGame);
                $scope.$digest();
            }
        }

        /**
         * Unhook from achievement factory events when directive is destroyed.
         * @method onDestroy
         */
        function onDestroy() {
            AchievementFactory.events.off('achievements:achievementGranted', onAchievementSetUpdated);
        }

        /**
         * This sets up the initial data for the directive
         */
        if($scope.achievementSet().achievements) {
            setAchievementData($scope.achievementSet());
        }

        //Only listen for dirtybits notifications if this is our user
        if ($scope.personaid === Origin.user.personaId().toString())
        {
            //Update page from dirtybits response
            AchievementFactory.events.on('achievements:achievementGranted', onAchievementSetUpdated);

            $scope.$on('$destroy', onDestroy);
        }

    }

    /**
     * Achievement Details Directive
     */
    function originAchievementGameDetails(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            controller: 'OriginAchievementGameDetailsCtrl',
            scope: {
                achievementSet: '&achievementset',
                isOgd: '&isogd',
                personaid: '@personaid',
                achievementStr: '@achievement',
                pointsStr: '@points',
                viewAllStr: '@viewall'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('achievements/views/achievement-game-details.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originAchievementGameDetails
     * @restrict E
     * @scope
     * @param {object} achievementset - achievementSet object
     * @param {LocalizedString} achievement "Achievements"
     * @param {LocalizedString} points "Points"
     * @param {LocalizedString} viewall "View All"
     * @description
     *
     * Achievement Game details page - Build out the UI for each game object passed.
     * Contains Name, Score and achievments list 
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-achievement-game-details achievementset="achievementSetObject" isogd="true/false"></origin-achievement-details>
     *     </file>
     * </example>
     *
     */
    // declaration
    angular.module('origin-components')
        .controller('OriginAchievementGameDetailsCtrl', OriginAchievementGameDetailsCtrl)
        .directive('originAchievementGameDetails', originAchievementGameDetails);
}());