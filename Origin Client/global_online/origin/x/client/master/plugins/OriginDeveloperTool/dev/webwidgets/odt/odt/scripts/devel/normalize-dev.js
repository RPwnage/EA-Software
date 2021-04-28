;(function($, undefined){
"use strict";

if (window.originDeveloper) { return; }

/**
 * CLASS: ClientNavigator
 * Provides high-level methods in-client navigation
 */
var ClientNavigator = function()
{
};

// Shows the passed URL in the user's default browser. This URL must be globally accessible; widget:// URLs will not be reachable to an external browser
ClientNavigator.prototype.launchExternalBrowser = function(url)
{
    window.open(url, 'external-browser');
};

// Create the Singleton, and expose to the world
window.clientNavigation = new ClientNavigator();

/**
 * CLASS: ClientSettings
 * Provides high-level methods in-client settings
 */
var ClientSettings = function()
{
};

ClientSettings.prototype.readSetting = function(settingName)
{
    if(settingName === 'EnvironmentName') {
        return 'production';
    }
    else {
        return '';
    }
};

// Create the Singleton, and expose to the world
window.clientSettings = new ClientSettings();

/**
 * CLASS: Connector
 */
var Connector = function()
{
    this.connections = [];    
};
Connector.prototype.connect = function(callback)
{
    this.connections.push(callback);
};
Connector.prototype.disconnect = function(callback)
{
    this.connections = this.connections.filter(function(other)
    {
        return callback !== other;
    });
};
Connector.prototype.trigger = function()
{
    var args = Array.prototype.slice.call(arguments);
    this.connections.forEach(function(callback)
    {
        callback.apply(null, args);
    });
};

/**
 * CLASS: ProductQuery
 */
var ProductQuery = function()
{
    this.startQuery = function(productIds)
    {
        var ProductQueryOperation;
        ProductQueryOperation = function()
        {
            this.succeeded = new Connector;
            this.failed = new Connector;
        };
        
        var queryOp = new ProductQueryOperation;
        window.setTimeout(function()
        {                
            queryOp.succeeded.trigger(window.entitlementManager._unownedOffers);
        }, 1000);
        
        return queryOp;
    };
}

window.productQuery = new ProductQuery;


/**
 * CLASS: GeolocationQuery
 */
var GeolocationQuery = function()
{
    this.startQuery = function(ipAddress)
    {
        var GeolocationQueryOperation;
        GeolocationQueryOperation = function()
        {
            this.succeeded = new Connector;
            this.failed = new Connector;
        };
        
        var queryOp = new GeolocationQueryOperation;
        window.setTimeout(function()
        {                
            queryOp.succeeded.trigger({
                geoCountry: 'FR',
                commerceCountry: 'FR',
                commerceCurrency: 'EUR'
            });
        }, 300);
        
        return queryOp;
    };
}

window.geolocationQuery = new GeolocationQuery;

/**
 * CLASS: EntitlementManager
 * Root object for accessing the current user's entitlement information
 */
var EntitlementManager = function()
{    
    this._entitlements = [
        {
            productId: "dragonage_na",
            contentId: "dragonage_na",
            masterTitleId: "0001",
            franchiseId: "2001",
            expansionId: "10001",
            localesSupported: ["en_US", "en_DE"],
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70377_262x372.jpg"],
            title: "Dragon Age(TM): Origins",
            owned: true,
            platformsSupported: ["PC","MAC"]
        },
        {
            productId: "darkspore_dd",
            contentId: "darkspore_dd",
            masterTitleId: "0001",
            franchiseId: "2001",
            expansionId: "10001",
            localesSupported: ["en_US"],
            boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70561_262x372.jpg"],
            title: "DARKSPORE(TM)",
            owned: true,
            platformsSupported: ["PC"]
        },
        {
            productId: "71067",
            contentId: "71067",
            masterTitleId: "0001",
            franchiseId: "2001",
            expansionId: "10001",
            localesSupported: ["en_US", "en_DE"],
            boxartUrls: ["http://static.cdn.ea.com/ebisu/u/f/products/70619/images/en_US/boxart_262x372.jpg"],
            title: "Battlefield 3 Limited Edition",
            platformsSupported: ["PC"],
            owned: true,
            expansions:
            [
                {
                    productId: "dragonage_na_expand",
                    contentId: "dragonage_na_expand",
                    masterTitleId: "0001",
                    franchiseId: "2001",
                    expansionId: "10001",
                    localesSupported: ["en_US", "en_DE"],
                    boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70377_262x372.jpg"],
                    title: "Dragon Age(TM): Origins",
                    owned: true,
                    platformsSupported: ["PC"]
                },
                {
                    productId: "darkspore_dd_expand",
                    contentId: "darkspore_dd_expand",
                    masterTitleId: "0001",
                    franchiseId: "2001",
                    expansionId: "10001",
                    localesSupported: ["en_US", "en_DE"],
                    boxartUrls: ["http://drh2.img.digitalriver.com/DRHM/Storefront/Company/ea/images/product/box/70561_262x372.jpg"],
                    title: "DARKSPORE(TM)",
                    owned: false,
                    platformsSupported: ["PC"]
                }
            ],
            addons:
            [
                {
                    productId: "43",
                    contentId: "43",
                    masterTitleId: "0001",
                    franchiseId: "2001",
                    expansionId: "10001",
                    localesSupported: ["en_US", "en_DE"],
                    boxartUrls: [],
                    title: "Game Soundtrack A Very Very Very Long Title",
                    owned: true,
                    platformsSupported: ["PC"]
                },
                {
                    productId: "45",
                    contentId: "45",
                    masterTitleId: "0001",
                    franchiseId: "2001",
                    expansionId: "10001",
                    localesSupported: ["en_US", "en_DE"],
                    boxartUrls: [],
                    title: "Micro Content",
                    owned: true,
                    platformsSupported: ["PC"]
                },
                {
                    productId: "61",
                    contentId: "61",
                    masterTitleId: "0001",
                    franchiseId: "2001",
                    expansionId: "10001",
                    localesSupported: ["en_US", "en_DE"],
                    boxartUrls: [],
                    title: "Unlockable Content",
                    owned: true,
                    platformsSupported: ["PC"]
                }
            ]
        }
    ];
    
    this._unownedOffers = [
    {
        addons: [],
        expansions: [],
        productId: "unowned_content",
        contentId: "unowned_content",
        masterTitleId: "0001",
        franchiseId: "2001",
        expansionId: "10001",
        localesSupported: ["en_US", "es_MX"],
        boxartUrls: [],
        title: "Some Unowned Title",
        platformsSupported: []
    },
    {
        addons: [],
        expansions: [],
        productId: "unowned_content2",
        contentId: "unowned_content2",
        masterTitleId: "00012",
        franchiseId: "2002",
        expansionId: "100012",
        localesSupported: ["en_US", "es_ES"],
        boxartUrls: [],
        title: "Some Unowned Title 2",
        platformsSupported: []
    }]
};

// Returns a list of entitlement objects belonging to the current user
EntitlementManager.prototype.__defineGetter__('topLevelEntitlements', function()
{
    return this._entitlements;
});

// Gets an entitlement based on its ID. Returns null if no entitlement is found
EntitlementManager.prototype.getEntitlementById = function(id)
{
    for(var i = 0; i < this._entitlements.length; i++)
    {
        if(this._entitlements[i].productId === id || this._entitlements[i].contentId === id)
        {
            return this._entitlements[i];
        }
    }
    for(var i = 0; i < this._unownedOffers.length; i++)
    {
        if(this._unownedOffers[i].productId === id || this._unownedOffers[i].contentId === id)
        {
            return this._unownedOffers[i];
        }
    }
    return this._entitlements[0];
};

// Create the Singleton, and expose to the world
window.entitlementManager = new EntitlementManager();

/**
 * CLASS: OriginDeveloper
 */
var OriginDeveloper = function()
{
    this.userId = "1000010000";
    this.fullSoftwareBuildMapAvailable = new Connector;
};

OriginDeveloper.prototype.reloadConfig = function()
{
    return true;
};

OriginDeveloper.prototype.applyChanges = function()
{
    var restartRequired = false;
    console.log('Applying changes, restartRequired = ' + restartRequired);
    return restartRequired;
};

OriginDeveloper.prototype.fetchFullSoftwareBuildMap = function(id)
{
    window.setTimeout(function()
    {                
        window.originDeveloper.fullSoftwareBuildMapAvailable.trigger(
        [
            id, 
            window.originDeveloper.softwareBuildMap(id)
        ]);
    }, 1000);
};

OriginDeveloper.prototype.softwareBuildMap = function(id)
{
	return [
		{
			"buildId" : "2012-01-01T01:00:00.000Z",
			"buildLiveDate" : "2012-01-01T01:00:00.000Z",
			"buildNotes" : "",
			"fileName" : "bf3_patch8_ww_20130227.zip",
			"urlType" : "",
            "buildReleaseVersion" : "1"
		},
		{
			"buildId" : "2013-01-01T01:10:00.000Z",
			"buildLiveDate" : "2013-01-01T01:10:00.000Z",
			"buildNotes" : "",
			"fileName" : "bf3_patch8_ww_20130227_2.zip",
			"urlType" : "Live",
            "buildReleaseVersion" : "2"
		},
		{
			"buildId" : "2020-02-02T02:20:00.000Z",
			"buildLiveDate" : "2020-02-02T02:20:00.000Z",
			"buildNotes" : "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
			"fileName" : "bf3_patch8_ww_20130227_2_production_really_long_name_etc_etc_extremely_long_hello_world.zip",
			"urlType" : "Staged",
            "buildReleaseVersion" : "2"
		}
	];
}


OriginDeveloper.prototype.cloudConfiguration = function(id)
{
    return ['Include: %Documents%/Battlefield 3/settings/','Include: *.xml','-----------------------------------------------------------------------'];
}

OriginDeveloper.prototype.clearRemoteArea = function(id)
{
    console.log('Clearing remote area');
}

OriginDeveloper.prototype.removeContent = function(id)
{
    console.log('Removing content ' + id);
}

// Shows the passed URL in the user's default browser. This URL must be globally accessible; widget:// URLs will not be reachable to an external browser
OriginDeveloper.prototype.launchExternalBrowser = function(url)
{
    window.open(url, 'external-browser');
};

OriginDeveloper.prototype.__defineGetter__('unownedIds', function()
{
    return ['unowned_content', 'unowned_content2'];
});

OriginDeveloper.prototype.deactivate = function()
{
    console.log('Deactivating ODT!');
}

OriginDeveloper.prototype.storeEnvironment = function()
{
    return 'approve-prod';
}

window.originDeveloper = new OriginDeveloper();

/**
 * CLASS: OriginCommon
 */
var OriginCommon = function()
{
};

OriginCommon.prototype.reloadConfig = function()
{
    return true;
}

OriginCommon.prototype.readOverride = function(section, override)
{
    if(section === '[Connection]' && override === 'EnvironmentName') {
        return 'production';
    }
    else {
        return '';
    }
}

OriginCommon.prototype.readOverride = function(section, override, id)
{
    if(section === '[Connection]' && override === 'OverrideInstallFilename' && id === 'darkspore_dd') {
        return 'INSTALL_FILENAME';
    }
    else if(section === '[Connection]' && override === 'OverrideDownloadPath' && id === 'dragonage_na') {
        return 'DOWNLOAD_PATH';
    }
    else if(section === '[Connection]' && override === 'OverrideSharedNetworkFolder' && id === '71067') {
        return 'C:\\builds';
    }
    else if(section === '[Connection]' && override === 'OverrideUpdateCheck' && id === '71067') {
        return 'scheduled';
    }
    else {
        return '';
    }
}

OriginCommon.prototype.writeOverride = function(section, override, id, value)
{
    // This function is overloaded in the C++ proxy.  One function accepts 3 parameters (section, override, value)
    // and the other accepts 4 (section, override, id, value).  Handle that here.
    if(typeof value !== 'undefined')
    {
        console.log('Writing product override ' + section + ' ' + override + '=' + value + 'for ID ' + id);
    }
    else
    {
        value = id;
        console.log('Writing product override ' + section + ' ' + override + '=' + value);
    }
}

window.originCommon = new OriginCommon();

})(jQuery);
