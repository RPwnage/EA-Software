/**
 * Jasmine functional test
 */

'use strict';

describe('Origin Game Violator Timer Component', function() {
    var $controller, UtilFactory, DateHelperFactory;

    beforeEach(function(){
        window.Origin = {
            locale: {
                languageCode: function() {
                    return '';
                },
                countryCode: function() {
                    return '';
                },
                threeLetterCountryCode: function() {
                    return '';
                },
                locale: function() {
                    return '';
                }
            }
        };
        angular.mock.module('origin-components');
        module(function($provide) {
            $provide.factory('ComponentsLogFactory', function() {
                return {
                    log: function() {}
                };
            });

            $provide.factory('GameViolatorFactory', function() {
                return {
                    enddateReached: function() {}
                };
            });
        });
    });

    beforeEach(inject(function(_$controller_, _UtilFactory_, _DateHelperFactory_) {
        $controller = _$controller_;
        UtilFactory = _UtilFactory_;
        DateHelperFactory = _DateHelperFactory_;
    }));

    describe('Get Text - precise format (default)', function() {
        var controller,
            $scope = {'$on': function(){}},
            $interval = function(){};

        beforeEach(function() {
            $scope.format = 'precise';
            $scope.daysleft = '{0} less than a day | {1} a day | ]1,+Inf] %days% days';
            $scope.hoursleft = '{0} less than an hour | {1} an hour | ]1,+Inf] %hours% hours';
            $scope.minutesleft = '{0} less than a minute | {1} 1 minute | ]1,+Inf] %minutes% minutes';
            $scope.secondsleft = '{1} %seconds% second | ]-1,+Inf] %seconds% seconds';
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
            expect($scope.getText()).toBe('a day');
        });

        it('should respond with pluralized messages for hours', function(){
            var futureDate = new Date();
            futureDate = futureDate.setMinutes(futureDate.getMinutes() + 123);
            $scope.enddate = futureDate;
            expect($scope.getText()).toBe('2 hours');
        });

        it('should respond with pluralized messages for minutes', function(){
            var futureDate = new Date();
            futureDate = futureDate.setSeconds(futureDate.getSeconds() + 200);
            $scope.enddate = futureDate;
            expect($scope.getText()).toBe('3 minutes');
        });

        it('should respond with pluralized messages for seconds', function(){
            var futureDate = new Date();
            futureDate = futureDate.getSeconds(futureDate.getSeconds() + 20);
            $scope.enddate = futureDate;
            expect($scope.getText()).toContain('seconds');
        });

        it('should respond with pluralized messages for past dates - defaults to 0 seconds', function(){
            var pastDate = new Date();
            pastDate = pastDate.getSeconds(pastDate.getSeconds() - 20);
            $scope.enddate = pastDate;
            expect($scope.getText()).toContain('0 seconds');
        });

        it('should supress display if the date is malformed', function() {
            $scope.enddate = 'foo bar';
            expect($scope.getText()).toBeFalsy();
        });
    });

    describe('Get Text - trial format', function() {
        var controller,
            $scope = {'$on': function(){}},
            $interval = function(){};

        beforeEach(function() {
            $scope.format = 'trial';
            $scope.daystrial = 'less than %days% days';
            $scope.hourstrial = ']-1, 23] less than %hours% hours | {24} less than a day';
            $scope.minutestrial = ']-1, 5] less than 5 minutes |]5, 15] less than 15 minutes | ]15, 30] less than 30 minutes | ]30,+Inf] less than an hour';
            controller = $controller('OriginGameViolatorTimerCtrl', {
                '$scope': $scope,
                '$interval': $interval,
                'DateHelperFactory': DateHelperFactory,
                'UtilFactory': UtilFactory
            });
        });

        it('should show a trial formatted time for days left', function() {
            var futureDate = new Date();
            futureDate = futureDate.setMinutes(futureDate.getMinutes() + 1505);
            $scope.enddate = futureDate;
            expect($scope.getText()).toBe('less than 2 days');
        });

        it('should show a trial formatted time for hours left', function() {
            var futureDate = new Date();
            futureDate = futureDate.setMinutes(futureDate.getMinutes() + 125);
            $scope.enddate = futureDate;
            expect($scope.getText()).toBe('less than 3 hours');
        });

        it('should show a trial formatted time for minutes left', function() {
            var futureDate = new Date();
            futureDate = futureDate.setMinutes(futureDate.getMinutes() + 3);
            $scope.enddate = futureDate;
            expect($scope.getText()).toBe('less than 5 minutes');
        });

        it('Will not render a time with less than a minute remaining', function() {
            var futureDate = new Date();
            futureDate = futureDate.setSeconds(futureDate.getSeconds() + 56);
            $scope.enddate = futureDate;
            expect($scope.getText()).toBeUndefined();
        });
    });
});