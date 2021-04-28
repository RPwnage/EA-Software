package com.ea.originx.automation.lib.resources;

/**
 * Enum for static account tags, with corresponding string constants defined.
 *
 * @author palui
 */
public enum AccountTags {

    /**
     * Tag used to identify an account that has 1103 entitlement to bypass Concurrency Guard.
     */
    ENTITLEMENT_1103("entitlement_1103"),
    /**
     * Tag used to identify an account that has a Nucleus status of
     * 'DEACTIVATED'.
     */
    DEACTIVATED("deactivated"),
    /**
     * Tag used to identify an account that has a Nucleus status of 'DELETED'.
     */
    DELETED("deleted"),
    /**
     * Tag used to identify an account that has a Nucleus status of 'BANNED'.
     */
    BANNED("banned"),
    /**
     * Tag used to identify an account that has a strong password policy.
     */
    STRONG_PASSWORD_POLICY("strong_policy"),
    /**
     * Tag used to identify an account that has a strong password policy but has
     * a weak password which needs to be changed on login.
     */
    STRONG_PASSWORD_POLICY_WEAK_PASSWORD("strong_policy_weak_pass"),
    /**
     * Tag used to identify an account that has a 'bonehead' password. Example
     * of a bonehead password: 'password'.
     * <br><br>
     * Note: Bonehead passwords cannot be changed.
     */
    BONEHEAD_PASSWORD("bonehead_password"),
    /**
     * Tag used to identify an account that has special characters in the email
     * address.
     */
    SPECIAL_CHARS_EMAIL("specialcharsemail"),
    /**
     * Tag used to identify an account that has a credit card with editable
     * information on file.
     */
    EDITABLE_CREDIT_CARD("editable_credit_card"),
    /**
     * Tag used to identify an account that has PayPal account information
     * saved.
     */
    PAY_PAL("pay_pal"),
    /**
     * Tag used to identify an account that has purchase history in lockbox.
     */
    ACCOUNT_ACTIVITY("account_activity"),
    /**
     * Tag used to identify an account that has been granted achievements.
     */
    GRANTED_ACHIEVEMENTS("granted_achievements"),
    /**
     * Tag used to identify an account that has been granted achievements AND
     * has achievement visibility set to 'public'.
     */
    GRANTED_ACHIEVEMENTS_PUBLIC("granted_public_achievements"),
    /**
     * Tag used to identify an account that has 1600 BioWare points.
     */
    BIOWARE_POINTS("1600_bw_points"),
    /**
     * Tag used to identify an account that has an active premier subscription
     * with no entitlements.
     */
    PREMIER_SUBSCRIPTION_ACTIVE_NO_ENTITLEMENTS("premier_subscription_active_no_entitlements"),
    /**
     * Tag used to identify an account that has an active subscription with no entitlements.
     */
    SUBSCRIPTION_ACTIVE_NO_ENTITLEMENTS("subscription_active_no_entitlements"),
    /**
     * Tag used to identify an account that has an active subscription.
     */
    SUBSCRIPTION_ACTIVE("subscription_active"),
    /**
     * Tag used to identify an account that has an expired subscription.
     */
    SUBSCRIPTION_EXPIRED("subscription_expired"),
    /**
     * Tag used to identify an account that has an active subscription but also
     * has an expired credit card on file.
     */
    SUBSCRIPTION_CREDIT_EXPIRED("subscription_credit_expired"),
    /**
     * Tag used to identify an account that has an expired subscription but also
     * owns entitlements granted through the subscription.
     */
    SUBSCRIPTION_EXPIRED_OWN_ENT("subscription_expired_own_ent"),
    /**
     * Tag used to identify an account that has an expired subscription but owns
     * vault content.
     */
    SUBSCRIPTION_EXPIRED_OWN_VAULT_CONTENT("subscription_exprired_own_vault_content"),
    // Accounts not used yet and not sure why needed
    // @TODO: clarify what 'extra' means
    SUBSCRIPTION_EXPIRED_EXTRA("subscription_expired_extra"),
    /**
     * Tag used to identify an account that is used for subscription license
     * tests.
     * <br>
     * Owns 'Bookworm'
     */
    SUBSCRIPTION_LICENSE_TEST("subscription_license_test"),
    /**
     * Tag used to identify an account that is used for subscription upgrade
     * tests.
     * <br>
     * Owns 'BF4 Standard'
     */
    SUBSCRIPTION_UPGRADE_TEST("subscription_upgrade_test"),
    // Non-mature subscriber accounts
    // NOTE: These accounts will eventually mature as time goes by, and will need to be
    //       maintained manually by modifying DOB or replacing with younger accounts
    /**
     * Tag used to identify an account that is non-mature and originated in
     * Germany.
     */
    SUBSCRIPTION_GERMANY_NON_MATURE("germany_non_mature"),
    /**
     * Tag used to identify an account that is non-mature (17) and originated in
     * UK.
     */
    SUBSCRIPTION_UK_NON_MATURE_17("uk_non_mature_17"),
    /**
     * Tag used to identify an account that is non-mature (15) and originated in
     * UK.
     */
    SUBSCRIPTION_UK_NON_MATURE_15("uk_non_mature_15"),
    /**
     * Tag used to identify an account that is non-mature and originated in
     * Taiwan.
     */
    SUBSCRIPTION_TAIWAN_NON_MATURE("taiwan_non_mature"),
    /**
     * Tag used to identify an account that is non-mature and originated in
     * Australia.
     */
    SUBSCRIPTION_AUSTRALIA_NON_MATURE("australia_non_mature"),
    // Accounts (10) with no entitlements - OATESTER0000 to OATESTER0009
    /**
     * Tag used to identify an account that has no entitlements.
     */
    NO_ENTITLEMENTS("no_entitlements"),
    /**
     * Tag used to identify an account that is used for entitlements SDK tests.
     */
    ENTITLEMENTS_SDK_TEST_ACCOUNT("sdk_entitlements_test_account"),
    /**
     * Tag used to identify an account that is used for commerce SDK tests.
     */
    SDK_COMMERCE_TEST_ACCOUNT("sdk_commerce_test_account"),
    /**
     * Tag used to identify an account that has an "Origin Store QA Deluxe
     * Edition" entitlement.
     */
    STORE_DELUXE_OFFER("store_deluxe_offer"),
    /**
     * Tag used to identify an account that is used for Continuous Downloader
     * tests.
     * <br><br>
     * NOTE: Should not be used for other tests as continuous downloader is a
     * high-priority test that can run for several days.
     */
    CONTINUOUS_DOWNLOADER_ALL("continuous_downloader_all"),
    /**
     * Tag used to identify an account that has 3 entitlements.
     */
    THREE_ENTITLEMENTS("three_entitlements"),
    /**
     * Tag used to identify an account that has 5 shared entitlements.
     */
    FIVE_SHARED_ENTITLEMENTS("five_shared_entitlements"),
    /**
     * Tag used to identify an account that has 10 extra entitlements.
     */
    TEN_EXTRA_ENTITLEMENTS("ten_extra_entitlements"),
    /**
     * Tag used to identify an account that is used for 100+ friends test.
     * This account is created by using GOS friends generator tool.
     */
    MORE_THAN_100_FRIENDS("more_than_100_friends"),
    /**
     * Tag used to identify an account that is used for testing status.
     * This account is friend to the account with tag 'MORE_THAN_100_FRIENDS'.
     */
    HAVE_ONE_FRIEND("has_one_friend"),
    /**
     * Tag used to identify an account that is used for DLC downloading tests.
     */
    DLC_TEST_ACCOUNT("have_DLC"),
    /**
     * Tag used to identify an account that is entitled to a third-party game,
     * such as Witcher 3.
     */
    HAVE_THIRD_PARTY_GAME("have_third_party_game"),
    /**
     * Tag used to identify an account that has been gifted games, such as Sims
     * 4 and ME3 Deluxe.
     */
    GIFTED_ENTITLEMENTS_RECEIVED_ACCOUNT("gifted_entitlement_received_account"),
    /**
     * Tag used to identify an account that has sent game gifts to another
     * account.
     */
    GIFTED_ENTITLEMENTS_SENT_ACCOUNT("gifted_entitlement_sent_account"),
    /**
     * Tag used to identify an account that has entitlements which can be used
     * for download/play tests, such as BF4 Premium, Sims 4 DDX, SW BF, DAI,
     * FIFA17, BF 1
     */
    ENTITLEMENTS_DOWNLOAD_PLAY_TEST_ACCOUNT("entitlements_download_play_test"),
    /**
     * Tag used to identify an account that has played "Mass Effect 3". (Can be
     * used as a 'Friends Who Play' on hovercard)
     */
    FRIENDS_WHO_PLAY("friends_who_play"),
    /**
     * Tag used to identify an account that has entitlements purchased and added
     * through subscription
     *
     */
    ENTITLEMENTS_IN_LIBRARY_THROUGH_PURCHASE_AND_SUBSCRIPTION("entitlements_in_library_through_purchase_and_subscription"),
    /**
     * Tag used to identify an account that has reached the maximum friends
     * allowed limit.
     */
    MAX_FRIENDS("max_friends"),
    // Accounts (3) for twitch broadcasting (OA_TWITCH000, OA_TWITCH001, OA_TWITCH002)
    // Note: "2" because all old TWITCH-tagged accounts were rendered unusable by a global password reset
    // that Twitch had to do after a security issue
    /**
     * Tag used to identify an account that is used for Twitch broadcast
     * testing.
     */
    TWITCH_2("twitch_2"),
    /**
     * Tag used to identify an account that does not have any friends.
     */
    PERFORMANCE_NO_FRIENDS("account_with_no_friends"),
    /**
     * Tag used to identify an account that has 20 friends.
     */
    PERFORMANCE_20_FRIENDS("account_with_20_friends"),
    /**
     * Tag used to identify an account that has 30 friends.
     */
    PERFORMANCE_30_FRIENDS("account_with_30_friends"),
    /**
     * Tag used to identify an account that does not have any entitlements and
     * also does not have any friends.
     */
    PERFORMANCE_NO_ENTITLEMENTS_NO_FRIENDS("performance_no_entitlement_no_friends"),
    /**
     * Tag used to identify an account that has 5 entitlements and 30 friends.
     */
    PERFORMANCE_5_ENTITLEMENT_30_FRIENDS("performance_5_entitlement_30_friends"),
    /**
     * Tag used to identify an account that has 6 entitlements and 40 friends.
     */
    PERFORMANCE_6_ENTITLEMENT_40_FRIENDS("performance_6_entitlement_40_friends"),
    X_PERFORMANCE("x_performance"),
    /**
     * Tag used to identify an account that is used for Mac testing.
     */
    MAC("mac"),
    /**
     * Tag used to identify an account that is used for Mac alpha testing.
     * <br><br>
     * NOTE: Has Dragon Age Origins (OfferID OFB-EAST:51787)
     */
    MAC_ALPHA("mac_alpha"),
    //Special account for gifting to have a unique first name ??
    GIFTING_KNOWN_NAME("gifting_known_name"),
    /**
     * Tag used to identify an account that has only unopened gifts.
     */
    ONLY_UNOPENED_GIFTS("only_unopened_gifts"),
    /**
     * Tag used to identify an account that has only opened gifts.
     */
    ONLY_OPENED_GIFTS("only_opened_gifts"),
    /**
     * Tag used to identify an account that has all 3 parts of a 3-part bundle.
     */
    THREE_OF_THREE_BUNDLE("three_of_three_bundle"),
    /**
     * Tag used to identify an account that has 2 parts of a 3-part bundle.
     */
    TWO_OF_THREE_BUNDLE("two_of_three_bundle"),
    /**
     * Tag used to identify an account that has 1 part of a 3-part bundle.
     */
    ONE_OF_THREE_BUNDLE("one_of_three_bundle"),
    /**
     * Tag used to identify an account that owns a bundle
     */
    ACCOUNT_WITH_BUNDLE("account_with_bundle"),
    /**
     * Tag used to identify an account that 'Battlefield 4' on its wishlist.
     */
    BF4_ON_WISHLIST("bf4_on_wishlist"),
    /**
     * Tag used to identify a subscriber account that has 'Battlefield 4
     * Standard' in game library through subscription
     */
    BF4_IN_LIBRARY_THROUGH_SUBSCRIPTION("bf4_in_library_through_subscription"),
    /**
     * Tag used to identify a subscriber account that has 'Fifa 18 Standard' in
     * game library through purchase
     */
    FIFA18_IN_LIBRARY_THROUGH_PURCHASE_SUBSCRIBER("fifa18_in_library_through_purchase_subscriber"),
    /**
     * Tag used to identify a subscriber account that has 'Battlefield 4
     * Standard' in game library through purchase
     */
    BF4_IN_LIBRARY_THROUGH_PURCHASE_SUBSCRIBER("bf4_in_library_through_purchase_subscriber"),
    /**
     * Tag used to identify a non-subscriber account that has 'Battlefield 4
     * Standard' in game library through purchase
     */
    BF4_IN_LIBRARY_THROUGH_PURCHASE_NON_SUBSCRIBER("bf4_in_library_through_purchase_non_subscriber"),
    /**
     * Tag used to identify an account that is entitled to most, if not all,
     * games and DLC.
     */
    PERSONALIZATION_FULL_ENTITLEMENTS("personalization_full_entitlements"),
    /**
     * Tag used to identify an account that is entitled to special games and DLC
     * that cannot be handled by PERSONALIZATION_FULL_ENTITLEMENTS due to
     * conflicts.
     */
    PERSONALIZATION_SPECIAL_ENTITLEMENTS("personalization_special_entitlements"),
    /**
     * Tag used to identify an account that is an active subscriber and is used
     * for PDP tests.
     */
    TEST_PDP_SUBSCRIBER("test_pdp_subscriber"),
    /**
     * Tag used to identify an account that is NOT an active subscriber and is
     * used for PDP tests.
     */
    TEST_PDP_NON_SUBSCRIBER("test_pdp_non_subscriber"),
    /**
     * Tag used to identify an account that has owns the Beta and Release version
     * of a game
     */
    BETA_RELEASE_ENTITLEMENT("beta_release_entitlement");



    private final String tagString;

    /**
     * Constructor
     *
     * @param tagString String constant of tag
     */
    private AccountTags(final String tagString) {
        this.tagString = tagString;
    }

    /**
     * Get the string value of the tag.
     *
     * @return string value of the tag
     */
    @Override
    public String toString() {
        return tagString;
    }
}