/**
 * Jasmine functional test
 */

'use strict';

describe('OGD Violator', function() {
    var $controller, UtilFactory, DateHelperFactory, moment, GamesDataFactory;

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide){
            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: function(){}
                };
            });
            $provide.factory('GamesDataFactory', function(){
                return {
                    events: {
                        'on': function(){},
                        'fire': function(){}
                    }
                };
            });
        });
    });

    beforeEach(inject(function(_$controller_, _GamesDataFactory_, _UtilFactory_, _DateHelperFactory_, _moment_) {
        $controller = _$controller_;
        GamesDataFactory = _GamesDataFactory_;
        UtilFactory = _UtilFactory_;
        DateHelperFactory = _DateHelperFactory_;
        moment = _moment_;
    }));

    describe('$scope', function() {
        var controller,
            $scope;

        beforeEach(function() {
            $scope = {
                '$on': function(){}
            };

            controller = $controller('OriginGamelibraryOgdViolatorCtrl', {
                $scope: $scope,
                GamesDataFactory: GamesDataFactory,
                UtilFactory: UtilFactory,
                DateHelperFactory: DateHelperFactory,
                moment: moment
            });
        });

        it('Generates a title for preloadon', function() {
            $scope.violatortype = 'preloadon';
            $scope.enddate = '2012-05-06T11:22:00Z';
            $scope.preloadon = 'Preload on %date%';
            expect($scope.getTitle()).toEqual('Preload on 05/06/2012');
        });

        it('Generates a title for releaseson', function() {
            $scope.violatortype = 'releaseson';
            $scope.enddate = '2012-05-06T11:22:00Z';
            $scope.releaseson = 'Releases %date%';
            expect($scope.getTitle()).toEqual('Releases 05/06/2012');
        });

        it('Generates a title for gameexpires', function() {
            var date = new Date('2012-05-06T11:22:00Z');
            $scope.violatortype = 'gameexpires';
            $scope.enddate = date;
            $scope.gameexpires = 'Expires on %date%';
            expect($scope.getTitle()).toEqual('Expires on 05/06/2012');
        });

        it('Generates a title for gamebeingsunset', function() {
            var date10days = new Date();
            date10days.setHours(date10days.getHours() + 246);
            $scope.violatortype = 'gamebeingsunset';
            $scope.enddate = date10days;
            $scope.gamebeingsunset = '{0} Game removed from vault today. | {1} Removed from vault in 1 day | ]1,+Inf] Removed from vault in %days% days';
            expect($scope.getTitle()).toEqual('Removed from vault in 10 days');
        });

        it('Generates a title for a known trial type (default case)', function() {
            $scope.violatortype = 'billingfailed';
            $scope.billingfailed = 'Billing Failed';
            expect($scope.getTitle()).toEqual('Billing Failed');
        });

        it('Gets a valid icon and color class', function() {
            $scope.violatortype = 'billingfailed';
            expect($scope.getIcon()).toEqual('warning');
            expect($scope.getIconColor()).toEqual('red');
        });

        it('Gets a live countdown status and the date to count to', function() {
            $scope.violatortype = 'trialnotexpired';
            expect($scope.getLiveCountdown()).toEqual('true');
        });
    });
});
