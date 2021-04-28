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

    describe('Testing Subtract Days functionality', function() {
        it('Will non destructively Subtract days to a date field', function() {
            expect(dateHelper.isInThePast(dateHelper.subtractDays(futureDate, 7))).toBe(true);
            expect(dateHelper.isInTheFuture(futureDate)).toBe(true);
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

        it('Will return the number total number of seconds and milliseconds until a timestamp', function() {
            var oneDayInFuture = new Date();
            var currentDate = new Date(oneDayInFuture); //clone date
            oneDayInFuture.setHours(oneDayInFuture.getHours()+24);

            var data = dateHelper.getCountdownData(oneDayInFuture, currentDate);
            //Because the test code can get lagged, allow up to a 5 second variance
            expect(data.totalSeconds).toBeGreaterThan(86399);
            expect(data.totalSeconds).toBeLessThan(86405);
            expect(data.totalMillis).toBeGreaterThan(86399999);
            expect(data.totalMillis).toBeLessThan(86405000);
        });
    });

    describe('Testing same date tester', function() {
        it('will compare two dates', function() {
            var date1 = new Date();
            var date2 = _.clone(date1);

            expect(dateHelper.isSameDate(date1, date2)).toBe(true);
        });

        it('will cope with non Date objects as input', function() {
            var date1 = new Date();
            var date2;

            expect(dateHelper.isSameDate(date1, date2)).toBe(false);
        });
    });

    describe('is between dates', function() {
        it('will evaluate true if the current time is between a start and end date', function() {
            expect(dateHelper.isBetweenDates(pastDate, futureDate)).toBe(true);
            expect(dateHelper.isBetweenDates(pastDate, pastDate)).toBe(false);
            expect(dateHelper.isBetweenDates(futureDate, futureDate)).toBe(false);
        });
    });

    describe('formatDateWithTimezone', function() {
        it('will take a UTC date object and format it to the user\'s tz', function() {
            var testDate = new Date('2016-10-18T04:00:00Z');

            expect(dateHelper.formatDateWithTimezone(testDate, {timezone: 'America/Vancouver'})).toEqual('October 17, 2016 9:00 PM PDT');
            expect(dateHelper.formatDateWithTimezone(testDate, {timezone: 'America/New_York'})).toEqual('October 18, 2016 12:00 AM EDT');
            expect(dateHelper.formatDateWithTimezone(testDate, {timezone: 'Australia/Brisbane'})).toEqual('October 18, 2016 2:00 PM AEST');
        });

        it('will take a local date and format it to the user\'s tz', function() {
            var testDate = new Date('2016-10-17T21:00:00-07:00'); // Pacific Daylight Time example

            expect(dateHelper.formatDateWithTimezone(testDate, {timezone: 'America/Vancouver'})).toEqual('October 17, 2016 9:00 PM PDT');
            expect(dateHelper.formatDateWithTimezone(testDate, {timezone: 'America/New_York'})).toEqual('October 18, 2016 12:00 AM EDT');
            expect(dateHelper.formatDateWithTimezone(testDate, {timezone: 'Australia/Brisbane'})).toEqual('October 18, 2016 2:00 PM AEST');
        });

        it('observes daylight savings time', function() {
            var testDate = new Date('2016-12-17T21:00:00-08:00'); // Pacific Standard Time example

            expect(dateHelper.formatDateWithTimezone(testDate, {timezone: 'America/Vancouver'})).toEqual('December 17, 2016 9:00 PM PST');
        });

        it('it will guess a user\'s timezone if none is provided', function() {
            var testDate = new Date('2016-12-17T21:00:00-08:00'); // Pacific Standard Time example

            expect(dateHelper.formatDateWithTimezone(testDate)).toContain('December');
        });

        it('Will reject invalid dates', function() {
            var testDate;

            expect(dateHelper.formatDateWithTimezone(testDate, {timezone: 'America/Vancouver'})).toEqual('');
        });

        it('Will allow users to override the default formatting', function() {
            var testDate = new Date('2016-12-17T21:00:00-08:00'); // Pacific Standard Time example

            expect(dateHelper.formatDateWithTimezone(testDate, {
                timezone: 'America/Vancouver',
                format: 'L'
            })).toEqual('12/17/2016');
        })
    });
});