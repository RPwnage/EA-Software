package com.ea.originx.automation.lib.helpers;

import com.ea.originx.automation.lib.resources.EACoreData;
import com.ea.vx.originclient.resources.CountryInfo;
import com.ea.vx.originclient.resources.EACore;

/**
 * A collection of helpers that modify the EACore configuration.
 */
public class EACoreHelper {

    /**
     * Set the port number on which the WebDriver should listen to.
     *
     * @param portNumber The port number on which the WebDriver should listen to
     */
    public static void setQtWebDriverPortNumberEACore(int portNumber, EACore eaCore) {
        eaCore.setEACoreValue(EACore.EACORE_AUTOMATION_SECTION, EACore.EACORE_QTWEBDRIVERPORT_KEY, Integer.toString(portNumber));
    }

    /**
     * Set the log dirty bits setting.
     *
     * @param eaCore The EACore configuration object.
     */
    public static void setLogDirtyBitsSetting(EACore eaCore) {
        eaCore.setEACoreValue(EACore.EACORE_LOGGING_SECTION, EACore.EACORE_LOG_DIRTY_BITS_KEY, EACore.EACORE_TRUE);
    }

    /**
     * Set the automation config URL.
     *
     * @param value The Origin X config URL
     * @param eaCore The EACore configuration object.
     */
    public static void setAutomationConfigURL(String value, EACore eaCore) {
        eaCore.setEACoreValue("URLs", "OriginXconfigURL", value);
    }

    /**
     * Set the debug menu setting.
     *
     * @param eaCore The EACore configuration object.
     */
    public static void setShowDebugMenuSetting(EACore eaCore) {
        eaCore.setEACoreValue(EACore.EACORE_QA_SECTION, EACore.EACORE_SHOW_DEBUG_MENU_KEY, EACore.EACORE_TRUE);
    }

    /**
     * Set the notification expiry time to 60 seconds.
     *
     * @param eaCore The EACore configuration object.
     */
    public static void extendNotificationExpiryTime(EACore eaCore) {
        eaCore.setEACoreValue(EACore.EACORE_AUTOMATION_SECTION, EACore.EACORE_NOTIFICATION_EXPIRATION_KEY, EACore.NOTIFICATION_EXPIRATION_VALUE);
    }

    /**
     * Set the automation experiment setting (Used for automation experimentation variant tests).
     * @param eaCore The EACore configuration object.
     */
    public static void setAutomationExperimentSetting(EACore eaCore) {
        eaCore.setEACoreValue("URLs", "OriginXconfigURL", "https://config.x.origin.com/automation");
    }

    /**
     * Set the subscription allowed time to play offline
       *
     * @param eaCore The EACore configuration object.
     * @param offlineTime time until the offline timer starts
     */
    public static void setSubscriptionMaxOfflinePlay(EACore eaCore, String offlineTime) {
        eaCore.setEACoreValue(EACore.EACORE_FEATURE_SECTION, "SubscriptionMaxOfflinePlay", offlineTime);
    }

    /**
     * Change the PI Entitlement to the Basic version.
     *
     * @param eaCore The EACore configuration object.
     * @return true if successfully changed, otherwise false
     */
    public static boolean changeToBaseEntitlementPIOverride(EACore eaCore) {
        return eaCore.setEACoreValue(EACore.EACORE_CONNECTION_SECTION, EACore.EACORE_BUILD_IDENTIFIER_OVERRIDE + EACoreData.PI_OFFER_ID, EACoreData.BASE_VERSION_PI_ENTITLEMENT_OVERRIDE);
    }

    /**
     * Overrides the Akamai location and fakes being in a different country.
     *
     * @param country The country to pretend to be in
     * @param eaCore The EACore configuration object
     */
    public static void overrideCountryTo(CountryInfo.CountryEnum country, EACore eaCore) {
        // Keys to override GeoIp (see https://confluence.ea.com/display/EBI/GEO+IP+Bypassing+on+X)
        eaCore.setEACoreValue("Services", "GeoLocationIPAddress", country.getGeoIp());
    }

    /**
     * Overrides the Akamai location and fakes being in a different country.
     *
     * @param country The country to pretend to be in
     * @param eaCore The EACore configuration object
     */
    public static void overrideCountryTo(String country, EACore eaCore) {
        // Keys to override GeoIp (see https://confluence.ea.com/display/EBI/GEO+IP+Bypassing+on+X)
        eaCore.setEACoreValue("Services", "GeoLocationIPAddress", country);
    }
}