/**
 * Jasmine functional test
 */

'use strict';

describe('Gifting API Factory', function() {
    var GiftingFactory, ObjectHelperFactory, FeatureConfig, GiftStateConstants;

    beforeEach(function() {
        window.OriginComponents = {};

        function Communicator() {}
        Communicator.prototype.fire = function() {};

        window.Origin = {
            log: {
                'message': function(data) {console.log(data);}
            },
            utils: {
                os: function() { return 'PCWIN'; },
                Communicator: Communicator
            },
            user: {
                userPid: function() {}
            }
        };

        angular.mock.module('origin-components');

        module(function($provide) {
            $provide.factory('ObjectHelperFactory', function() {
                return {
                    transformWith: function() {}
                };
            });

            $provide.factory('FeatureConfig', function() {
                return {};
            });

            $provide.factory('GiftStateConstants', function() {
                return {
                    RECEIVED: 'ACCEPTED',
                    OPENED: 'ACTIVATED'
                };
            });
        });
    });

    beforeEach(inject(function(_GiftingFactory_, _ObjectHelperFactory_, _FeatureConfig_, _GiftStateConstants_) {
        GiftingFactory = _GiftingFactory_;
        ObjectHelperFactory = _ObjectHelperFactory_;
        FeatureConfig = _FeatureConfig_;
        GiftStateConstants = _GiftStateConstants_;
    }));

    describe('getGift', function() {
        var entitlements = [
            {
                'offerId':'Origin.OFR.50.0000940',
                'grantDate':'2017-02-24T22:41:00Z',
            }, {
                'offerId':'Origin.OFR.50.0001169',
                'grantDate':'2017-02-24T22:43:00Z',
            }, {
                'offerId':'Origin.OFR.50.0001111'
            }
        ];

        var allGifts = [
            {
                giftId: 1000000369651,
                message:null,
                offerId:'Origin.OFR.50.0001169',
                sendDate:10000,
                senderName:'subsm24',
                senderPersonaId:1000185381347,
                status:'ACTIVATED'
            }, {
                giftId:1000000169651,
                message:'sadfsaafasfas',
                offerId:'Origin.OFR.50.0000940',
                sendDate:10000,
                senderName:'subsm24',
                senderPersonaId:1000185381347,
                status:'ACCEPTED'
            }, {
                giftId:1000000169652,
                message:'sadfsaafasfas',
                offerId:'Origin.OFR.50.0000940',
                sendDate:20000,
                senderName:'subsm24',
                senderPersonaId:1000185381347,
                status:'ACCEPTED'
            }, {
                giftId:1000000169651,
                message:'sadfsaafasfas',
                offerId:'Origin.OFR.50.0001111',
                senderName:'subsm24',
                senderPersonaId:1000185381347,
                status:'ACCEPTED'
            }, {
                giftId:1000000169652,
                message:'sadfsaafasfas',
                offerId:'Origin.OFR.50.0001111',
                senderName:'subsm24',
                senderPersonaId:1000185381347,
                status:'ACCEPTED'
            }
        ];

        it('will return an opened gift', function() {
            GiftingFactory.extractUnopenedGifts(entitlements, allGifts);
            expect(GiftingFactory.getGift(allGifts[0].offerId, GiftStateConstants.OPENED)).toEqual(allGifts[0]);
        });
        
        it('will return undefined if looking for an unopened gift that is open', function() {
            GiftingFactory.extractUnopenedGifts(entitlements, allGifts);
            expect(GiftingFactory.getGift(allGifts[0].offerId, GiftStateConstants.RECEIVED)).toEqual(undefined);
        });

        it('will return the most recent unopened gift', function() {
            GiftingFactory.extractUnopenedGifts(entitlements, allGifts);
            expect(GiftingFactory.getGift(allGifts[2].offerId, GiftStateConstants.RECEIVED)).toEqual(allGifts[2]);
        });

        it('will return undefined if looking for an opened gift that is unopened', function() {
            GiftingFactory.extractUnopenedGifts(entitlements, allGifts);
            expect(GiftingFactory.getGift(allGifts[1].offerId, GiftStateConstants.OPENED)).toEqual(undefined);
        });

        it('will return undefined it it finds no matching opened gifts', function() {
            GiftingFactory.extractUnopenedGifts(entitlements, allGifts);
            expect(GiftingFactory.getGift('Origin.OFR.50.222222', GiftStateConstants.OPENED)).toEqual(undefined);
        });

        it('will return undefined it it finds no matching unopened gifts', function() {
            GiftingFactory.extractUnopenedGifts(entitlements, allGifts);
            expect(GiftingFactory.getGift('Origin.OFR.50.222222', GiftStateConstants.RECEIVED)).toEqual(undefined);
        });

        it('will return undefined if offerId is undefined and status is unopened', function() {
            GiftingFactory.extractUnopenedGifts(entitlements, allGifts);
            expect(GiftingFactory.getGift(undefined, GiftStateConstants.RECEIVED)).toEqual(undefined);
        });

        it('will return undefined if offerId is undefined and status is opened', function() {
            GiftingFactory.extractUnopenedGifts(entitlements, allGifts);
            expect(GiftingFactory.getGift(undefined, GiftStateConstants.OPENED)).toEqual(undefined);
        });

        it('will return undefined if status is undefined for an opened gift offerId', function() {
            GiftingFactory.extractUnopenedGifts(entitlements, allGifts);
            expect(GiftingFactory.getGift(allGifts[0].offerId, undefined)).toEqual(undefined);
        });

        it('will return undefined if status is undefined for an unopened gift offerId', function() {
            GiftingFactory.extractUnopenedGifts(entitlements, allGifts);
            expect(GiftingFactory.getGift(allGifts[1].offerId, undefined)).toEqual(undefined);
        });

        it('will return undefined if status is undefined for a non existant gift offerId', function() {
            GiftingFactory.extractUnopenedGifts(entitlements, allGifts);
            expect(GiftingFactory.getGift('Origin.OFR.50.222222', undefined)).toEqual(undefined);
        });

        it('will return undefined if offerId and status are undefined', function() {
            GiftingFactory.extractUnopenedGifts(entitlements, allGifts);
            expect(GiftingFactory.getGift(undefined, undefined)).toEqual(undefined);
        });

        it('will return the first gift if sendDate is undefined', function() {
            GiftingFactory.extractUnopenedGifts(entitlements, allGifts);
            expect(GiftingFactory.getGift(allGifts[3].offerId, GiftStateConstants.RECEIVED)).toEqual(allGifts[3]);
        });
    });
});