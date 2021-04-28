window.originTranslator = {
	fallbackStrings: $fallbackStrings,

	translate: function(string_id) {
        var fallbackStrings = originTranslator.fallbackStrings;
        var localizedStrings = originTranslator.localizedStrings;
        // Don't spam the log for the same string ID
        function warnOnce(message) {
            var warned = {};

            if (string_id in warned) {
                // Already complained
                return;
            }

            // Print a pre-formatted message
            console.warn("originTranslator: " + string_id + ": " + message);
            warned[string_id] = true;
        }

		if (string_id in localizedStrings) {
			return localizedStrings[string_id];
		}
		
        if (string_id in fallbackStrings) {
			return fallbackStrings[string_id];
		}

		warnOnce("String missing");
		return string_id;
	}
}

window.tr = function(string_id) {
	return originTranslator.translate(string_id)
}
