window.originTranslator = {
    fallbackStrings: {
    "application_name": "Origin",
    "ebisu_client_close": "Close",
    "ebisu_client_cancel": "Cancel"
    },

    translate: function(string_id) {
        var fallbackStrings = originTranslator.fallbackStrings;

        if (string_id in fallbackStrings) {
            return fallbackStrings[string_id];
        }

        return string_id;
    }
}

window.tr = function(string_id) {
    return originTranslator.translate(string_id)
}
