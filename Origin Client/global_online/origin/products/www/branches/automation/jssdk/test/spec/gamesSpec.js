describe('Origin Games API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.games.consolidatedEntitlements()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.games.consolidatedEntitlements(true).then(function(result) {

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
            expect(result.itemId).toEqual('ITM-EAST:51121');
            expect(result.financeId).toEqual('1004905');
            expect(result.offerType).toEqual('Extra Content');
            expect(result.updatedDate).toEqual('2015-03-04T20:50:53Z');

            var expectedBaseAttributes = {
                "platform": "MAC/PCWIN"
            }
            expect(result.baseAttributes).toEqual(expectedBaseAttributes);

            var expectedCustomAttributes = {
                "imageServer": "https://Eaassets-a.akamaihd.net/origin-com-store-final-assets-prod"
            }
            expect(result.customAttributes).toEqual(expectedCustomAttributes);

            var expectedMdmHierarchy = {
                "mdmHierarchy": [
                    {
                        "mdmFranchise": {
                            "franchise": "SimCity",
                            "franchiseId": 51312
                        },
                        "mdmMasterTitle": {
                            "masterTitle": "SIMCITY (2013)",
                            "masterTitleId": 182555
                        },
                        "type": "Alternate"
                    },
                    {
                        "mdmFranchise": {
                            "franchise": "SimCity",
                            "franchiseId": 51312
                        },
                        "mdmMasterTitle": {
                            "masterTitle": "SIMCITY: CITIES OF TOMORROW (EP1)",
                            "masterTitleId": 182095
                        },
                        "type": "Primary"
                    }
                ]
            }
            expect(result.mdmHierarchies).toEqual(expectedMdmHierarchy);

            var expectedPublishing = {
                "publishingAttributes": {
                    "contentId": "1004905_ep1",
                    "greyMarketControls": false,
                    "isDownloadable": false,
                    "isPublished": true,
                    "originDisplayType": "Expansion"
                },
                "softwareControlDates": {
                    "softwareControlDate": [
                        {
                            "downloadStartDate": "2013-11-12T05:00:14Z",
                            "platform": "PCWIN",
                            "releaseDate": "2013-11-12T05:00:14Z"
                        },
                        {
                            "downloadStartDate": "2013-11-12T05:00:59Z",
                            "platform": "MAC",
                            "releaseDate": "2013-11-12T05:00:59Z"
                        }
                    ]
                },
                "softwareLocales": {
                    "locale": [
                        {
                            "value": "cs_CZ"
                        },
                        {
                            "value": "da_DK"
                        },
                        {
                            "value": "de_DE"
                        },
                        {
                            "value": "en_US"
                        },
                        {
                            "value": "es_ES"
                        },
                        {
                            "value": "fi_FI"
                        },
                        {
                            "value": "fr_FR"
                        },
                        {
                            "value": "hu_HU"
                        },
                        {
                            "value": "it_IT"
                        },
                        {
                            "value": "ja_JP"
                        },
                        {
                            "value": "ko_KR"
                        },
                        {
                            "value": "nl_NL"
                        },
                        {
                            "value": "no_NO"
                        },
                        {
                            "value": "pl_PL"
                        },
                        {
                            "value": "pt_BR"
                        },
                        {
                            "value": "ru_RU"
                        },
                        {
                            "value": "sv_SE"
                        },
                        {
                            "value": "zh_TW"
                        }
                    ]
                }
            }
            expect(result.publishing).toEqual(expectedPublishing);

            var expectedLocalizable = {
                "displayName": "SimCity™: Cities of Tomorrow",
                "longDescription": "Will you create a utopian society underpinned by clean technology, or allow a giant corporation to plunder and pollute in the name of feeding your Sims’ insatiable consumerism? In addition to expanding outward, cities will have the ability to build into the sky with enormous multi-zone MegaTowers. Education and research will help you discover new technologies that make your cities less polluted, less reliant on natural resources, managed day-to-day by service drones, and as a byproduct– at risk for resource-draining giant robot attacks on your city. As the population increases, your Sims will live, work, and play closer together. Whether they do so in harmony and prosperity or as members of an exploited workforce is up to you. <br><br><b>KEY FEATURES</b><br><br>•&nbsp;<b>Build into the Sky&nbsp;–&nbsp;</b>Create towering multi-zone MegaTowers that allow your Sims to live, work, and play without ever having to touch the ground.<br><br> •&nbsp;<b>Corporate Consumerism vs. Green Utopian&nbsp;–&nbsp;</b>Unlock two new city specializations that allow you to build a resource-hungry mega corporation powered by a low-wealth workforce, or an urban utopia that develops clean technology and is controlled by the rich.<br><br> •&nbsp;<b>Transform Tomorrow&nbsp;–&nbsp;</b>Watch your city transform as it adapts to the changing times, with new options based on real-world technology such as Mag Levs that rise above the city streets and small buildings, futurized vehicles, and service drones.<br><br> •&nbsp;<b>New Disaster&nbsp;–&nbsp;</b>Brace yourself for an all-new type of disaster only fitting a technologically-advanced economy: a giant robot attack on your city! <br><br>",
                "packArtLarge": "/182095/231.0x326.0/1004905_LB_231x326_NA_^_2013-10-22-17-10-26_c443e2ff6e6d86e6544f2adb7c52b47bb17f31d4.jpg",
                "packArtMedium": "/182095/142.0x200.0/1004905_MB_142x200_NA_^_2013-10-22-17-10-49_3910f1675f3645479aafe9ea6e0908196bfe9d2c.jpg",
                "packArtSmall": "/182095/63.0x89.0/1004905_SB_63x89_NA_^_2013-10-22-17-11-11_8752c3ff7ed216df0d1590d71265df3d6ddff4c3.jpg",
                "shortDescription": "Redefine your skyline."
            }
            expect(result.localizableAttributes).toEqual(expectedLocalizable);

            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.games.catalogInfo() - SW List and Objects', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.games.catalogInfo('OFB-DA2E:24572', 'en_US', false).then(function(result) {

            expect(result.offerId).toEqual('OFB-DA2E:24572');
            expect(result.defaultLocale).toEqual('DEFAULT');

            var expectedSoftwareList = [
                {
                    "downloadURLs": {
                        "downloadURL": [
                            {
                                "buildReleaseVersion": "4",
                                "downloadURL": "/p/eagames/bioware/dragonage/da2_dlc/da2_the_black_emporium_mac_ww_v4.zip",
                                "downloadURLType": "live",
                                "effectiveDate": "2013-06-06T11:20:35Z"
                            }
                        ]
                    },
                    "fulfillmentAttributes": {
                        "addonDeploymentStrategy": "Hot Deployable",
                        "commerceProfile": "bioware",
                        "downloadPackageType": "DownloadInPlace",
                        "installCheckOverride": "[com.transgaming.dragonage2]/Contents/Resources/transgaming/c_drive/Program Files/Dragon Age 2/addins/da2_prc_one/manifest.xml",
                        "installerPath": "__Installer/DLC/The Black Emporium",
                        "metadataInstallLocation": "__Installer/DLC/The Black Emporium"
                    },
                    "softwarePlatform": "MAC"
                },
                {
                    "downloadURLs": {
                        "downloadURL": [
                            {
                                "buildReleaseVersion": "3",
                                "downloadURL": "/p/eagames/bioware/dragonage/da2_dlc/da2_the_black_emporium_pcwin_ww_v3.zip",
                                "downloadURLType": "live",
                                "effectiveDate": "2013-06-06T08:44:49Z"
                            }
                        ]
                    },
                    "fulfillmentAttributes": {
                        "addonDeploymentStrategy": "Hot Deployable",
                        "commerceProfile": "bioware",
                        "downloadPackageType": "DownloadInPlace",
                        "installCheckOverride": "[HKEY_LOCAL_MACHINE\\SOFTWARE\\BioWare\\Dragon Age II\\Install Dir]\\addins\\da2_prc_one\\manifest.xml",
                        "installerPath": "addins\\da2_prc_one",
                        "metadataInstallLocation": "addins\\da2_prc_one"
                    },
                    "softwarePlatform": "PCWIN"
                }
            ];
            expect(result.publishing.softwareList.software).toEqual(expectedSoftwareList);

            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.games.catalogInfoPrivate() - SW List and Objects', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.games.catalogInfoPrivate('OFB-DA2E:24572', 'en_US').then(function(result) {

            expect(result.offerId).toEqual('OFB-DA2E:24572');
            expect(result.defaultLocale).toEqual('DEFAULT');

            var expectedSoftwareList = [
                {
                    "downloadURLs": {
                        "downloadURL": [
                            {
                                "buildReleaseVersion": "4",
                                "downloadURL": "/p/eagames/bioware/dragonage/da2_dlc/da2_the_black_emporium_mac_ww_v4.zip",
                                "downloadURLType": "live",
                                "effectiveDate": "2013-06-06T11:20:35Z"
                            }
                        ]
                    },
                    "fulfillmentAttributes": {
                        "addonDeploymentStrategy": "Hot Deployable",
                        "commerceProfile": "bioware",
                        "downloadPackageType": "DownloadInPlace",
                        "installCheckOverride": "[com.transgaming.dragonage2]/Contents/Resources/transgaming/c_drive/Program Files/Dragon Age 2/addins/da2_prc_one/manifest.xml",
                        "installerPath": "__Installer/DLC/The Black Emporium",
                        "metadataInstallLocation": "__Installer/DLC/The Black Emporium"
                    },
                    "softwarePlatform": "MAC"
                },
                {
                    "downloadURLs": {
                        "downloadURL": [
                            {
                                "buildReleaseVersion": "3",
                                "downloadURL": "/p/eagames/bioware/dragonage/da2_dlc/da2_the_black_emporium_pcwin_ww_v3.zip",
                                "downloadURLType": "live",
                                "effectiveDate": "2013-06-06T08:44:49Z"
                            }
                        ]
                    },
                    "fulfillmentAttributes": {
                        "addonDeploymentStrategy": "Hot Deployable",
                        "commerceProfile": "bioware",
                        "downloadPackageType": "DownloadInPlace",
                        "installCheckOverride": "[HKEY_LOCAL_MACHINE\\SOFTWARE\\BioWare\\Dragon Age II\\Install Dir]\\addins\\da2_prc_one\\manifest.xml",
                        "installerPath": "addins\\da2_prc_one",
                        "metadataInstallLocation": "addins\\da2_prc_one"
                    },
                    "softwarePlatform": "PCWIN"
                }
            ];
            expect(result.publishing.softwareList.software).toEqual(expectedSoftwareList);

            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.games.getCriticalCatalogInfo()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.games.getCriticalCatalogInfo('en_TEST').then(function(result) {

            var expected = {
                "imageServer": "https://Eaassets-a.akamaihd.net/origin-com-store-final-assets-prod",
                "offers": [
                    {
                        "b": "/69882/231.0x326.0/70381_LB_231_en_GB_^_2013-01-31-08-17-53_3db847430dee24bcb347db6b5510ffc3b9ceced63858876d231807a21491b227.jpg",
                        "d": true,
                        "et": "Full Game",
                        "mt": 69882,
                        "n": "Need for Speed™ Undercover",
                        "o": "DR:105868200",
                        "r": {
                            "PCWIN": "2008-11-19T00:00:00Z"
                        },
                        "t": "Normal Game"
                    },
                    {
                        "b": "/52657/PACKART_PRESET_LARGE_JPG_231x326/71762_LB_231_^_2012-12-05-16-28-07_8bf5137a06a2ec2c9e752790453275f81c2ba045fb9cf16c42ae59b3945558dc.jpg",
                        "d": true,
                        "ec": [
                            "OFB-EAST:109546252",
                            "OFB-EAST:109546253",
                            "OFB-EAST:109546295",
                            "OFB-EAST:109546298",
                            "OFB-EAST:109546299",
                            "OFB-EAST:109546300",
                            "OFB-EAST:109546301",
                            "OFB-EAST:109546302",
                            "OFB-EAST:109546360",
                            "OFB-EAST:109546361",
                            "OFB-EAST:109546362",
                            "OFB-EAST:109546363",
                            "OFB-EAST:109546714",
                            "OFB-EAST:55002",
                            "OFB-EAST:55003",
                            "OFB-EAST:55004",
                            "OFB-EAST:59716"
                        ],
                        "et": "Full Game",
                        "mp": {
                            "PCWIN": "71762"
                        },
                        "mt": 52657,
                        "n": "Dead Space™ 3",
                        "o": "OFB-EAST:50885",
                        "r": {
                            "PCWIN": "2013-02-05T05:01:00Z"
                        },
                        "t": "Normal Game"
                    }
                ]
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

        Origin.games.getOcdByPath('en_US', 'fifa/fifa-15/standard-edition').then(function(result) {

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
        var endPoint = 'http://127.0.0.1:1402/response/reset';
        var req = new XMLHttpRequest();
        req.open('POST', endPoint, false);
        req.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded; charset=utf-8');
        req.send('');

    });

	it('Origin.games.getOfferIdByPath()', function(done) {

		var asyncTicker = new AsyncTicker();
		asyncTicker.tick(1000, done, 'Expect getOfferIdByPath() to be resolved');

		Origin.games.getOfferIdByPath('/fifa/fifa-15/standard-edition').then(function(result) {

			var expected =  {
				"country" : "US",
				"offerId" : "Origin.OFR.50.0000462",
				"storePath" : "/fifa/fifa-15/standard-edition"
			};

			expect(result).toEqual(expected);

			asyncTicker.clear(done);
		}).catch(function(error) {
			expect.toFail(error.message);
			asyncTicker.clear(done);
		});
	});

	it('Origin.games.getBaseGameOfferIdByMasterTitleId()', function(done) {

		var asyncTicker = new AsyncTicker();
		asyncTicker.tick(1000, done, 'Expect getBaseGameOfferIdByMasterTitleId() to be resolved');

		Origin.games.getBaseGameOfferIdByMasterTitleId('182555').then(function(result) {

			var expected = [
				{
					"customAttributes" : {
						"gameEditionTypeFacetKeyRankDesc" : "3000"
					},
					"offerId" : "OFB-EAST:60108"
				},
				{
					"customAttributes" : {},
					"offerId": "Origin.OFR.50.0000742"
				},
				{
					"customAttributes" : {
						"gameEditionTypeFacetKeyRankDesc" : "9000"
					},
					"offerId" : "Origin.OFR.50.0000741"
				},
				{
					"customAttributes" : {
						"gameEditionTypeFacetKeyRankDesc" : "3000"
					},
					"offerId" : "OFB-EAST:109552122"
				},
				{
					"customAttributes" : {
						"gameEditionTypeFacetKeyRankDesc" : "3000"
					},
					"offerId" : "Origin QA.OFR.50.0000114"
				}
			];

			expect(result).toEqual(expected);

			asyncTicker.clear(done);
		}).catch(function(error) {
			expect.toFail(error.message);
			asyncTicker.clear(done);
		});
	});

	it('Origin.games.directEntitle()', function(done) {

		var asyncTicker = new AsyncTicker();
		asyncTicker.tick(1000, done, 'directEntitle did not return on time');

		Origin.games.directEntitle("OFFERID").then(function(result) {

			expect(result.success).toEqual(true)
			expect(result.message).toEqual(undefined)

			asyncTicker.clear(done);
		}).catch(function(error) {
			expect.toFail(error.message);
			asyncTicker.clear(done);
		});
	});

	it('Origin.games.getOdcProfile()', function(done) {

		var asyncTicker = new AsyncTicker();
		asyncTicker.tick(1000, done, 'getOdcProfile did not return on time');

		Origin.games.getOdcProfile("odcid", "en").then(function(result) {
			expect(result.odcData).toEqual("some odc data");

			asyncTicker.clear(done);
		}).catch(function(error) {
			expect.toFail(error.message);
			asyncTicker.clear(done);
		});
	});

	it('Origin.games.getPrice()', function(done) {

		var asyncTicker = new AsyncTicker();
		asyncTicker.tick(1000, done, 'getPrice did not return on time');

		Origin.games.getPrice(["offer1", "offer2", "offer3"], "USA", "USD").then(function(result) {

            var expected = [[
            {
                "offerId":"offer1",
                "offerType":"game",
                "rating": {
                    "finalTotalPrice": 19.99,
                    "originalTotalPrice": 29.99,
                    "originalTotalUnitPrice": 29.99,
                    "promotions" : {},
                    "quanity": 1,
                    "recommendedPromotions":{},
                    "totalDiscountAmount" : 10.00,
                    "totalDiscountRate" : 33.33 }
            },
            {
                "offerId":"offer2",
                "offerType":"game",
                "rating": {
                    "finalTotalPrice": 9.99,
                    "originalTotalPrice": 9.99,
                    "originalTotalUnitPrice": 9.99,
                    "promotions" : {},
                    "quanity": 1,
                    "recommendedPromotions":{},
                    "totalDiscountAmount" : 0.00,
                    "totalDiscountRate" : 0.00 }
            },
            {
                "offerId":"offer3",
                "offerType":"game",
                "rating": {
                    "finalTotalPrice": 20.00,
                    "originalTotalPrice": 40.00,
                    "originalTotalUnitPrice": 20.00,
                    "promotions" : {},
                    "quanity": 2,
                    "recommendedPromotions":{},
                    "totalDiscountAmount" : 20.00,
                    "totalDiscountRate" : 50.00 }
            }]];

			expect(result).toEqual(expected);
			asyncTicker.clear(done);
		}).catch(function(error) {
			expect.toFail(error.message);
			asyncTicker.clear(done);
		});
	});

});
