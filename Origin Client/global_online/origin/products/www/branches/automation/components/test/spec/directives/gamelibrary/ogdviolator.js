/**
 * Jasmine functional test
 */

'use strict';

describe('OGD Violator', function() {
    var $controller, UtilFactory, DateHelperFactory, moment, GamesDataFactory, PurchaseFactory;

    beforeEach(function() {
        window.OriginComponents = {};
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
            $provide.factory('PurchaseFactory', function(){
                return {
                    buy: function() {},
                    renewSubscription: function() {}
                };
            });
        });
    });

    beforeEach(inject(function(_$controller_, _GamesDataFactory_, _UtilFactory_, _DateHelperFactory_, _PurchaseFactory_, _moment_) {
        $controller = _$controller_;
        GamesDataFactory = _GamesDataFactory_;
        UtilFactory = _UtilFactory_;
        DateHelperFactory = _DateHelperFactory_;
        PurchaseFactory = _PurchaseFactory_;
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
            expect($scope.getTitle()).toContain('Preload on');
        });

        it('Generates a title for releaseson', function() {
            $scope.violatortype = 'releaseson';
            $scope.enddate = '2012-05-06T11:22:00Z';
            $scope.releaseson = 'Releases %date%';
            expect($scope.getTitle()).toContain('Releases');
        });

        it('Generates a title for gameexpires', function() {
            var date = new Date('2012-05-06T11:22:00Z');
            $scope.violatortype = 'gameexpires';
            $scope.enddate = date;
            $scope.gameexpires = 'Expires on %date%';
            expect($scope.getTitle()).toContain('Expires');
        });

        it('Generates a title for a known trial type (default case)', function() {
            $scope.violatortype = 'preorderfailed';
            $scope.preorderfailed = 'Preorder Failed';
            expect($scope.getTitle()).toEqual('Preorder Failed');
        });

        it('Gets a valid icon and color class', function() {
            $scope.violatortype = 'preorderfailed';
            expect($scope.getIcon()).toEqual('warning');
            expect($scope.getIconColor()).toEqual('red');
        });

        it('Gets a live countdown status and the date to count to', function() {
            $scope.violatortype = 'legacytrialnotexpired';
            expect($scope.getLiveCountdown()).toEqual('fixed-date');
        });
    });
});
