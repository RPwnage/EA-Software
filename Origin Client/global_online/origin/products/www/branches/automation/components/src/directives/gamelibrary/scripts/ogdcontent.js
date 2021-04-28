/**
 * file gamelibrary/ogdcontent.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-gamelibrary-ogd-content',
        LOAD_TABS_DELAY_MS = 400;

    function OriginGamelibraryOgdContentCtrl($scope, $compile, $timeout, AppCommFactory, GamesDataFactory, ComponentsLogFactory, UtilFactory, EntitlementStateRefiner, AchievementFactory) {

        var localize = UtilFactory.getLocalizedStrGenerator(CONTEXT_NAMESPACE),
            personaId = Origin.user.personaId(),
            stateChangeStartEventHandle = null,
            tabDelayTimeoutHandle,
            achievementSetId;

        $scope.strings = {
            friendsWhoPlay: localize($scope.friendsWhoPlayStr, 'friends-who-play-str'),
            expansions: localize($scope.expansionsStr, 'expansions-str'),
            achievements: localize($scope.achievementsStr, 'achievements-str')
        };

        /**
         * WIP: Mockup of menu content in OCD
         */
        $scope.nav = [
            {
                'origin-gamelibrary-ogd-content-panel': {
                    anchorName: $scope.strings.friendsWhoPlay,
                    title: $scope.strings.friendsWhoPlay,
                    friendlyname: 'friends-who-play',
                    hideForUnderAge: true,
                    items: [
                        {
                            'origin-gamelibrary-ogd-friendswhoplay': {
                                'offerid': $scope.offerId
                            }
                        }
                    ]
                }
            }, {
                'origin-gamelibrary-ogd-content-panel': {
                    anchorName: $scope.strings.expansions,
                    title: $scope.strings.expansions,
                    friendlyname: 'expansions',
                    items: [
                        {
                            'origin-gamelibrary-ogd-expansions': {
                                'offerid': $scope.offerId
                            }
                        }
                    ]
                }
            }
        ];

        function transformToNavItems(arr) {
            return _.map(arr, function(item) {
                return item['origin-gamelibrary-ogd-content-panel'];
            });
        }

        /**
         * Update the topic from a rebroadcasted uirouter event
         * @param  {Object} event    event
         * @param  {Object} toState  $state
         * @param  {Object} toParams $stateParams
         */
        function updateTopic(event, toState, toParams) {
            if (toParams.topic) {
                $scope.topic = toParams.topic;
                //$scope.$digest();
            }
        }

        /**
         * Mapping function for extracting topics from a list of panels.
         * @param  {Object} panel an instance of a panel
         * @return {string} the friendlyname
         */
        function getTopicList(panel) {
            return panel['origin-gamelibrary-ogd-content-panel'].friendlyname;
        }

        /**
         * Filter for under age
         * @param {Object[]} data the OCD data feed
         * @return {Object[]} filtered list
         */
        function filterUnderAge(data) {
            if (Origin.user.underAge()) {
                return data.filter( function(item) {
                    var isUnderAge = item['origin-gamelibrary-ogd-content-panel'].hideForUnderAge === true;
                    return !isUnderAge;
                });
            } else {
                return data;
            }
        }

        /**
         * Set the achievement tab and update dom 
         */
        function setAchievementTab(achievementData) {
            if (achievementData && achievementData.achievements) {
                var achievementTab = {
                    'origin-gamelibrary-ogd-content-panel': {
                        anchorName: $scope.strings.achievements,
                        title: $scope.strings.achievements,
                        friendlyname: 'achievements',
                        hideForUnderAge: true,
                        items: [
                            {
                                'origin-achievement-details': {
                                    'achievementsetid': achievementSetId,
                                    'nucleusid': Origin.user.userPid(),
                                    'isogd': true
                                }
                            }
                        ]
                    }
                };

                // Add achievement tab info to nav
                $scope.nav.splice(1, 0, achievementTab);
            }

            $scope.nav = filterUnderAge($scope.nav);
            $scope.validateRoute($scope.topic, $scope.offerId);
            $scope.$emit('ogd:tabsready', transformToNavItems($scope.nav));
            tabDelayTimeoutHandle = $timeout($scope.updateDom, LOAD_TABS_DELAY_MS, false);

            //need a digest here as this is triggered by a promise
            $scope.$digest();
        }

        /**
         * Handle error from catalog request or entitlement request.
         * @param  {Object} error error message
         */
        function onError(error) {
            ComponentsLogFactory.error('[Origin-Gamelibrary-Ogd-Content-Directive Ctrl] error', error);
        }

        /**
         * Get achievement set ID, isPreorder state and get achievements for the game.
         * @param  {Object} catalogData catalog info for this offer ID
         * @param  {Object} entitlementData entitlement info for this offer ID
         */
        function getAchievementSet(catalogData, entitlementData) {
            achievementSetId = _.head(AchievementFactory.determineOwnedAchievementSets(catalogData));

            if (achievementSetId && !EntitlementStateRefiner.isPreorder(entitlementData)) {
                return AchievementFactory.getAchievementSet(personaId, achievementSetId);
            }
        }

        /**
         * Validate the tab route and redirect to the first tab in the panel set
         * if it's not a valid deeplink
         */
        $scope.validateRoute = function(topic, offerId) {
            var topics = $scope.nav.map(getTopicList);

            if(topics.length > 0 && topics.indexOf(topic) === -1) {
                AppCommFactory.events.fire('uiRouter:go', 'app.game_gamelibrary.ogd.topic', {
                    offerid: offerId,
                    topic: topics[0]
                });
            }
        };

        stateChangeStartEventHandle = AppCommFactory.events.on('uiRouter:stateChangeStart', updateTopic);

        $scope.$on('$destroy', function(){
            if(stateChangeStartEventHandle) {
                stateChangeStartEventHandle.detach();
            }

            $timeout.cancel(tabDelayTimeoutHandle);
        });

        // Get catalog and entilement info to render achievement tab
        Promise.all([
            GamesDataFactory.getCatalogInfo([$scope.offerId]),
            GamesDataFactory.getEntitlement($scope.offerId),
        ])
        .then(_.spread(getAchievementSet))
        .then(setAchievementTab)
        .catch(onError);

    }

    function originGamelibraryOgdContent(ComponentsConfigFactory, $compile, OcdHelperFactory) {
        function originGamelibraryOgdContentLink(scope, element) {
            /**
             * Generate markup for the content panels
             *
             * @param {Object[]} data the OCD data feed
             * @return {string} HTML or content panels
             */
            function generateContentPanels(data) {
                var target = element.find('.origin-ogd-expansions .content');

                if(data && data.length > 0) {
                    for(var i = 0; i < data.length; i++) {
                        var node = data[i]['origin-gamelibrary-ogd-content-panel'],
                            slingData = data[i];

                        node['ng-if'] = ['topic === \'', node.friendlyname, '\''].join('');

                        // ORCORE-3546 Remove the "title" attribute from the DOM so we don't show unwanted tooltips
                        var domElement = OcdHelperFactory.slingDataToDom(slingData);
                        domElement.removeAttribute('title');

                        target.append($compile(domElement)(scope));
                    }
                }

                target = null;
            }

            /**
             * Update the dom regions from OCD data
             */
            scope.updateDom = function() {
                generateContentPanels(scope.nav);
                scope.$digest();
            };
        }

        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid',
                topic: '@topic',
                tabhorizontalrulecolor: '@tabhorizontalrulecolor',
                tabactiveselectioncolor: '@tabactiveselectioncolor',
                tabactivetextcolor: '@tabactivetextcolor',
                tabtextcolor: '@tabtextcolor',
                friendsWhoPlayStr: '@',
                achievementsStr: '@',
                expansionsStr: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdcontent.html'),
            link: originGamelibraryOgdContentLink,
            controller: OriginGamelibraryOgdContentCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdContent
     * @restrict E
     * @element ANY
     * @scope
     * @param {OfferId} offerid the offer id
     * @param {string} topic the topic name to use as a default
     * @param {string|OCD} tabhorizontalrulecolor color of the tab bar horizontal rule theme
     * @param {string|OCD} tabactiveselectioncolor the tab bar active selection underline theme
     * @param {string|OCD} tabactivetextcolor The color of the hover and active text
     * @param {string|OCD} tabtextcolor the color of inactive text in the tab bar
     * @param {LocalizedString} friends-who-play-str Friends Who Play
     * @param {LocalizedString} achievements-str Achievements
     * @param {LocalizedString} expansions-str Expansions
     *
     * @description
     *
     * Origin Game Library OGD content directive
     */
    angular.module('origin-components')
        .controller('OriginGamelibraryOgdContentCtrl', OriginGamelibraryOgdContentCtrl)
        .directive('originGamelibraryOgdContent', originGamelibraryOgdContent);
})();
