var mSimOutageRate = 0;
var mOutageErrCode = 504; // We use 504 instead of 500 or 503 as that causes Blaze server to retry. We want to avoid retry when simulating outage.

var log = require("./logutil");
var util = require("./responseutil");

var OUTAGE_HEADER = "x-sim-outage";

function setSimOutageRate(val) {
    if (val >= 0 && val <= 100) {
        mSimOutageRate = val;
        if (mSimOutageRate > 0) {
            log.LOG(" Mock service will simulate outage at " + mSimOutageRate + "% rate (unless overridden by x-sim-outage in request header).");
        }
    }
    else {
        log.LOG(" Invalid outage rate " + val + "% specified for mock service. No outage will be simulated.");
    }
}


function outageErrCode() { return mOutageErrCode; }

function shouldSimOutage(headers) {

    var outage = false;

    if (!util.isUnspecified(headers) && headers.hasOwnProperty(OUTAGE_HEADER)) {
        // The request specified an outage header. Honor that.
        if (headers[OUTAGE_HEADER] === "true") {
            outage = true;
            log.LOG(" Forcing outage response due to header");
        }
        else if (headers[OUTAGE_HEADER] === "false") {
            outage = false;
            log.LOG(" Not Forcing outage response due to header");
        }
    }
    else { // Derive the outage behavior based on the sim rate

        var rndNum = Math.floor(Math.random() * 9); // get a number from 0 to 9 so total 10 numbers
        outage = (rndNum >= (10 - ((mSimOutageRate / 100) * 10))); // probabilistic outage

        if (outage)
            log.LOG(" Forcing outage response due to simRate");
    }
    

    return outage;
}

module.exports.setSimOutageRate = setSimOutageRate;
module.exports.shouldSimOutage = shouldSimOutage;
module.exports.outageErrCode = outageErrCode;
