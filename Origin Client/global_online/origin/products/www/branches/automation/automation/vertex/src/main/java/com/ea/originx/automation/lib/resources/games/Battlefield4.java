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
public final class Battlefield4 extends EntitlementInfo {

    // Final Stand
    public static final String BF4_FINAL_STAND_NAME = "Battlefield 4™ Final Stand";
    public static final String BF4_FINAL_STAND_OFFER_ID = "OFB-EAST:109550823";
    public static final String BF4_FINAL_STAND_PARTIAL_PDP_URL = "/store/battlefield/battlefield-4/expansion/battlefield-4-final-stand?";

    // Legacy Operations
    public static final String BF4_LEGACY_OPERATIONS_NAME = "Battlefield 4™ Legacy Operations";
    public static final String BF4_LEGACY_OPERATIONS_OFFER_ID = "Origin.OFR.50.0000911";

    // BF4 bronze battlepack
    public static final String BF4_BRONZE_BATTLEPACK_NAME = "Battlefield 4™ Bronze Battlepack";
    public static final String BF4_BRONZE_BATTLEPACK_OFFER_ID = "Origin.OFR.50.0000093";
    public static final String BF4_BRONZE_BATTLEPACK_PARTIAL_PDP_URL = "/store/battlefield/battlefield-4/addon/battlefield-4-bronze-battlepack?";

    public static final String BF4_AIR_VEHICLE_SHORTCUT_KIT_NAME = "Battlefield 4™ Air Vehicle Shortcut Kit";
    public static final String BF4_AIR_VEHICLE_SHORTCUT_KIT_OFFER_ID = "Origin.OFR.50.0000036";
    
    // BF4 Editions
    public static final String BF4_STANDARD_EDITION_NAME = "Standard Edition";
    public static final String BF4_DIGITAL_DELUXE_EDITION_NAME = "Digital Deluxe Edition";
    public static final String BF4_PREMIUM_EDITION_NAME = "Premium Edition";
    
    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor. Make private as this class should never be instantiated
     */
    public Battlefield4() {
        super(configure()
                .parentName(EntitlementInfoConstants.BATTLEFIELD_4_PARENT_NAME)
                .name(EntitlementInfoConstants.BF4_STANDARD_NAME)
                .offerId(EntitlementInfoConstants.BF4_STANDARD_OFFER_ID)
                .partialPdpUrl(EntitlementInfoConstants.BATTLEFIELD_4_PARTIAL_PDP_URL)
                .ocdPath(EntitlementInfoConstants.BF4_STANDARD_OCD_PATH)
        );
    }
}
