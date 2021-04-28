package com.ea.originx.automation.lib.resources.games;

import com.ea.vx.originclient.resources.games.EntitlementInfo;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import java.lang.invoke.MethodHandles;

/**
 * Sims 4 DLC
 *
 * @author rocky
 */
public final class Sims4 extends EntitlementInfo {

    // name for Add-Ons
    public static final String SIMS4_CITY_LIVING_NAME = "The Sims™ 4 City Living";
    public static final String SIMS4_DLC_BACKYARD_NAME = "The Sims™ 4 Backyard Stuff";
    public static final String SIMS4_GET_BACK_TO_WORK_NAME = "The Sims™ 4 Get to Work";
    public static final String SIMS4_PREORDER_TEST_DLC_NAME = "Super Duper Sims Test Offer: Preorder"; //Fake DLC for Pre-order
    public static final String SIMS4_DISCOUNT_TEST_DLC_NAME = "Super Duper Sims Test Offer: Discounted"; // Fake DLC for Discounted

    // Offer IDs for Add-Ons
    public static final String SIMS4_CITY_LIVING_NAME_OFFER_ID = "SIMS4.OFF.SOLP.0x000000000001D5ED";
    public static final String SIMS4_DLC_BACKYARD_OFFER_ID = "SIMS4.OFF.SOLP.0x0000000000020176";
    public static final String SIMS4_GET_BACK_TO_WORK_OFFER_ID = "SIMS4.OFF.SOLP.0x0000000000011AC5";
    public static final String SIMS4_PREORDER_TEST_DLC_OFFER_ID = "SIMS4.OFR.50.0000052";
    public static final String SIMS4_DISCOUNT_TEST_DLC_OFFER_ID = "SIMS4.OFR.50.0000053";

    // Partial PDP url
    public static final String SIMS4_DLC_BACKYARD_PARTIAL_PDP_URL = "/store/the-sims/the-sims-4/addon/the-sims-4-backyard-stuff?";
    public static final String SIMS4_GET_BACK_TO_WORK_PDP_URL = "/store/the-sims/the-sims-4/expansion/the-sims-4-get-to-work?";
    public static final String SIMS4_PREORDER_TEST_DLC_NAME_PARTIAL_PDP_URL = "/store/the-sims/the-sims-4/addon/super-duper-sims-test-offer-preorder?";
    public static final String SIMS4_DISCOUNT_TEST_DLC_PARTIAL_PDP_URL = "/store/the-sims/the-sims-4/addon/super-duper-sims-test-offer-discounted?";

    // ocd-path
    public static final String SIMS4_TRIAL_OCD_PATH = "/the-sims/the-sims-4/trial";
    public static final String SIMS4_DEMO_OCD_PATH = "/the-sims/the-sims-4/demo";

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    /**
     * Constructor. Make private as this class should never be instantiated
     */
    public Sims4() {
        super(configure()
                .name(EntitlementInfoConstants.SIMS4_STANDARD_NAME)
                .offerId(EntitlementInfoConstants.SIMS4_STANDARD_OFFER_ID)
                .partialPdpUrl(EntitlementInfoConstants.SIMS4_STANDARD_PARTIAL_PDP_URL)
        );
    }
}
