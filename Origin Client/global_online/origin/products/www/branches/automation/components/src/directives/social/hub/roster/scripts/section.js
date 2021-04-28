/**
 * Created by tirhodes on 2015-02-20.
 * @file section.js
 */
(function () {

    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    //https://github.com/darkskyapp/binary-search/blob/master/index.js
    /*jshint bitwise: false */
    function binarySearch(haystack, needle, comparator, low, high) {
        var mid, cmp;

        if (typeof low === 'undefined') {
            low = 0;
        }

        if (typeof high === 'undefined') {
            high = haystack.length - 1;
        }

        while(low <= high) {
            // Note that "(low + high) >>> 1" may overflow, and results in a typecast
            // to double (which gives the wrong results)
            mid = low + (high - low >> 1);
            cmp = +comparator(haystack[mid], needle);

            if(cmp < 0.0) { // Too low
                low  = mid + 1;
            } else if(cmp > 0.0) { // Too high
                high = mid - 1;
            } else { // Key found
                return { found: true, index : mid };
            }
        }

        // Key not found
        return { found: false, index: low };
    }


    /**
    * The controller
    */
    function OriginSocialHubRosterSectionCtrl($scope, $filter, $timeout, RosterDataFactory, UtilFactory, SocialHubFactory, LocalStorageFactory) {
        $scope.roster = {};
        $scope.rosterArray = [];
        $scope.filteredRosterArray = [];
        $scope.rosterLength = 0;

        $scope.sectionID = -1;

        // default start value
        $scope.start = 0;
        $scope.overflowStart = 0;

        $scope.filterText = '';
        $scope.limit = RosterDataFactory.getSectionViewportFriendCount() * 3; // three-times oversample

        $scope.bottomPadding = 0;
        $scope.topPadding = 0;

        $scope.onlineCount = 0;
        $scope.dataIsFiltered = false;

        var self = this;

        function setHeaderHeight() {
            if ((typeof $scope.headerHeight==='undefined' || $scope.headerHeight === 0) && $scope.$headerElement.height()){
                $scope.headerHeight = $scope.$headerElement.outerHeight();
            }
        }

        function isOnline(friend) {
            return (!!friend.presence && friend.presence.presenceType!=='UNAVAILABLE' && friend.presence.show==='ONLINE');
        }

        function isAway(friend) {
            return (!!friend.presence && friend.presence.presenceType!=='UNAVAILABLE' && friend.presence.show==='AWAY');
        }

        function isInGame(friend) {
            return (!!friend.presence && friend.presence.show==='ONLINE' && !!friend.presence.gameActivity && typeof friend.presence.gameActivity.title !== 'undefined' && friend.presence.gameActivity.title.length);
        }

        function isJoinable(friend) {
            return (isInGame(friend) && !!friend.presence && !!friend.presence.gameActivity && !!friend.presence.gameActivity.joinable);
        }

        function isBroadcasting(friend) {
            return (isInGame(friend) && !!friend.presence && !!friend.presence.gameActivity && !!friend.presence.gameActivity.twitchPresence && friend.presence.gameActivity.twitchPresence.length);
        }

        function originSort(a, b) {
            var result = UtilFactory.compareOriginId(a, b);

            // Broadcasting ?
            if (isBroadcasting(a) && isBroadcasting(b)) {

                if (isJoinable(a) && isJoinable(b)) {
                    // use origin id
                } else if (isJoinable(a)) {
                    result = -1;
                } else if (isJoinable(b)) {
                    result = 1;
                }

            } else if (isBroadcasting(a)) {
                result = -1;
            } else if (isBroadcasting(b)) {
                result = 1;
            } else {

                // Not broadcasting. In-game perhaps?
                if (isInGame(a) && isInGame(b)) {

                    if (isJoinable(a) && isJoinable(b)) {
                        // use origin id
                    } else if (isJoinable(a)) {
                        result = -1;
                    } else if (isJoinable(b)) {
                        result = 1;
                    }

                } else if (isInGame(a)) {
                    result = -1;
                } else if (isInGame(b)) {
                    result = 1;
                } else {

                    // Online at least?
                    if (isOnline(a) && isOnline(b)) {
                        // use origin id
                    } else if (isOnline(a)) {
                        result = -1;
                    } else if (isOnline(b)) {
                        result = 1;
                    } else {
                        // Perhaps these friends are Away?
                        if (isAway(a) && isAway(b)) {
                            // use origin id
                        } else if (isAway(a)) {
                            result = -1;
                        } else if (isAway(b)) {
                            result = 1;
                        } else {
                            // that's it! either these friends are offline, or invisible,
                            // either way use origin id
                        }
                    }
                }
            }

            return result;
        }


        function handleSocialAreaFilterCount() {
            if($scope.sectionID === -1) {
                $scope.sectionID = $scope.$parent.getNewSectionID($scope.rosterLength);
            } else {
                $scope.$parent.calculateTotalFilteredFriendCount($scope.sectionID, $scope.rosterLength);
            }
        }

        function filterRosterArray() {
            if ($scope.filterText.length) {
                $scope.filteredRosterArray = $filter('filter')($scope.rosterArray, {'originId': $scope.filterText});
            } else {
                $scope.filteredRosterArray = $scope.rosterArray;
            }
            $scope.rosterLength = $scope.filteredRosterArray.length;
        }

        function handleUpdateFilter(filter) {
            $scope.filterText = filter;
            filterRosterArray();

            if ($scope.filterText.length) {
                $scope.dataIsFiltered = true;

                // Open section if data is filtered
                self.openSection();
            } else {
                $scope.dataIsFiltered = false;
                if (LocalStorageFactory.get('origin-social-hub-section-FRIENDSLIST-'+$scope.filter+'-closed-'+Origin.user.originId(), null, true)) {
                    self.closeSection();
                }
            }

            RosterDataFactory.updateRosterViewport();

            handleSocialAreaFilterCount();
        }

        RosterDataFactory.events.on('updateFriendsListFilter', handleUpdateFilter);


         var callThrottledScopeDigest = UtilFactory.throttle(function() {
             $timeout(function() { $scope.$digest(); }, 0, false);
         }, 1000);

        function calculateViewportPadding() {

            // If viewport is hidden, set padding to 0
            var $viewport = $scope.$sectionElement.find('.origin-social-hub-section-viewport');
            if (($viewport.length > 0) && ($viewport.is(':hidden'))) {
                $scope.topPadding = 0;
                $scope.bottomPadding = 0;
                return;
            }

            $scope.limit = RosterDataFactory.getSectionViewportFriendCount() * 3; // three-times oversample

            var panelHeight = RosterDataFactory.getFriendPanelHeight();
            $scope.topPadding = 0;
            if ($scope.rosterLength > 0) {

                $scope.topPadding = ($scope.overflowStart) * panelHeight;

                // add section header height if header is invisible
                $scope.topPadding += ($scope.overflowStart ? $scope.headerHeight : 0);

            }
//            $scope.topPadding = ($scope.rosterLength > 0) ? ($scope.start * panelHeight) + ($scope.start ? $scope.headerHeight : 0) : 0;

            $scope.bottomPadding = Math.max($scope.rosterLength - $scope.limit - $scope.overflowStart, 0) * panelHeight;

            // console.log('TR: '+ $scope.filter +' $scope.start : ' + $scope.start );
            // console.log('TR: '+ $scope.filter +' $scope.limit : ' + $scope.limit );
            // console.log('TR: '+ $scope.filter +' $scope.rosterLength : ' + $scope.rosterLength );
            // console.log('TR: '+ $scope.filter +' panelHeight : ' + panelHeight );
            // console.log('TR: '+ $scope.filter +' $scope.topPadding: ' + $scope.topPadding);
            // console.log('TR: '+ $scope.filter +' $scope.bottomPadding: ' + $scope.bottomPadding);

        }

        function handleUpdateRoster(friend) {
            // Don't add friends if they have no originId. They will be re-added when the originId is determined
            if (typeof friend.originId !== 'undefined' && friend.originId.length > 0) {

                // Add to roster
                var result = binarySearch($scope.rosterArray, friend, originSort);
                var replace = ( result.found ? 1 : 0 );
                $scope.rosterArray.splice(result.index, replace, friend);
                $scope.roster[friend.nucleusId] = friend;

                //console.log('TR handleUpdateRoster: result:' + JSON.stringify(result) );

                filterRosterArray();

                if (result.index <= $scope.limit) {
                    callThrottledScopeDigest();
                }

                setHeaderHeight();
                calculateViewportPadding();

                handleSocialAreaFilterCount();

                //console.log('TR handleUpdateRoster end: ' + $scope.filter + ' : ' + $scope.rosterArray.length);
            }
        }

        // this signal handler is deferred when accepting a friend request. This allows for the animation to be displayed in pendingitem
        function handleDeleteRoster(nucleusId, deferSignalHandler) {
            if (!deferSignalHandler) {
                $.each($scope.rosterArray, function(i, friend) {
                    if (friend.nucleusId === nucleusId) {
                        $scope.rosterArray.splice(i, 1);
                        return false;
                    }
                });

                filterRosterArray();

                delete $scope.roster[nucleusId];

                setHeaderHeight();
                calculateViewportPadding();

                callThrottledScopeDigest();
            }
        }

        this.deleteFromRoster = function(nucleusId) {
            handleDeleteRoster(nucleusId, false);
        };

        function handleClearRoster() {

            $scope.rosterArray = [];
            $scope.filteredRosterArray = [];
            $scope.rosterLength = 0;

            $scope.roster = {};

            setHeaderHeight();
            calculateViewportPadding();
            callThrottledScopeDigest();
        }

        RosterDataFactory.events.on('socialRosterUpdate' + $scope.filter, handleUpdateRoster);
        RosterDataFactory.events.on('socialRosterDelete' + $scope.filter, handleDeleteRoster);
        RosterDataFactory.events.on('socialRosterClear' + $scope.filter, handleClearRoster);

        this.friendYPosition = function (index, height) {

            var friendYPos = 0;
            var sectionDomElement = $scope.$sectionElement.get(0);
            var offsetParent = (!!sectionDomElement ? sectionDomElement.offsetParent : null);
            if (!!offsetParent) {

                //            var pos = $scope.$sectionElement.position();
                //            var top = pos.top; // - $scope.topPadding;

                // TR ORCORE-2452 The above doesn't work in the dev.x environment,
                // replacing with the below unless and until we figure out why the above fails
                var clientRect = sectionDomElement.getBoundingClientRect();
                var parentClientRect = offsetParent.getBoundingClientRect();

                var top = clientRect.top - parentClientRect.top;

                friendYPos = top + (index * height);
            }

            //console.log('TR vis: top:' + top);
            //console.log('TR vis: index:' + index);
            //console.log('TR vis: height:' + height);
            //console.log('TR vis: friendPos:' + friendPos);
            //console.log('TR vis: viewportHeight:' + viewportHeight);
            //console.log('TR vis: visible:' + visible);
            return friendYPos;
        };

        this.isFriendVisible = function(index, height) {

            var viewportHeight = RosterDataFactory.getRosterViewportHeight();
            var friendPos = this.friendYPosition(index, height);
            var visible = (friendPos > (0 - height)) && (friendPos <= viewportHeight);

            //console.log('TR vis: visible:' + visible);
            return visible;
        };

        function handleUpdateViewport() {
            // reset to default start values
            $scope.start = 0;
            $scope.overflowStart = 0;
            var sectionDomElement = $scope.$sectionElement.get(0);
            var offsetParent = (!!sectionDomElement ? sectionDomElement.offsetParent : null);
            if (!!offsetParent) {
                var viewportFriendCount = RosterDataFactory.getSectionViewportFriendCount();
                $scope.limit = viewportFriendCount * 3; // three-times oversample

                //var pos = $scope.$sectionElement.position();
                //var top = pos.top;

                // TR ORCORE-2452 The above doesn't work in the dev.x environment,
                // replacing with the below unless and until we figure out why the above fails
                var clientRect = sectionDomElement.getBoundingClientRect();
                var parentClientRect = offsetParent.getBoundingClientRect();

                var top = clientRect.top - parentClientRect.top;

                var panelHeight = RosterDataFactory.getFriendPanelHeight();

                setHeaderHeight();

                //console.log('TR: section: ' + $scope.sectionName + ' top: ' + top  + ' bottom: ' + bottom + ' rosterLength: ' + $scope.rosterLength + ' header: ' + $scope.headerHeight);
                if ($scope.rosterLength > 0) {

                    if ((top+$scope.headerHeight) < 0) {
                        var sectionYOffset = Math.abs(top + $scope.headerHeight);

                        //console.log('TR: section: ' + $scope.filter + ' y offset: ' + sectionYOffset + ' panel height: ' +  panelHeight);
                        if (sectionYOffset > panelHeight) {
                            $scope.start = Math.min(Math.floor( sectionYOffset / panelHeight ), $scope.rosterLength);

                            // Adjust the start so that it is above the visible viewport
                            if ($scope.start > viewportFriendCount) {
                                $scope.overflowStart = Math.max( $scope.start - viewportFriendCount, 0 );
                            } else {
                                $scope.overflowStart = 0;
                            }

                        }

                    }

                }

                var index = 0;
                while (index < $scope.limit) {
                    if ($scope.filteredRosterArray.length > ($scope.start + index)) {
                        RosterDataFactory.updateFriendDirective($scope.filteredRosterArray[$scope.start + index].nucleusId);
                    } else {
                        break;
                    }

                    index++;
                }

                calculateViewportPadding();

                //console.log('TR: section ' + $scope.filter + ' $scope.start: ' +  $scope.start);
                //console.log('TR: section ' + $scope.filter + ' $scope.overflowStart: ' +  $scope.overflowStart);
                //console.log('TR: section ' + $scope.filter + ' panelHeight: ' + panelHeight);
                //console.log('TR: section: ' + $scope.filter + ' top:' + $scope.topPadding);
                //console.log('TR: section: ' + $scope.filter + ' bottom:' + $scope.bottomPadding);


            }
            $timeout(function() { $scope.$digest(); }, 0, false);
        }

        var droppedHandleUpdateViewport = UtilFactory.dropThrottle(handleUpdateViewport, 100);

        RosterDataFactory.events.on('updateRosterViewport', droppedHandleUpdateViewport);

        function handleRosterCleared() {
            $scope.roster = {};
            $scope.rosterArray = [];
            $scope.filteredRosterArray = [];
            $scope.rosterLength = $scope.rosterArray.length;

            $scope.topPadding = 0;
            $scope.bottomPadding = 0;

            callThrottledScopeDigest();
        }

        RosterDataFactory.events.on('rosterCleared', handleRosterCleared);

        this.openSection = function() {
            $scope.$sectionElement.find('.origin-social-hub-section-downarrow').removeClass('origin-social-hub-section-downarrow-rotate');
            $scope.$headerElement.addClass('header-visible');

            $scope.$viewportElement.show();
            calculateViewportPadding();
        };

        this.closeSection = function() {
            $scope.$sectionElement.find('.origin-social-hub-section-downarrow').addClass('origin-social-hub-section-downarrow-rotate');
            $scope.$headerElement.removeClass('header-visible');

            $scope.$viewportElement.hide();
            calculateViewportPadding();
        };

        this.toggleSectionVisible = function() {
            if ($scope.$viewportElement.is(':hidden')) {
                self.openSection();
                LocalStorageFactory.delete('origin-social-hub-section-FRIENDSLIST-'+$scope.filter+'-closed-'+Origin.user.originId(), true);
            } else {
                self.closeSection();
                LocalStorageFactory.set('origin-social-hub-section-FRIENDSLIST-'+$scope.filter+'-closed-'+Origin.user.originId(), 'true', true);
            }
            $scope.$digest();
        };

        this.getRoster = function(filter) {
            // Load the roster for this section
            RosterDataFactory.getRoster(filter).then(function(roster) {

                for(var id in roster) {
                    handleUpdateRoster(roster[id]);
                }
            });
        };

        this.updateSectionToggleState = function () {
            if (LocalStorageFactory.get('origin-social-hub-section-FRIENDSLIST-'+$scope.filter+'-closed-'+Origin.user.originId(), null, true)) {
                self.closeSection();
            } else {
                self.openSection();
            }
        };

        SocialHubFactory.events.on('unpopOutHub', self.updateSectionToggleState);

        function onDestroy() {
            RosterDataFactory.events.off('updateFriendsListFilter', handleUpdateFilter);
            RosterDataFactory.events.off('socialRosterUpdate' + $scope.filter, handleUpdateRoster);
            RosterDataFactory.events.off('socialRosterDelete' + $scope.filter, handleDeleteRoster);
            RosterDataFactory.events.off('updateRosterViewport', droppedHandleUpdateViewport);
            RosterDataFactory.events.off('rosterCleared', handleRosterCleared);
            SocialHubFactory.events.off('unpopOutHub', self.updateSectionToggleState);
        }

        $scope.$on('$destroy', onDestroy);
    }

    // Post repeat directive for logging the rendering time
    angular.module('origin-components').directive('rosterLoadPerformanceDirective',
      function($timeout, RosterDataFactory) {

            return function(scope) {
                if (scope.$last) {
                    $timeout(function(){
                        if (RosterDataFactory.isRosterLoaded()) {
                            var rosterLoadStartTime = RosterDataFactory.getRosterLoadStartTime();
                            if( rosterLoadStartTime ) {
                                //console.log('TR: rosterLoadPerformanceDirective:  milliseconds:' + (Date.now() - rosterLoadStartTime - 5000));
                            }
                        }
                    }, 5000);
                }
            };
        }
    );

    // Start from filter
    angular.module('origin-components').filter('startFrom', function() {
        return function(input, start) {
            if (!!input) {
                return input.slice(start);
            } else {
                return [];
            }
        };
    });

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubRosterSection
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} titlestr the title string of the section
     * @param {BooleanEnumeration} showonlinecount boolean will show the number of online users in the section header if true
     * @param {string} filter Filter string which determines what items show up in the section
     * @description
     *
     * origin social hub -> roster area -> section
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-hub-roster-section
     *            titlestr="Pending Approval"
     *            showonlinecount="false"
     *            filter="FAVORITES"
     *         ></origin-social-hub-roster-section>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubRosterSectionCtrl', OriginSocialHubRosterSectionCtrl)
        .directive('originSocialHubRosterSection', function(ComponentsConfigFactory, RosterDataFactory, ObserverFactory, KeyEventsHelper) {

            return {
                restrict: 'E',
                controller: 'OriginSocialHubRosterSectionCtrl',
                scope: {
                    'titleStr': '@titlestr',
                    'showOnlineCount': '@showonlinecount',
                    'filter': '@'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/roster/views/section.html'),
                link: function(scope, elem, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    scope.$sectionElement = angular.element(elem).find('.origin-social-hub-section');
                    scope.$headerElement = angular.element(elem).find('.origin-social-hub-section-header');
                    scope.$viewportElement = angular.element(elem).find('.origin-social-hub-section-viewport');

                    function handleRightKeyup (e) {
                        e.stopPropagation();
                        ctrl.openSection();
                        scope.$digest();
                        angular.element(elem).focus();
                    }
                    function handleLeftKeyup (e) {
                        e.stopPropagation();
                        ctrl.closeSection();
                        scope.$digest();
                        angular.element(elem).focus();
                    }
                    function handleLeftClick () {
                        ctrl.toggleSectionVisible();
                    }
                    function handleKeyup (e) {
                        KeyEventsHelper.mapEvents(e, {
                            'right': handleRightKeyup,
                            'left': handleLeftKeyup
                        });
                    }

                    angular.element(elem).on('click', '.origin-social-hub-section-header', handleLeftClick);
                    angular.element(elem).on('keyup', handleKeyup);

                    function onDestroy () {
                        angular.element(elem).off('click', handleLeftClick);
                        angular.element(elem).off('keyup', handleKeyup);
                    }
                    scope.$on('$destroy', onDestroy);

                    ctrl.getRoster(scope.filter);

                    if (scope.showOnlineCount) {
                        ObserverFactory.create(RosterDataFactory.getOnlineCountWatch())
                            .attachToScope(scope, 'onlineCount');
                    }

                    ctrl.updateSectionToggleState();

                }
            };

        });
}());
