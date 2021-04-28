/**
 * Jasmine functional test
 */

'use strict';

describe('Origin Game Violator Component', function() {
    var $controller, UtilFactory, DateHelperFactory;

    beforeEach(function(){
        angular.mock.module('origin-components');
        module(function($provide) {
            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: function(message){}
                };
            });
        });
    });

    beforeEach(inject(function(_$controller_, _UtilFactory_, _DateHelperFactory_) {
        $controller = _$controller_;
        UtilFactory = _UtilFactory_;
        DateHelperFactory = _DateHelperFactory_;
    }));

    describe('Get Text', function() {
        var controller,
            $scope = {'$on': function(){}},
            $interval = function(){};

        beforeEach(function() {
            controller = $controller('OriginGameViolatorTimerCtrl', {
                '$scope': $scope,
                '$interval': $interval,
                'DateHelperFactory': DateHelperFactory,
                'UtilFactory': UtilFactory
            });
        });

        it('should respond with pluralized messages for days', function(){
            var futureDate = new Date();
            futureDate = futureDate.setHours(futureDate.getHours() + 26);
            $scope.enddate = futureDate;
            $scope.daysleft = '{0} less than a day | {1} a day | ]1,+Inf] %days% days';
            expect($scope.getText()).toBe('a day');
        });

        it('should respond with pluralized messages for hours', function(){
            var futureDate = new Date();
            futureDate = futureDate.setMinutes(futureDate.getMinutes() + 123);
            $scope.enddate = futureDate;
            $scope.hoursleft = '{0} less than an hour | {1} an hour | ]1,+Inf] %hours% hours';
            expect($scope.getText()).toBe('2 hours');
        });

        it('should respond with pluralized messages for minutes', function(){
            var futureDate = new Date();
            futureDate = futureDate.setSeconds(futureDate.getSeconds() + 200);
            $scope.enddate = futureDate;
            $scope.minutesleft = '{0} less than a minute | {1} 1 minute | ]1,+Inf] %minutes% minutes';
            expect($scope.getText()).toBe('3 minutes');
        });

        it('should respond with pluralized messages for seconds', function(){
            var futureDate = new Date();
            futureDate = futureDate.getSeconds(futureDate.getSeconds() + 20);
            $scope.enddate = futureDate;
            $scope.secondsleft = '{1} %seconds% second | ]0,+Inf] %seconds% seconds';
            expect($scope.getText()).toContain('seconds');
        });

        it('should respond with pluralized messages for past dates - defaults to 0 seconds', function(){
            var pastDate = new Date();
            pastDate = pastDate.getSeconds(pastDate.getSeconds() - 20);
            $scope.enddate = pastDate;
            $scope.secondsleft = '{1} %seconds% second | ]-1,+Inf] %seconds% seconds';
            expect($scope.getText()).toContain('0 seconds');
        });

        it('should supress display if the date is malformed', function(){
            $scope.enddate = 'foo bar';
            expect($scope.getText()).toBeFalsy();
        });
    });
});