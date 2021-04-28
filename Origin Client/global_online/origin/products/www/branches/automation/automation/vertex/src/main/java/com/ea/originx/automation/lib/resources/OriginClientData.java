package com.ea.originx.automation.lib.resources;

/**
 * An interface to contain all the Origin Client Data constants
 *
 * @author rmcneil, micwang
 */
public final class OriginClientData {

    // paths
    public static final String PROGRAM_DATA_NONXP_PATH = "C:\\ProgramData\\Origin\\";

    // fake credit card information for purchases on non-production environments
    public static final String ACCOUNT_PRIVACY_SECURITY_ANSWER = "Kedi";
    public static final String CREDIT_CARD_NUMBER = "4024007141018613";
    public static final String ALTERNATE_CREDIT_CARD_NUMBER = "5400540054005400";
    public static final String INVALID_CREDIT_CARD_NUMBER = "0000000000000000";
    public static final String CREDIT_CARD_EXPIRATION_YEAR = "2020";
    public static final String CREDIT_CARD_EXPIRATION_MONTH = "06";
    public static final String CSV_CODE = "000";
    public static final String STREET = "EA Canada";
    public static final String CITY = "Burnaby";
    public static final String POSTAL_CODE = "V5G 4X1";
    public static final String COUNTRY = "Canada";
    public static final String ZIP_CODE = "98111";

    // fake credit card information used for triggering threeDS work flow
    public static final String THREEDS_CREDIT_CARD_NUMBER = "4000000000000002";
    public static final String THREEDS_CREDIT_CARD_EXPIRATION_YEAR = "2020";
    public static final String THREEDS_CREDIT_CARD_EXPIRATION_MONTH = "01";
    public static final String THREEDS_COUNTRY = "United States";
    public static final String THREEDS_POSTAL_CODE = "94112";

    //PDP Hero Different Buttons Offerids
    public static final String TRY_DEMO_BUTTON_OFFER_ID = "OFB-EAST:109551057";
    public static final String GET_TRIAL_BUTTON_OFFER_ID = "OFB-EAST:109552153";
    public static final String DOWNLOAD_TRIAL_BUTTON_OFFER_ID = "Origin.OFR.50.0001531";
    public static final String PLAY_THE_TRIAL_BUTTON_OFFER_ID = "Origin.OFR.50.0001475";

    // Colors for text across Origin
    public static final String ORIGIN_ACCESS_DISCOUNT_COLOUR = "#04bd68";
    public static final String ORIGIN_CLICK_FRIEND_HIGHLIGHT_COLOUR = "#c3c6ce";
    public static final String ORIGIN_URL_MESSAGE_COLOUR = "#2ac4f5";
    public static final String PRIMARY_CTA_BUTTON_COLOUR = "#f56c2d"; // orange - Buy Button/Direct Aquisition/Play Now CTA
    public static final String ORIGIN_ACCESS_COMPARISON_TABLE_BACKGROUND = "#141b20"; // light black
    public static final String BUY_BUTTON_TRANSPARENT_COLOUR = "#000000"; // black
    public static final String ORIGINAL_PRICE_STRIKETHROUGH_COLOUR = "#787d85"; // dark gray
    public static final String PROGRESS_BAR_DOWNLOADING_COLOUR = "#04bd68"; // green
    public static final String PLAYABLE_INDICATOR_ACTIVE_COLOUR = "#04bd68"; // green
    public static final String WISHLIST_HEART_ICON_COLOUR = "#df3d00"; // red
    public static final String CHECK_MARK_ICON_COLOUR = "#05a68a"; //dark green
    public static final String OGD_DOWNLOAD_ICON_COLOR = "#c3c6ce"; // light gray
    public static final String VIOLATOR_WARNING_ICON_COLOR = "#df3d00"; // red
    
    //User dot presence colors from Social Hub
    public static final String USER_DOT_PRESENCE_OFFLINE_COLOUR = "#c3c6ce"; // light gray
    public static final String ORIGIN_ONLINE_STATUS_COLOUR = "#04bd68"; //green
    public static final String ORIGIN_AWAY_STATUS_COLOUR = "#df3d00"; //red
    public static final String ORIGIN_INVISIBLE_STATUS_COLOUR = "#c3c6ce"; // grey
    public static final String ORIGIN_IN_GAME_STATUS_COLOUR = "#2ac4f5"; //blue

    // default Origin Access Discount Percentage
    public static final int ORIGIN_ACCESS_DISCOUNT_PERCENTAGE = 10;

    // Captcha text for Automation
    public static final String CAPTCHA_AUTOMATION = "automation hack";

    //Origin Access
    public static final int ORIGIN_ACCESS_TRIAL_LENGTH = 7;
    public static final String LOG_PATH = PROGRAM_DATA_NONXP_PATH + "\\Logs";
    public static final String ORIGIN_ACCESS_MONTHLY_PLAN_PRICE = "4.99";
    public static final String ORIGIN_ACCESS_YEARLY_PLAN_PRICE = "29.99";
    public static final String ORIGIN_ACCESS_MONTHLY_PLAN_PRICE_WITH_VAT = "5.49";
    public static final String ORIGIN_ACCESS_YEARLY_PLAN_PRICE_WITH_VAT = "33.02";
    public static final String ORIGIN_ACCESS_PREMIER_YEARLY_PLAN_PRICE_WITH_VAT = "129.99";
    public static final String ORIGIN_ACCESS_PREMIER_MONTHLY_PLAN_PRICE_WITH_VAT = "19.99";
    public static final String ORIGIN_ACCESS_MONTHLY_PLAN_OFFER_ID = "Origin.OFR.50.0001171"; //4.99 per month
    public static final String ORIGIN_ACCESS_YEARLY_PLAN_OFFER_ID = "Origin.OFR.50.0000751"; // 29.99 per year
    public static final String ORIGIN_ACCESS_PREMIER_MONTHLY_PLAN = "monthly";
    public static final String ORIGIN_ACCESS_PREMIER_YEARLY_PLAN = "yearly";
    public static final String ORIGIN_ACCESS_YEARLY_PLAN_EU_PRICE = "24.99";
    public static final String ORIGIN_ACCESS_SELECTED_PLAN_BORDER_COLOR = "#04BD68"; // green
    
    // Constants for Origin Logging
    public static final String CLIENT_LOG = "Client_Log.txt";

    // Add friends title
    public static final String[] NO_FRIENDS_TITLE = {"Add", "some", "friends", "have", "any", "yet"};
    public static final String CHECKOUT_COUNTRY_USA = "United States";
    public static final int INSTALLATION_TIMEOUT_SEC = 60;
    public static final String TERMS_OF_SERVICE_URL_REGEX = "http://.*tos.ea.com.*WEBTERMS.*";
    public static final String STATE_BC = "British Columbia";

    // Origin X URLs
    public static final String MAIN_SPA_PAGE_URL = ".*origin.com.*(home|discover|store|game-library|settings|profile).*";
    public static final String CORPORATE_INFORMATION_URL_REGEX = "http://.*www.ea.com.*about.*";
    public static final int DOWNLOAD_PREPARATION_TIMEOUT_SEC = 60;
    public static final String ACCOUNT_SETTINGS_PAGE_URL_REGEX = "https://myaccount.*.ea.com/.*";

    // Common url for switching context to any igo based window
    public static final String OIG_PAGE_URL_REGEX = ".*Origin::Engine::IGOQWidget.*";
    public static final String PRIVACY_COOKIE_POLICY_URL_REGEX = "http://.*tos.ea.com.*WEBPRIVACY.*";

    // Game slideout download/install/play state transition timeout
    public static final int DOWNLOAD_INITIAL_TIMEOUT_SEC = 60;
    public static final String EULA_URL_REGX = "http://.*www.ea.com.*product-eulas.*";
    public static final String INVALID_PATH = "C:\\invalid path#@^\\";
    public static final String GIFT_RECEIVED_URL_REGEX_TMPL = ".*origin.com.*gift.*receive/%s";
    public static final String DEFAULT_GAME_INSTALLER = "setup.exe";
    public static final int GAME_LAUNCH_TIMEOUT_SEC = 60;

    // processes
    public static final String ORIGIN_PROCESS_NAME = "Origin.exe";
    public static final int INSTALLATION_PREPARATION_SEC = 60;
    public static final String LEGAL_NOTICES_URL_REGEX = "http://.*www.ea.com.*legal-notices.*";
    public static final int PAUSE_DOWNLOAD_TIMEOUT_SEC = 60;
    public static final String TERMS_OF_SALE_URL_REGEX = "http://.*tos.ea.com.*termsofsale.*";

    //Common url and timeout used in all test cases
    public static final String LOGIN_PAGE_URL_REGEX = "^https://signin.*";

    //Notification Toast expiry time (delay time)
    public static final int DEFAULT_NOTIFICATION_EXPIRY_TIME = 1200; // msec, per developer MFong 20160610

    //Default number of friends showing in friends tab
    public static final int DEFAULT_FRIENDS_TAB_FRIEND_COUNT = 9;

    //Default number of additional friends showing in friends tab when clicking 'View More' button
    public static final int DEFAULT_VIEW_MORE_FRIEND_COUNT = 50;

    /*
     * Private constructor; we never want an object instantiated.
     */
    private OriginClientData() {
        // Do Nothing
    }
}
