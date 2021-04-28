package com.ea.originx.automation.lib.resources.games;

import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import java.lang.invoke.MethodHandles;

/**
 * A class to manage the Mass Effect 3 basegame and DLC entitlements
 *
 * @author rocky
 */
public final class MassEffect3 extends EntitlementInfo {

    // name for Add-Ons
    public static final String ME3_EXTENDED_CUT_NAME = "Extended Cut";
    public static final String ME3_CITADEL_NAME = "Citadel";

    // Offer IDs for Add-Ons
    public static final String ME3_EXTENDED_CUT_OFFER_ID = "OFB-EAST:58040";
    public static final String ME3_CITADEL_OFFER_ID = "OFB-MASS:57550";

    // Already used redemption codes
    public static final String ME3_REDEMPTION_CODE_USED_PROD = "FPX7-KYMQ-ZERX-Y6WA-VC2F";
    public static final String ME3_REDEMPTION_CODE_USED_NON_PROD = "FPWE-BLNA-SFVZ-3MCN-D9T9";

    // For achievement
    public static final String ME3_ACHIEVEMENT_PRODUCT_ID = "68481_69317_50844";
    public static final String ME3_ACHIEVEMENT_DRIVEN_NAME = "Driven";
    public static final String ME3_ACHIEVEMENT_DRIVEN_ID = "102";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor. Make private as this class should never be instantiated
     */
    public MassEffect3() {
        super(configure()
                .name(EntitlementInfoConstants.ME3_STANDARD_NAME)
                .offerId(EntitlementInfoConstants.ME3_STANDARD_OFFER_ID)
                .partialPdpUrl(EntitlementInfoConstants.ME3_STANDARD_PARTIAL_PDP_URL)
                .productCode(EntitlementInfoConstants.ME3_PRODUCT_CODE)
        );
    }
}
