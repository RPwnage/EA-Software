window.originTranslator = {
    fallbackStrings: {
        "ebisu_client_explain_your_rating": "We'd love to know your reasons for the above score (optional).",
        "ebisu_client_origin_one_survey_text": "Your feedback is valuable. Please take a moment to answer the questions below. Your responses will be kept confidential.",
        "ebisu_client_allow_contact_due_to_feedback": "It’s okay to contact me about this feedback.",
        "ebisu_client_submit": "Submit",
        "ebisu_client_cancel": "Cancel",
        "ebisu_client_extremely_likely" : "Extremely likely",
        "ebisu_client_not_at_all" : "Very Unlikely",
        "ebisu_client_game_nps_title_caps" : "%1 TEAM WANTS YOUR FEEDBACK",
        "ebisu_client_game_nps_how_likely_are_you_to_recommend_game" : "How likely are you to recommend %1 to a friend?"
    },

    translate: function (string_id) {
        var fallbackStrings = originTranslator.fallbackStrings;

        if (string_id in fallbackStrings) {
            return fallbackStrings[string_id];
        }

        return string_id;
    }
}

window.tr = function (string_id) {
    return originTranslator.translate(string_id);
}
