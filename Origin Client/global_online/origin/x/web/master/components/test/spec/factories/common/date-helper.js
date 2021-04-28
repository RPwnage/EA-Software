/**
 * Jasmine functional test
 */

'use strict';

describe('Date Factory', function() {
    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide){
            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: function(message){}
                }
            })
        });
    });

    var dateHelper;
    var futureDate = new Date();
    futureDate.setMinutes(futureDate.getMinutes()+20);
    var pastDate = new Date();
    pastDate.setMinutes(pastDate.getMinutes()-20);

    beforeEach(inject(function(_DateHelperFactory_){
        dateHelper = _DateHelperFactory_;
    }));

    describe('Testing date object validity', function() {
        it('will accept a real javascript date object', function(){
            var date = new Date();
            expect(dateHelper.isValidDate(date)).toBe(true);
        });

        it('will reject an invalid date or garbage input', function() {
            expect(dateHelper.isValidDate('foo bar')).toBe(false);
            expect(dateHelper.isValidDate(0)).toBe(false);
            expect(dateHelper.isValidDate()).toBe(false);
            expect(dateHelper.isValidDate({})).toBe(false);
        });
    });

    describe('Testing isInTheFuture functionality', function() {
        it('will return true if the date is in the future', function() {
            expect(dateHelper.isInTheFuture(futureDate)).toBe(true);
        });
        it('will return false if the date is in the past', function() {
            expect(dateHelper.isInTheFuture(pastDate)).toBe(false);
        });
        it('will return false if the date is malformed', function() {
            expect(dateHelper.isInTheFuture('foo')).toBe(false);
        });
    });

    describe('Testing isInThePast functionality', function() {
        it('will return true if the date is in the past', function() {
            expect(dateHelper.isInThePast(pastDate)).toBe(true);
        });
        it('will return false if the date is in the future', function() {
            expect(dateHelper.isInThePast(futureDate)).toBe(false);
        });
        it('will return false if the date is malformed', function() {
            expect(dateHelper.isInThePast('foo')).toBe(false);
        });
    });

    describe('Testing Add Days functionality', function() {
        it('Will non destructively add days to a date field', function() {
            expect(dateHelper.isInTheFuture(dateHelper.addDays(pastDate, 7))).toBe(true);
            expect(dateHelper.isInThePast(pastDate)).toBe(true);
        });
    });

    describe('Testing countdown data getter', function() {
        it('Will work if only a future date is supplied', function(){
            var oneDayInFuture = new Date();
            oneDayInFuture.setHours(oneDayInFuture.getHours()+26);

            var data = dateHelper.getCountdownData(oneDayInFuture);
            expect(data.days).toBe(1);
        });

        it('Will break down a date 26 hours into the future', function() {
            var oneDayInFuture = new Date();
            var currentDate = new Date(oneDayInFuture); //clone date
            oneDayInFuture.setHours(oneDayInFuture.getHours()+26);
            oneDayInFuture.setMinutes(oneDayInFuture.getMinutes()+3);
            oneDayInFuture.setSeconds(oneDayInFuture.getSeconds()+12);

            var data = dateHelper.getCountdownData(oneDayInFuture, currentDate);
            expect(data.days).toBe(1);
            expect(data.hours).toBe(2);
            expect(data.minutes).toBe(3);
            expect(data.seconds).toBe(12);
        });

        it('Will return 0 for all fields if a past date is supplied', function() {
            var oneDayInPast = new Date();
            oneDayInPast.setHours(oneDayInPast.getHours()-26);

            var data = dateHelper.getCountdownData(oneDayInPast);
            expect(data.days).toBe(0);
            expect(data.hours).toBe(0);
            expect(data.minutes).toBe(0);
            expect(data.seconds).toBe(0);
        });
    });
});