describe('eax-experimentsdk', function() {

    beforeEach(function() {
        originalTimeout = jasmine.DEFAULT_TIMEOUT_INTERVAL;
        jasmine.DEFAULT_TIMEOUT_INTERVAL = 10000;
        jasmine.Ajax.install();
    });

    afterEach(function() {
        jasmine.DEFAULT_TIMEOUT_INTERVAL = originalTimeout;
        jasmine.Ajax.uninstall();
    });

    //tests successful initialization, including successful load of segments and experiments
    it('testing for Experiment.init() success returning true', function(done) {
        var req;

//        console.log('testing for Experiment.init() success returning true');

        Experiment.init('dev', 'preview', 'usa', 'en-us')
            .then(function(result) {
                expect(result).toBe(true);
                done();
            });

        req = jasmine.Ajax.requests.at(0);

        //segment data
        req.respondWith({
            "status": 200,
            "contentType": "application/json",
            "responseText": segmentDataStr
        });

        //wait for the above response to get handled and initiate the next xhr request
        //otherise requests.at(1) will return null
        setTimeout(function() {
            var req;
            req = jasmine.Ajax.requests.at(1);
            //experiment data
            jasmine.Ajax.requests.at(1).respondWith({
                'status': 200,
                'contentType': 'application/json',
                'responseText': experimentDataStr
            });

        }, 1000);
    });

    it('testing for loadSegment failure and Experiment.init() will return error', function(done) {
        var req;

//        console.log('testing for loadSegment failure and Experiment.init() will return error');

        Experiment.init('dev', 'preview', 'usa', 'en-us')
            .then(function(result) {
                expect(result).not.toBe(true);
                done();
            }).catch(function(error){
                expect(error).not.toBe(true);
                done();
            })

        req = jasmine.Ajax.requests.at(0);

        //segment data
        req.respondWith({
            "status": 404,
            "contentType": "application/json",
            "responseText": ""
        });
    });

    it('testing for loadSegment failure and Experiment.init() will return error', function(done) {
        var req;

//        console.log('testing for loadSegment failure and Experiment.init() will return error');

        Experiment.init('dev', 'preview', 'usa', 'en-us')
            .then(function(result) {
                expect(result).not.toBe(true);
                done();
            }).catch(function(error){
                expect(error).not.toBe(true);
                done();
            })

        req = jasmine.Ajax.requests.at(0);

        //segment data
        req.respondWith({
            "status": 200,
            "contentType": "application/json",
            "responseText": segmentDataStr
        });

        //wait for the above response to get handled and initiate the next xhr request
        //otherise requests.at(1) will return null
        setTimeout(function() {
            var req;
            req = jasmine.Ajax.requests.at(1);

            //experiment data
            jasmine.Ajax.requests.at(1).respondWith({
                'status': 404,
                'contentType': 'application/json',
                'responseText': ''
            });

        }, 1000);

    });

    it('testing Experiment.setUser() to be called with non-null/non-undefined userdata', function() {
        //dummy data
        var userData = {
            originId: 'masgflow',
            personaId: '393132267',
            userPID: '',
            underAge: false,
            dob: '1901-01-01',
            email: 'myamada@ea.com',
            emailStatus: 'VERIFIED',
            emailSignup: false,
            locale: 'en_us',
            storeFront: 'usa',
            subscriber: false,
            subscriberInfo: {},
            entitlements: ['OFB-EAST:109546867', 'Origin.OFR.50.0000524', 'Origin.OFR.50.0001662','DR:225064100', 'OFB-EAST:46560', 'DR:224434500', 'OFB-EAST:42366', 'OFB-EAST:109552318'],
            extraContentEntitlements: ['OFB-EAST:109549069', 'OFB-EAST:109550764', 'Origin.OFR.50.0001379']
        };

//        console.log('testing Experiment.setUser() to be called with non-null/non-undefined userdata');

        spyOn(Experiment, 'setUser').and.callThrough();
        Experiment.setUser(userData);
        expect(Experiment.setUser).not.toHaveBeenCalledWith(null);
        expect(Experiment.setUser).not.toHaveBeenCalledWith(undefined);
    });


    it('test for Experiment.inExperiment() to be bucket user in variant', function (done) {
        //tests  init (to succeed), setUser, and inExperiment (bucketed)
        var req,

            userData = {
                originId: 'masgflow',
                personaId: '393132267',
                userPID: '12297642087',
                underAge: false,
                dob: '1901-01-01',
                email: 'myamada@ea.com',
                emailStatus: 'VERIFIED',
                emailSignup: false,
                locale: 'en_us',
                storeFront: 'usa',
                subscriber: false,
                subscriberInfo: {},
                entitlements: ['OFB-EAST:109546867', 'Origin.OFR.50.0000524', 'Origin.OFR.50.0001662','DR:225064100', 'OFB-EAST:46560', 'DR:224434500', 'OFB-EAST:42366', 'OFB-EAST:109552318'],
                extraContentEntitlements: ['OFB-EAST:109549069', 'OFB-EAST:109550764', 'Origin.OFR.50.0001379']
            },

            successObj = {
                'result': true,
                'addedCustomDimension': true
            };

//        console.log('test for Experiment.inExperiment() to be bucket user in variant');

        Experiment.init('dev', 'preview', 'usa', 'en-us')
            .then(function(result) {
                expect(result).toBe(true);

                spyOn(Experiment, 'setUser').and.callThrough();
                Experiment.setUser(userData);
                expect(Experiment.setUser).not.toHaveBeenCalledWith(null);
                expect(Experiment.setUser).not.toHaveBeenCalledWith(undefined);

                Experiment.inExperiment('my-home-video-button', 'showButton')
                    .then(function(result) {
                        expect(result).toEqual(successObj);
                        done();
                    });

            });

        req = jasmine.Ajax.requests.at(0);

        //segment data
        req.respondWith({
            "status": 200,
            "contentType": "application/json",
            "responseText": segmentDataStr
        });

        setTimeout(function() {
            var req;
            req = jasmine.Ajax.requests.at(1);
            //experiment data
            jasmine.Ajax.requests.at(1).respondWith({
                'status': 200,
                'contentType': 'application/json',
                'responseText': experimentDataStr
            });

        }, 1000);

    });


    it('testing for Experiment.inExperiment() not bucketing user in variant', function (done) {
        //tests  init (to succeed), setUser, and not in specified variant
        var req,

            userData = {
                originId: 'masgflow',
                personaId: '393132267',
                userPID: '12296602350',
                underAge: false,
                dob: '1901-01-01',
                email: 'myamada@ea.com',
                emailStatus: 'VERIFIED',
                emailSignup: false,
                locale: 'en_us',
                storeFront: 'usa',
                subscriber: false,
                subscriberInfo: {},
                entitlements: ['OFB-EAST:109546867', 'Origin.OFR.50.0000524', 'Origin.OFR.50.0001662','DR:225064100', 'OFB-EAST:46560', 'DR:224434500', 'OFB-EAST:42366', 'OFB-EAST:109552318'],
                extraContentEntitlements: ['OFB-EAST:109549069', 'OFB-EAST:109550764', 'Origin.OFR.50.0001379']
            },

            notVariantObj = {
                'result': false,
                'addedCustomDimension': true
            };

//        console.log('testing for Experiment.inExperiment() not bucketing user in variant');

        Experiment.init('dev', 'preview', 'usa', 'en-us')
            .then(function(result) {
                expect(result).toBe(true);

                spyOn(Experiment, 'setUser').and.callThrough();
                Experiment.setUser(userData);
                expect(Experiment.setUser).not.toHaveBeenCalledWith(null);
                expect(Experiment.setUser).not.toHaveBeenCalledWith(undefined);

                Experiment.inExperiment('my-home-video-button', 'showButton')
                    .then(function(result) {
                        expect(result).toEqual(notVariantObj);
                        done();
                    });

            });

        req = jasmine.Ajax.requests.at(0);

        //segment data
        req.respondWith({
            "status": 200,
            "contentType": "application/json",
            "responseText": segmentDataStr
        });

        setTimeout(function() {
            var req;
            req = jasmine.Ajax.requests.at(1);
            //experiment data
            jasmine.Ajax.requests.at(1).respondWith({
                'status': 200,
                'contentType': 'application/json',
                'responseText': experimentDataStr
            });

        }, 1000);

    });


    it('testing for Experiment.inExperiment() failing bucketing because not in segment', function (done) {
        //tests  init (to succeed), setUser, and inExperiment (not in segment)
        var req,

            userData = {
                originId: 'masgflow',
                personaId: '393132267',
                userPID: '12297642087',
                underAge: false,
                dob: '1901-01-01',
                email: 'myamada@ea.com',
                emailStatus: 'VERIFIED',
                emailSignup: false,
                locale: 'en_us',
                storeFront: 'nzl',
                subscriber: false,
                subscriberInfo: {},
                entitlements: ['OFB-EAST:109546867', 'Origin.OFR.50.0000524', 'Origin.OFR.50.0001662','DR:225064100', 'OFB-EAST:46560', 'DR:224434500', 'OFB-EAST:42366', 'OFB-EAST:109552318'],
                extraContentEntitlements: ['OFB-EAST:109549069', 'OFB-EAST:109550764', 'Origin.OFR.50.0001379']
            },

            returnObj = {
                'result': false,
                'addedCustomDimension': false
            };

//        console.log('testing for Experiment.inExperiment() failing bucketing because not in segment');

        Experiment.init('dev', 'preview', 'nzl', 'en-us')
            .then(function(result) {
                expect(result).toBe(true);

                spyOn(Experiment, 'setUser').and.callThrough();
                Experiment.setUser(userData);
                expect(Experiment.setUser).not.toHaveBeenCalledWith(null);
                expect(Experiment.setUser).not.toHaveBeenCalledWith(undefined);

                Experiment.inExperiment('my-home-video-button', 'showButton')
                    .then(function(result) {
                        expect(result).toEqual(returnObj);
                        done();
                    });

            });

        req = jasmine.Ajax.requests.at(0);

        //segment data
        req.respondWith({
            "status": 200,
            "contentType": "application/json",
            "responseText": segmentDataStr
        });

        setTimeout(function() {
            var req;
            req = jasmine.Ajax.requests.at(1);
            //experiment data
            jasmine.Ajax.requests.at(1).respondWith({
                'status': 200,
                'contentType': 'application/json',
                'responseText': experimentDataStr
            });

        }, 1000);

    });

    it('test for Experiment.setCustomDimension() setting custom dimension', function() {
        var customDimension = 'experiment1|variantA,my-home-video-button|showButton';

//        console.log('test for Experiment.setCustomDimension() setting custom dimension');

        Experiment.setUserCustomDimension(customDimension);
        expect(Experiment.getUserCustomDimension()).toBe(customDimension);
    });


    it('test for Experiment.setCustomDimension() setting custom dimension with expired experiments', function() {
        var customDimension = 'experiment1|variantA,expiredExperiment,my-home-video-button|showButton,expiredExperiment2|expVariant,experiment1',
            activeDimension = 'experiment1|variantA,my-home-video-button|showButton,experiment1';

        Experiment.setUserCustomDimension(customDimension);
        expect(Experiment.getUserCustomDimension()).toBe(activeDimension);
    });

    it('test for Experiment.clearCustomDimension() clearing custom dimension', function() {
        var customDimension = 'experiment1|variantA,my-home-video-button|showButton';

//        console.log('test for Experiment.clearCustomDimension() clearing custom dimension');

        //ensure custom dimension is non-empty
        Experiment.setUserCustomDimension(customDimension);
        expect(Experiment.getUserCustomDimension()).toBe(customDimension);

        Experiment.clearCustomDimension();
        expect(Experiment.getUserCustomDimension()).toBe('');
    });


    it('testing for Experiment.version() to return non-null', function() {
//        console.log('testing for Experiment.version() to return non-null');
        // Since version could change, we can only test it does not crash
        expect(Experiment.version()).not.toBeNull();
        expect(Experiment.version().length > 0).toBeTruthy();
    });

/* re-enable if want to verify distribution, no need to run it as unit test each time it is built
    it('Experiment.testDistribution', function() {
        console.log('=============================');
        console.log('testing Experiment.testDistribution() 20, 20, 60');

        var distributionObj = Experiment.testDistribution(nucleusIds, '25334a0c-1370-4d70-977b-1869132c8273', 20, 20, 60);

        console.log('testing distribution of 20, 20, 60');
        console.log('distribution: expectedtotal=', distributionObj.expectedTotal, ' actualtotal=', distributionObj.actualTotal);
        for (var i = 0; i < 3; i++) {
            console.log('distribution:', i, distributionObj.bucketedPercentages[i]);
        }

        console.log('testing actualTotal = expectedTotal');
        expect(distributionObj.actualTotal).toEqual(distributionObj.expectedTotal);

        var roundedPercentage = Math.round(distributionObj.bucketedPercentages[0] * 100) / 100
        console.log('testing first bucket to be close to 20%', roundedPercentage);
        expect(roundedPercentage).toBeCloseTo(0.20, 1);

        roundedPercentage = Math.round(distributionObj.bucketedPercentages[1] * 100) / 100
        console.log('testing second bucket to be close to 20%', roundedPercentage);
        expect(roundedPercentage).toBeCloseTo(0.20, 1);

        roundedPercentage = Math.round(distributionObj.bucketedPercentages[2] * 100) / 100
        console.log('testing third bucket to be close to 60%', roundedPercentage);
        expect(roundedPercentage).toBeCloseTo(0.60, 1);
    });
*/
});