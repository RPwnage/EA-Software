;(function($, undefined){
"use strict";

if (window.originCommon) { return; }
if (window.shiftQuery) { return; }

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
 * CLASS: DcmtDownloadManagerProxy
 */
var DcmtDownloadManagerProxy = function()
{
    this.startDownload = function(buildId, buildVersion)
    {
        console.info("startDownload: " + buildId + ", " + buildVersion);
    };
}

window.dcmtDownloadManager = new DcmtDownloadManagerProxy;

/**
 * CLASS: ShiftQueryProxy
 */
var ShiftQueryProxy = function()
{
    var ShiftQueryOperation;
    ShiftQueryOperation = function()
    {
        this.always = new Connector;
        this.done = new Connector;
        this.fail = new Connector;
    };
    
    this.deliveryStatusUpdated = new Connector;
    
    this._builds = [
        {
            buildId: "4ac6c4-3b8985-432754",
            version: "Concept 1",
            milestone: "Concept",
            buildTypes: ["Debug", "Final", "Juice"],
            lifeCycleStatus: "",
            status: "COMPLETED",
            approvalStates: ""
        },
        {
            buildId: "8b4c11-9325bc-abcdef",
            version: "Beta 2",
            milestone: "Beta",
            buildTypes: ["Localization", "Marketing"],
            lifeCycleStatus: "",
            status: "AVAILABLE",
            approvalStates: ""
        },
        {
            buildId: "8b4c11-9325bc-abcde1",
            version: "Beta 3",
            milestone: "Beta",
            buildTypes: ["Online"],
            lifeCycleStatus: "",
            status: "AVAILABLE",
            approvalStates: ""
        },
        {
            buildId: "8b4c11-9325bc-abcde2",
            version: "Beta 4",
            milestone: "Beta",
            buildTypes: ["Online", "Profile", "QA"],
            lifeCycleStatus: "",
            status: "AVAILABLE",
            approvalStates: ""
        },
        {
            buildId: "8b4c11-9325bc-abcde3",
            version: "Beta 5",
            milestone: "Beta",
            buildTypes: ["Release"],
            lifeCycleStatus: "",
            status: "AVAILABLE",
            approvalStates: ""
        },
        {
            buildId: "8b4c11-9325bc-abcde4",
            version: "Beta 6",
            milestone: "Beta",
            buildTypes: ["Ship", "Available"],
            lifeCycleStatus: "",
            status: "AVAILABLE",
            approvalStates: ""
        },
        {
            buildId: "8b4c11-9325bc-abcde5",
            version: "Beta 7",
            milestone: "Beta",
            buildTypes: ["QA"],
            lifeCycleStatus: "",
            status: "AVAILABLE",
            approvalStates: ""
        },
        {
            buildId: "8b4c11-9325bc-abcde6",
            version: "Beta 8",
            milestone: "Beta",
            buildTypes: ["QA"],
            lifeCycleStatus: "",
            status: "AVAILABLE",
            approvalStates: ""
        },
        {
            buildId: "8b4c11-9325bc-abcde7",
            version: "Beta 9",
            milestone: "Beta",
            buildTypes: ["QA"],
            lifeCycleStatus: "",
            status: "AVAILABLE",
            approvalStates: ""
        },
        {
            buildId: "8b4c11-9325bc-abcde8",
            version: "Beta 10",
            milestone: "Beta",
            buildTypes: ["QA"],
            lifeCycleStatus: "",
            status: "AVAILABLE",
            approvalStates: ""
        },
        {
            buildId: "8b4c11-9325bc-abcde9",
            version: "Beta 11",
            milestone: "Beta",
            buildTypes: ["QA"],
            lifeCycleStatus: "",
            status: "AVAILABLE",
            approvalStates: ""
        },
        {
            buildId: "8b4c11-9325bc-abcd10",
            version: "Beta 12",
            milestone: "Beta",
            buildTypes: ["QA"],
            lifeCycleStatus: "",
            status: "AVAILABLE",
            approvalStates: ""
        },
        {
            buildId: "8b4c11-9325bc-abcd11",
            version: "Beta 13",
            milestone: "Beta",
            buildTypes: ["Debug", "Final", "Juice", "Localization", "Marketing", "Online", "Profile", "QA", "Release", "Ship", "Available"],
            lifeCycleStatus: "",
            status: "AVAILABLE",
            approvalStates: ""
        }];
        
    this.authenticate = function(name, pass)
    {
        console.info("authenticate: " + name + ", " + pass);
        var queryOp = new ShiftQueryOperation;
        window.setTimeout(function()
        {
            queryOp.done.trigger();
            queryOp.always.trigger();
        }, 1000);
        return queryOp;
    };
    
    this.getAvailableBuilds = function(offerId)
    {
        console.info("getAvailableBuilds: " + offerId);
        var queryOp = new ShiftQueryOperation;
        window.setTimeout(function()
        {
            queryOp.done.trigger(window.shiftQuery._builds);
            queryOp.always.trigger();
        }, 1000);
        return queryOp;
    };
    
    this.placeDeliveryRequest = function(offerId)
    {
        console.info("placeDeliveryRequest: " + offerId);
        var queryOp = new ShiftQueryOperation;
        window.setTimeout(function()
        {
            queryOp.done.trigger();
            queryOp.always.trigger();
        }, 1000);
        return queryOp;
    };
    
    this.getDeliveryStatus = function(maxResults)
    {
        console.info("getDeliveryStatus: " + maxResults);
        var queryOp = new ShiftQueryOperation;
        window.setTimeout(function()
        {
            queryOp.done.trigger();
            queryOp.always.trigger();
        }, 1000);
        return queryOp;
    };
    
    this.generateDownloadUrl = function(buildId)
    {
        console.info("generateDownloadUrl: " + buildId);
        var queryOp = new ShiftQueryOperation;
        window.setTimeout(function()
        {
            queryOp.done.trigger();
            queryOp.always.trigger();
        }, 1000);
        return queryOp;
    };
}

window.shiftQuery = new ShiftQueryProxy;

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
};

// Returns a list of entitlement objects belonging to the current user
EntitlementManager.prototype.__defineGetter__('topLevelEntitlements', function()
{
    return this._entitlements;
});

// Gets an entitlement based on its ID. Returns null if no entitlement is found
EntitlementManager.prototype.getEntitlementById = function(id)
{
    return this._entitlements[0];
};

// Create the Singleton, and expose to the world
window.entitlementManager = new EntitlementManager();

/**
 * CLASS: OriginDeveloper
 */
var OriginDeveloper = function()
{
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

OriginDeveloper.prototype.readOverride = function(section, override)
{
    if(section === '[Connection]' && override === 'EnvironmentName') {
        return 'production';
    }
    else {
        return '';
    }
}

OriginDeveloper.prototype.readOverride = function(section, override, id)
{
    if(section === '[Connection]' && override === 'OverrideInstallFilename') {
        return 'INSTALL_FILENAME';
    }
    else if(section === '[Connection]' && override === 'OverrideDownloadPath' && id === 'OFB-EAST:41331') {
        return 'DOWNLOAD_PATH';
    }
    else {
        return '';
    }
}

OriginDeveloper.prototype.writeOverride = function(section, override, value)
{
    console.log('Writing general override ' + section + ' ' + override + '=' + value);
}

OriginDeveloper.prototype.writeOverride = function(section, override, id, value)
{
    console.log('Writing product override ' + section + ' ' + override + '=' + value + 'for ID ' + id);
}

window.originCommon = new OriginDeveloper();

})(jQuery);
