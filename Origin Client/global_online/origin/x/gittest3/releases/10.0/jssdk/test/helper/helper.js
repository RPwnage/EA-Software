var AsyncTicker = function() {
    var timeoutID;
    return {
        tick : function(timeout, done, timeoutMsg) {
            timeoutID = window.setTimeout(function() {
                expect().toFail(timeoutMsg);
                window.clearTimeout(timeoutID);
                done();
            }, timeout);
        },
        clear : function(done) {
            window.clearTimeout(timeoutID);
            done();
        }
    };
};

var matcher = {
    toFail : function(util, customEqualityTesters) {
        return {
            compare : function(expected, actual) {
                return {
                    message : actual
                };
            }
        };
    },
    toPass : function(util, customEqualityTesters) {
        return {
            compare : function(expected, actual) {
                return {
                    pass : true,
                    message : actual
                };
            }
        };
    }
};
