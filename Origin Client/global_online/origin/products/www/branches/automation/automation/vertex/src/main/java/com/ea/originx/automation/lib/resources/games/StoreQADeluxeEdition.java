package com.ea.originx.automation.lib.resources.games;

import com.ea.vx.originclient.resources.games.EntitlementInfo;
import java.lang.invoke.MethodHandles;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

/**
 * A class to manage the Origin Store QA Deluxe entitlement
 * (OriginQABuilds.OFR.0009)
 *
 * @author glivingstone
 */
public final class StoreQADeluxeEdition extends EntitlementInfo {

    // Offer IDs for Expansions
    public static final String QA_DELUXE_EXPANSION_004_OFFER_ID = "ATOffer.Offer0074.Exp";
    public static final String QA_DELUXE_EXPANSION_QA_TEST_OFFER_ID = "OriginQAExpansions.OFR.0002";

    // Offer IDs for Add-Ons
    public static final String QA_DLC_OFFER_ADDON_OFFER_ID = "OriginQADLC.OFR.0001";
    public static final String QA_DELUXE_ADDON_001_PUBLISHED_OFFER_ID = "Origin QA.OFR.50.0000178";
    public static final String QA_DELUXE_ADDON_005_PUBLISHED_OFFER_ID = "Origin QA.OFR.50.0000182";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor. Make private as this class should never be instantiated
     */
    public StoreQADeluxeEdition() {
        super(configure()
                .name(EntitlementInfoConstants.QA_DELUXE_NAME)
                .offerId(EntitlementInfoConstants.QA_DELUXE_OFFER_ID)
        );
    }
}
