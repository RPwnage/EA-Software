describe('Origin Games API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.games.baseGameEntitlements()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.games.baseGameEntitlements(true).then(function(result) {

            var expected = [{
                "entitlementId" : 1000100595502,
                "entitlementTag" : "ORIGIN_DOWNLOAD",
                "entitlementType" : "ORIGIN_DOWNLOAD",
                "grantDate" : "2014-08-19T22:11:00Z",
                "isConsumable" : false,
                "offerId" : "OFB-EAST:50669",
                "originPermissions" : "0",
                "projectId" : "307302",
                "status" : "ACTIVE",
                "updatedDate" : "2014-11-07T12:12:16Z",
                "useCount" : 0,
                "version" : 0
            }, {
                "entitlementId" : 1000100995502,
                "entitlementTag" : "ORIGIN_DOWNLOAD",
                "entitlementType" : "ORIGIN_DOWNLOAD",
                "grantDate" : "2014-08-19T22:12:00Z",
                "isConsumable" : false,
                "offerId" : "OFB-EAST:59474",
                "originPermissions" : "0",
                "projectId" : "307302",
                "status" : "ACTIVE",
                "updatedDate" : "2015-03-04T06:50:07Z",
                "useCount" : 0,
                "version" : 0
            }, {
                "entitlementId" : 1000100795502,
                "entitlementTag" : "ORIGIN_DOWNLOAD",
                "entitlementType" : "ORIGIN_DOWNLOAD",
                "grantDate" : "2014-08-19T22:12:00Z",
                "isConsumable" : false,
                "offerId" : "OFB-EAST:58847",
                "originPermissions" : "0",
                "projectId" : "307302",
                "status" : "ACTIVE",
                "updatedDate" : "2015-03-03T13:24:43Z",
                "useCount" : 0,
                "version" : 0
            }];

            for (var i = 0; i < expected.length; i++) {
                expect(result).toContain(expected[i]);
            }

            expect(result.length).toEqual(expected.length, 'returned result should have ' + expected.length + ' entries');
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.games.catalogInfo()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.games.catalogInfo('Origin.OFR.50.0000143', 'en_US', false).then(function(result) {
            expect(result.offerId).toEqual('Origin.OFR.50.0000143');
            expect(result.defaultLocale).toEqual('DEFAULT');
            asyncTicker.clear(done);
        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.games.extraContentEntitlements()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.games.extraContentEntitlements('182555').then(function(result) {

            var expected = [{
                "entitlementId" : 1000102395502,
                "entitlementSource" : "ORIGIN-CODE-WEB",
                "entitlementTag" : "TOY_ROLLER_COASTER_CROWN",
                "entitlementType" : "DEFAULT",
                "grantDate" : "2014-09-18T21:20:00Z",
                "groupName" : "SimCityPCDLC",
                "isConsumable" : false,
                "offerId" : "OFB-EAST:109552718",
                "originPermissions" : "0",
                "productCatalog" : "OFB",
                "projectId" : "307382",
                "status" : "ACTIVE",
                "updatedDate" : "2014-01-09T11:31:01Z",
                "useCount" : 0,
                "version" : 0
            }, {
                "entitlementId" : 1000102195502,
                "entitlementSource" : "ORIGIN-CODE-WEB",
                "entitlementTag" : "CITY_WW_BERLIN",
                "entitlementType" : "DEFAULT",
                "grantDate" : "2014-09-18T21:17:00Z",
                "groupName" : "SimCityPCDLC",
                "isConsumable" : false,
                "offerId" : "OFB-EAST:60173",
                "originPermissions" : "0",
                "productCatalog" : "OFB",
                "projectId" : "301779",
                "status" : "ACTIVE",
                "updatedDate" : "2015-03-09T18:07:30Z",
                "useCount" : 0,
                "version" : 0
            }, {
                "entitlementId" : 1000102595502,
                "entitlementSource" : "ORIGIN-CODE-WEB",
                "entitlementTag" : "SC_CITIES_OF_TOMORROW",
                "entitlementType" : "DEFAULT",
                "grantDate" : "2014-09-18T21:20:00Z",
                "groupName" : "SimCityPC",
                "isConsumable" : false,
                "offerId" : "OFB-EAST:65822",
                "originPermissions" : "0",
                "productCatalog" : "OFB",
                "projectId" : "307382",
                "status" : "ACTIVE",
                "updatedDate" : "2015-01-07T19:29:53Z",
                "useCount" : 0,
                "version" : 0
            }];

            for (var i = 0; i < expected.length; i++) {
                expect(result.entitlements).toContain(expected[i]);
            }

            expect(result.entitlements.length).toEqual(expected.length, 'returned result should have ' + expected.length + ' entries');
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.games.getOcdByOfferId() en_US', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.games.getOcdByOfferId('OFB-EAST:109552155', 'en_US').then(function(result) {

            var expected = {
                "headers" : "Content-Type: application/json\r\n",
                "data" : {
                    "gamehub" : {
                        "name" : "standard-edition",
                        "locale" : "en",
                        "components" : {
                            "origin-home-recommended-action-trial" : {
                                "image" : "/content/dam/originx/web/app/home/actions/tile-bf4-long.png",
                                "title" : "Battlefield 4™ (Origin Game Time) - It's Game Time!",
                                "subtitle" : "Continue your trial!",
                                "description" : "You have 48 hours left in your trial."
                            },
                            "origin-home-recommended-action-ito" : {
                                "title" : "bf4 title",
                                "description" : "bf4 description",
                                "offerid" : "111"
                            }
                        },
                        "subhubs" : { },
                        "offers" : {
                            "OFB-EAST:109546867" : "/games/OFB-EAST:109546867/offer.ocd",
                            "OFB-EAST:109552156" : "/games/OFB-EAST:109552156/offer.ocd",
                            "OFB-EAST:109552154" : "/games/OFB-EAST:109552154/offer.ocd"
                        },
                        "keys" : {
                            "sling" : "/content/web/app/games/battlefield/battlefield-4/standard-edition",
                            "offerId" : "/games/OFB-EAST:109552155/offer.ocd"
                        }
                    }
                }
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });
    });

    it('Origin.games.getOcdByOfferId()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.games.getOcdByOfferId('OFB-EAST:109552155').then(function(result) {

            var expected = {
                "headers" : "Content-Type: application/json\r\n",
                "data" : {
                    "gamehub" : {
                        "name" : "standard-edition",
                        "locale" : "en",
                        "components" : {
                            "origin-home-recommended-action-trial" : {
                                "image" : "/content/dam/originx/web/app/home/actions/tile-bf4-long.png",
                                "title" : "Battlefield 4™ (Origin Game Time) - It's Game Time!",
                                "subtitle" : "Continue your trial!",
                                "description" : "You have 48 hours left in your trial."
                            },
                            "origin-home-recommended-action-ito" : {
                                "title" : "bf4 title",
                                "description" : "bf4 description",
                                "offerid" : "111"
                            }
                        },
                        "subhubs" : { },
                        "offers" : {
                            "OFB-EAST:109546867" : "/games/OFB-EAST:109546867/offer.ocd",
                            "OFB-EAST:109552156" : "/games/OFB-EAST:109552156/offer.ocd",
                            "OFB-EAST:109552154" : "/games/OFB-EAST:109552154/offer.ocd"
                        },
                        "keys" : {
                            "sling" : "/content/web/app/games/battlefield/battlefield-4/standard-edition",
                            "offerId" : "/games/OFB-EAST:109552155/offer.ocd"
                        }
                    }
                }
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });
    });

    it('Origin.games.getOcdByPath()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.games.getOcdByPath('en_US', 'fifa', 'fifa-15', 'standard-edition').then(function(result) {

            var expected = {
                "headers" : "Content-Type: application/json\r\n",
                "data" : {
                    "gamehub" : {
                        "name" : "standard-edition",
                        "locale" : "en",
                        "components" : { },
                        "subhubs" : { },
                        "offers" : {
                            "Origin.OFR.50.0000570" : "/games/Origin.OFR.50.0000570/offer.ocd",
                            "Origin.OFR.50.0000622" : "/games/Origin.OFR.50.0000622/offer.ocd",
                            "Origin.OFR.50.0000571" : "/games/Origin.OFR.50.0000571/offer.ocd",
                            "Origin.OFR.50.0000581" : "/games/Origin.OFR.50.0000581/offer.ocd",
                            "Origin.OFR.50.0000462" : "/games/Origin.OFR.50.0000462/offer.ocd",
                            "Origin.OFR.50.0000761" : "/games/Origin.OFR.50.0000761/offer.ocd",
                            "Origin.OFR.50.0000536" : "/games/Origin.OFR.50.0000536/offer.ocd"
                        },
                        "keys" : {
                            "sling" : "/content/web/app/games/fifa/fifa-15/standard-edition"
                        }
                    }
                }
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

        // Reset mockup response queue just in case
        var endPoint = 'http://127.0.0.1:1400/response/reset';
        var req = new XMLHttpRequest();
        req.open('POST', endPoint, false);
        req.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded; charset=utf-8');
        req.send('');
    });

});
