describe('Origin Subscription API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.subs.userSubscriptionBasic()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.subscription.userSubscriptionBasic().then(function(result) {
            var expected = {
                firstSignUpDate : '2015-09-23T23:05:31',
                firstSignUpSubs : '/subscriptionsv2/1000020317627',
                subscriptionUri : ['/subscriptionsv2/1000020317627']
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.subs.userSubscriptionDetails()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        var expectedSubscription = {
            "userUri" : "/users/1000201921887",
            "offerUri" : "/offers/Origin.OFR.50.0000746",
            "source" : "ORIGIN-STORE-CLIENT-CA",
            "status" : "ENABLED",
            "dateCreated" : "2015-04-15T23:43:41",
            "dateModified" : "2015-04-15T23:43:43",
            "entitlementsUri" : "subscriptionsv2/1000000121887/entitlements",
            "eventsUri" : "/subscriptionsv2/1000000121887/events",
            "invoicesUri" : "/subscriptionsv2/1000000121887/invoice",
            "scheduleOpesUri" : "/subscriptionsv2/1000000121887/scheduledOperations",
            "subscriptionUri" : "/subscriptionsv2/1000000121887",
            "billingMode" : "RECURRING",
            "anniversaryDay" : 15,
            "subsStart" : "2015-04-15T23:43:43",
            "subsEnd" : "2098-07-15T23:43:42",
            "nextBillingDate" : "2015-05-15T23:43:43",
            "freeTrial" : false,
            "accountUri" : "/billingaccounts/1010081321887"
        };

        var expectedSubscriptionEvent = {
            "SubscriptionEvent" : [
            { "eventType" : "CREATED", "eventDate" : "2015-04-15T23:43:43", "eventStatus" : "COMPLETED", "eventId" : 1000000321887 },
            { "eventType" : "PROVISONED", "eventDate" : "2015-04-15T23:46:41", "eventStatus" : "COMPLETED", "eventId" : 1000001921887 },
            { "eventType" : "PROVISONED", "eventDate" : "2015-04-21T01:11:36", "eventStatus" : "COMPLETED", "eventId" : 1000002121887 },
            { "eventType" : "PROVISONED", "eventDate" : "2015-04-23T06:28:32", "eventStatus" : "COMPLETED", "eventId" : 1000020321887 },
            { "eventType" : "PROVISONED", "eventDate" : "2015-04-23T06:30:00", "eventStatus" : "COMPLETED", "eventId" : 1000022521887 },
            { "eventType" : "PROVISONED", "eventDate" : "2015-04-23T06:32:13", "eventStatus" : "COMPLETED", "eventId" : 1000023921887 },
            { "eventType" : "PROVISONED", "eventDate" : "2015-04-23T06:32:42", "eventStatus" : "COMPLETED", "eventId" : 1000024121887 },
            { "eventType" : "PROVISONED", "eventDate" : "2015-04-23T06:34:29", "eventStatus" : "COMPLETED", "eventId" : 1000024321887 },
            { "eventType" : "PROVISONED", "eventDate" : "2015-04-23T20:06:30", "eventStatus" : "COMPLETED", "eventId" : 1000040921887 },
            { "eventType" : "PROVISONED", "eventDate" : "2015-04-24T22:10:35", "eventStatus" : "COMPLETED", "eventId" : 1000044321887 },
            { "eventType" : "PROVISONED", "eventDate" : "2015-05-06T08:48:11", "eventStatus" : "COMPLETED", "eventId" : 1000058521887 }
        ]};

        var expectedSubscriptionOperation = {
            "ScheduleOperation" : [
            {   "operationId" : "1000000121887",
                "operationName" : "NOTIFIED",
                "status" : "PENDING",
                "scheduleDate" : "2015-05-10T23:43:43",
                "PropertiesInfo" : [
                    {   "name" : "NOTIFY_TYPE",
                        "value" : "PRE_RECURRING"
                    },
                    {   "name" : "CYCLE COUNT",
                        "value" : "2"
                    }
                ]
            },
            {   "operationId" : "1000000721887",
                "operationName" : "CYCLE",
                "status" : "PENDING",
                "scheduleDate" : "2015-05-15T23:43:43",
                "PropertiesInfo" : [
                    {   "name" : "CYCLE COUNT",
                        "value" : "2"
                    },
                    {   "name" : "CYCLE DURATION",
                       "value":
                        "<CycleDurationDate><start>2015-04-15T23:43:43</start><end>2015-05-15T23:43:42</end></CycleDurationDate>"
                    }
                ]
            },
            {   "operationId" : "1000001321887",
                "operationName" : "RENEWED",
                "status" : "PENDING",
                "scheduleDate" : "2098-07-15T23:43:43",
                "PropertiesInfo" : [
                    {   "name" : "TARGET_OFFER_ID",
                        "value" : "Origin.OFR.50.0000746"
                    }
                ]
            },
            {   "operationId" : "1000001721887",
                "operationName" : "NOTIFIED",
                "status" : "PENDING",
                "scheduleDate" : "2098-07-12T23:43:43",
                "PropertiesInfo" : [
                    {   "name" : "NOTIFY_TYPE",
                        "value" : "PRE_AUTORENEW"
                    },
                    {   "name" : "RENEW_IN_DAYS",
                        "value" : "3"
                    }
                ]
            }
        ]};

        Origin.subscription.userSubscriptionDetails("/users/1000201921887", true).then(function(result) {
            expect(result.Subscription).toEqual(expectedSubscription);
            expect(result.GetSubscriptionEventsResponse).toEqual(expectedSubscriptionEvent);
            expect(result.GetScheduleOperationResponse).toEqual(expectedSubscriptionOperation);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.subs.getUserVaultInfo()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(5000, done, 'Expect onResolve() to be executed');

        Origin.subscription.getUserVaultInfo(true).then(function(result) {
            expect(result.game[0].offerId).toEqual("OFB-EAST:48217");
            expect(result.game[1].offerId).toEqual("OFB-EAST:109547339");
            expect(result.game[2].offerId).toEqual("DR:225064100");
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });
    
    it('Origin.subscription.vaultEntitle()', function(done) {
       
       var asyncTicker = new AsyncTicker();
       asyncTicker.tick(1000, done, 'Expect vaultEntitle() to be resolved');
       
       Origin.subscription.vaultEntitle('1000020112889', 'Origin.OFR.50.0000846').then(function(result) {
           
           var expected = {
               fulfillmentResponse : {
                   entitlementUris : {
                       entitlementUri : [
                           "/entitlements/1010157312889",
                           "/entitlements/1010157512889",
                           "/entitlements/1010157712889",
                           "/entitlements/1010157912889",
                           "/entitlements/1010158112889",
                           "/entitlements/1010158312889",
                           "/entitlements/1010158512889",
                           "/entitlements/1010158712889",
                           "/entitlements/1010158912889",
                           "/entitlements/1010159112889",
                           "/entitlements/1010159312889",
                           "/entitlements/1010159512889"
                       ]
                   },
                   subscriptionUris : {
                       subscriptionUri : "/subscriptionsv2/1000020112889"
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
    
    it('Origin.subscription.vaultRemove()', function(done) {
       
       var asyncTicker = new AsyncTicker();
       asyncTicker.tick(1000, done, 'Expect vaultRemove() to be resolved');
       
       Origin.subscription.vaultRemove('1000000121887', 'OFB-EAST:109549060').then(function(result) {
           expect(result).toBeTruthy();
           asyncTicker.clear(done);
       }).catch(function(error) {
          expect().toFail(error.message);
          asyncTicker.clear(done); 
       }); 
              
    });

    it('Origin.subscription.hasUsedTrial()', function(done) {

       var asyncTicker = new AsyncTicker();
       asyncTicker.tick(1000, done, 'Expect hasUsedTrial() to be resolved');

       Origin.subscription.hasUsedTrial().then(function(result) {
           expect(result).toEqual({hasUsedTrial: false});
           asyncTicker.clear(done);
       }).catch(function(error) {
          expect().toFail(error.message);
          asyncTicker.clear(done);
       });

    });
    
});
