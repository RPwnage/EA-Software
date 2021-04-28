/**
 * Jasmine functional test
 */

'use strict';

var localStorage = {};

describe('Important Important Information Tile', function() {
    var $controller, UtilFactory;

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

    beforeEach(inject(function(_$controller_, _UtilFactory_) {
        $controller = _$controller_;
        UtilFactory = _UtilFactory_;
    }));

    describe('$scope', function() {
        var $scope, Md5Factory, controller;

        beforeEach(function() {
            $scope = {'$digest': function(){}, '$evalAsync': function(){}};
            Md5Factory = {
                md5: function() {
                    return "md5hash";
                }
            };
            controller = $controller('OriginGamelibraryOgdImportantInfoCtrl', {
                $scope: $scope,
                UtilFactory: UtilFactory,
                Md5Factory: Md5Factory
            });
        });

        it('Allows for dismissibility if the dismissibile flag is set to "true"', function() {
            $scope.dismissible = 'true';
            expect($scope.isDismissible()).toEqual(true);
        });

        it('Prohibits dismissibility if flag is set to "false"', function(){
            $scope.dismissible = 'false';
            expect($scope.isDismissible()).toEqual(false);
        });

        it('Prohibits dismissibility if flag is set to a value not in the value map', function() {
            $scope.dismissible = 'lizard';
            expect($scope.isDismissible()).toEqual(false);
        });

        it('Will be immediately visible if a start time and end time is not provided', function() {
            expect($scope.isWithinViewableDateRange()).toEqual(true);
        });

        it('Will be visible after the campaign begins', function() {
            var now = new Date();
            var campaignStartTime = new Date(now.getTime());
            campaignStartTime.setMinutes(campaignStartTime.getMinutes() - 1);

            $scope.startdate = campaignStartTime.toISOString();
            expect($scope.isWithinViewableDateRange()).toEqual(true);
        });

        it('Will not be visible before the campaign begins', function() {
            var now = new Date();
            var campaignStartTime = new Date(now.getTime());
            campaignStartTime.setMinutes(campaignStartTime.getMinutes() + 1);

            $scope.startdate = campaignStartTime.toISOString();
            expect($scope.isWithinViewableDateRange()).toEqual(false);
        });

        it('Will be deactivated when the campaign ends', function() {
            var now = new Date();
            var campaignEndTime = new Date(now.getTime());
            campaignEndTime.setMinutes(campaignEndTime.getMinutes() - 1);


            $scope.enddate = campaignEndTime.toISOString();
            expect($scope.isWithinViewableDateRange()).toEqual(false);
        });

        it('Will be active between valid start and end dates', function() {
            var now = new Date();
            var campaignStartTime = new Date(now.getTime());
            campaignStartTime.setMinutes(campaignStartTime.getMinutes() - 1);
            var campaignEndTime = new Date(now.getTime());
            campaignEndTime.setMinutes(campaignEndTime.getMinutes() + 1);

            $scope.startdate = campaignStartTime.toISOString();
            $scope.enddate = campaignEndTime.toISOString();
            expect($scope.isWithinViewableDateRange()).toEqual(true);
        });

        it('Will identify an icon class if a valid icon enumeration is provided', function() {
            $scope.icon = 'timer';
            expect($scope.getIconClass()).toEqual('otkicon-timer');
            expect($scope.hasIcon()).toEqual(true);
        });

        it('Will identify an icon class with a color if a valid icon and iconcolor enumeration is provided', function() {
            $scope.icon = 'warning';
            $scope.iconcolor = 'red';
            expect($scope.getIconClass()).toEqual('otkicon-warning red');
            expect($scope.hasIcon()).toEqual(true);
        });

        it('Will deny invalid icons', function() {
            $scope.icon = '290ei209ie029ie90';
            expect($scope.getIconClass()).toEqual('');
            expect($scope.hasIcon()).toEqual(false);
        });

        it('Will inidicate previously dimsissed items by storing the state in local storage', function() {
            $scope.title = "This is a title";
            $scope.offerid = "OFBEAST:12423";
            $scope.startdate = "2015-01-20T20:20:10Z";
            $scope.enddate = "2015-02-20T20:20:10Z";
            $scope.dismiss();
            expect($scope.isDismissed()).toEqual(true);
        });

        it('Will include a date html element when livecountdown is "true", offerid is set, the %enddate% template tag is provided, and enddate is filled in', function(){
            $scope.title = "This livecountdown ends on: %enddate%";
            $scope.livecountdown = "true";
            $scope.enddate = "2016-01-20T00:00:00Z";
            $scope.offerid = "Abc123";
            expect($scope.getTitle()).toEqual('This livecountdown ends on: <span class="important-info-timer"></span>');
        });

        it('Will provide a pass through title when livecountdown is false', function() {
            $scope.title = "This is a title";
            expect($scope.getTitle()).toEqual('This is a title');
        });
    });
});
