describe('Origin Feeds API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it("Origin.feeds.retrieveStoryData() merchandise story type", function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.feeds.retrieveStoryData('merchandise', 0, 'undefined', 'en_US').then(function(result) {

            expected = [{
                "alignment": "left",
                "backgroundColor": "",
                "friendlyId": "games-30--off--opoi-48445-",
                "headline": "Get over 30 games and DLC for just $9.99 / month",
                "image": "http://qa.review.assets.cms.origin.com/content/dam/originx/web/app/home/games_1200x168_en_US_buynow.jpg",
                "legalPageURL": "https://review.www.cms.origin.com/en-us/promo/bf4-premium-30--off--opoi-48445-",
                "legalTitle": "Terms and Conditions",
                "link": "javascript:window.promoManager.viewUrlOnStoreTab(\"https://review.www.cms.origin.com/en-us/store/cart/add/eyJPRkItRUFTVDoxMDk1…jIiOjF9?intcmp=opm_TO_BF4Premium_login&promo-code=GQHR-K2N3-GHVY-AFM4-XZGL\")",
                "targetText": "Save Now",
                "text": "Get the best games on Origin, new releases, all DLC, and more.",
                "theme": "dark",
                "thumbnail": "http://qa.review.assets.cms.origin.com/content/dam/originx/web/app/home/games_1200x168_en_US_buynow.jpg"
            },
            {
                "alignment": "right",
                "backgroundColor": "#ffffff",
                "friendlyId": "monetization-message-30off-recon",
                "headline": "Gear Up For Less",
                "image": "https://review.assets.cms.origin.com/content/dam/eadam/B/BATTLEFIELD_4/PromoManager/1016136_opm_690x380_en_US_buynow.jpg",
                "legalPageURL": "",
                "legalTitle": "",
                "link": "javascript:window.promoManager.viewUrlOnStoreTab(\"https://review.www.cms.origin.com/en-us/store/deals/pages/battlefieldshortcuts?intcmp=opm_merch_ReconShortcutKit_login\")",
                "targetText": "Save Now",
                "text": "The Recon Shortcut Kit is now 30% off. In fact, all Battlefield Shortcuts and Battlepacks are on sale!",
                "theme": "light",
                "thumbnail": "https://review.assets.cms.origin.com/content/dam/eadam/B/BATTLEFIELD_4/PromoManager/1016136_opm_47x47_en_US_buynow.jpg"
            },
            {
                "alignment": "right",
                "backgroundColor": "",
                "friendlyId": "pvz-ingame-updates",
                "headline": "Meet Your New Army.",
                "image": "https://review.assets.cms.origin.com/content/dam/eadam/Promo-Manager/1014576_opm_690x380_updates.jpg",
                "legalPageURL": "",
                "legalTitle": "",
                "link": "javascript:window.promoManager.viewUrlOnStoreTab(\"https://review.www.cms.origin.com/en-us/store/buy/plants-vs-zombies-garden-â€¦are/pc-download/base-game/standard-edition?intcmp=opm_merch_PvZGWDLC_Login\")",
                "targetText": "Get It Now",
                "text": "Garden Warfare has it all, and it keeps getting better. Brand new game modes, powerful character variants and more. All for free!",
                "theme": "dark",
                "thumbnail": "https://review.assets.cms.origin.com/content/dam/eadam/Promo-Manager/1014576_opm_47x47_updates.jpg"
            },
            {
                "alignment": "right",
                "backgroundColor": "",
                "friendlyId": "titanfal-gametime",
                "headline": "Play for free! All Weekend Long.",
                "image": "https://review.assets.cms.origin.com/content/dam/eadam/Promo-Manager/opm_gamtime_titanfall_en-us_690x380.jpg",
                "legalPageURL": "",
                "legalTitle": "",
                "link": "javascript:window.promoManager.viewUrlOnStoreTab(\"https://www.origin.com/store/free-games/game-time?easid=opm_merch-GT_Titanfall_login\")",
                "targetText": "Start Game Time",
                "text": "Game Time is on. Download Titanfall and join your friends. This isn\"t a demo. This is 48 hours with the full game.",
                "theme": "dark",
                "thumbnail": "https://review.assets.cms.origin.com/content/dam/eadam/Promo-Manager/opm_gamtime_titanfall_en-us_47x47.jpg"
            }];

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it("Origin.feeds.retrieveStoryData() newdlc story type", function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.feeds.retrieveStoryData('newdlc', 0, 'undefined', 'en_US').then(function(result) {

            expected = [{
                "offerId": "OFB-EAST:109550761"
            },
            {
                "offerId": "OFB-EAST:109550762"
            },
            {
                "offerId": "OFB-EAST:109550763"
            },
            {
                "offerId": "OFB-EAST:109550764"
            }];

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it("Origin.feeds.retrieveStoryData() recfriends story type", function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.feeds.retrieveStoryData('recfriends', 0, 'undefined', 'en_US').then(function(result) {

            expected = [{
                "userId": "12295990004"
            },
            {
                "userId": "1000047150868"
            },
            {
                "userId": "1000141721863"
            },
            {
                "userId": "12296602350"
            }];

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it("Origin.feeds.retrieveStoryData() mostpopulargame story type", function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.feeds.retrieveStoryData('mostpopulargame', 0, 'undefined', 'en_US').then(function(result) {

            expected = [{
                "imageUrl": "https://eaassets-a.akamaihd.net/origin-com-store-final-assets-prod/76889/231.0x326.0/1007077_LB_231x326_en_US_%5E_2014-08-26-14-16-37_dc204da02063323b31b517ee74425c49b2e2a4ae.png",
                "offerId": "OFB-EAST:109552316"
            }];

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

});
