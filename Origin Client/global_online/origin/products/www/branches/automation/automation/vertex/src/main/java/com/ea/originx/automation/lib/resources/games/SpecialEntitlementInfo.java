package com.ea.originx.automation.lib.resources.games;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * While adding certain entitlements to the game library- it either adds a
 * different entitlement(which has different offerId) or a set of entitlements
 * For each pdpOfferId there is corresponding set of gameLibraryOfferIds
 *
 * For example:When you add Sims3 Starter Pack, we entitle Sims3 Standard
 * Edition in the game library Same as for Crysis Digital Deluxe Edition -
 * Crysis Hunter Edition which has Deluxe Edition plus 2 DLCs
 *
 * @author nvarthakavi
 */
public class SpecialEntitlementInfo {

    protected SpecialEntitlement specialEntitlementEnum;

    /**
     * Constructor
     *
     * @param specialEntitlementEnum SpecialEntitlement enum of the
     * OASpecialEntitlementInfo
     */
    public SpecialEntitlementInfo(SpecialEntitlement specialEntitlementEnum) {
        this.specialEntitlementEnum = specialEntitlementEnum;
    }

    /**
     * Constructor
     *
     * @param offerId offer id of the entitlement for the
     * OASpecialEntitlementInfo
     */
    public SpecialEntitlementInfo(String offerId) {
        this.specialEntitlementEnum = SpecialEntitlementInfo.SpecialEntitlement.parseSpecialEntitlementEnum(offerId);
    }

    public enum SpecialEntitlement {

        SIMS3_STARTER_PACK("OFB-EAST:64312", Arrays.asList("Origin.OFR.50.0001250")), // The Sims3 Starter Pack
        // THE SIMS3 - Origin.OFR.50.0001250

        CRYSIS_DELUXE_EDITION("Origin.OFR.50.0001097", Arrays.asList("OFB-EAST:49562")), // Crysis 3 Digital Deluxe Edition
        // CRYSIS3 HUNTER EDITION - OFB-EAST:49562

        MEDAL_OF_HONOUR("Origin.OFR.50.0000369", Arrays.asList("OFB-EAST:1000011")), //Medal of Honour Allied Assault War Chest
        // MEDAL OF HONOUR ALLIED ASSAULT - OFB-EAST:1000011

        COMMAND_AND_CONQUER("OFB-EAST:54863", Arrays.asList("OFB-EAST:52019", "OFB-EAST:52017", "OFB-EAST:52208", "OFB-EAST:52210", "OFB-EAST:52212", "OFB-EAST:52016",
                "OFB-EAST:52209", "OFB-EAST:52211", "OFB-EAST:55312", "OFB-EAST:52018")); //Command and Conquer The Ultimate Collection Additional
        // COMMAND AND CONQUER RED ALERT 2 AND YURI'S REVENGE - OFB-EAST:52019
        // COMMAND AND CONQUER RED ALERT 2, COUNTER STRIKE AND THE AFTERMATH - OFB-EAST:52017
        // COMMAND AND CONQUER RENEGADE - OFB-EAST:52208
        // COMMAND AND CONQUER TIBERIAN WARS AND KANE's WRATH - OFB-EAST:52210
        // COMMAND AND CONQUER TIBERIAN TWILIGHT - OFB-EAST:52212
        // COMMAND AND CONQUER AND THE COVERT OPERATIONS - OFB-EAST:52016
        // COMMAND AND CONQUER GENERALS AND ZERO HOUR - OFB-EAST:52209
        // COMMAND AND CONQUER RED ALERT 3 AND UPRISING - OFB-EAST:52211
        // COMMAND AND CONQUER THE ULTIMATE COLLECTION ADDITIONAL - OFB-EAST:55312
        // COMMAND AND CONQUER TIBERIAN SUN AND FIRESTORM - OFB-EAST:52018

        private final String pdpOfferId;
        private final List<String> gameLibraryOfferIds;

        /**
         * Constructor for SpecialEntitlement Enum
         *
         * @param offerId the PDP offer id of the entitlement
         * @param gameLibraryOfferIds the game library offer ids of all the
         * corresponding entitlement for the PDP offer id
         */
        SpecialEntitlement(String offerId, List<String> gameLibraryOfferIds) {
            this.pdpOfferId = offerId;
            this.gameLibraryOfferIds = gameLibraryOfferIds;
        }

        /**
         * To get the PDP offerId of the entitlement
         *
         * @return the offerId
         */
        public String getPDPOfferId() {
            return pdpOfferId;
        }

        /**
         * To get the list of Game Library offerId for the entitlement
         *
         * @return the Game Library Offer Ids of the entitlement
         */
        public List<String> getGameLibraryOfferIds() {
            return gameLibraryOfferIds;
        }

        /**
         * Get SpecialEntitlement enum for the given offer id
         *
         * @param offerId the offerId of the entitlement
         * @return {@link SpecialEntitlement} with matching offer id, otherwise
         * throw exception
         */
        public static SpecialEntitlement parseSpecialEntitlementEnum(String offerId) {
            for (SpecialEntitlement type : SpecialEntitlement.values()) {
                if (type.pdpOfferId.equals(offerId)) {
                    return type;
                }
            }
            String errMessage = String.format("Cannot find SpecialEntitlement enum the offer id for '%s'. A new enum type should be added", offerId);
            System.err.println(errMessage);  // Exception may not shown in output, log the error
            throw new RuntimeException(errMessage);
        }
    }

    /**
     * To get the PDP offerId of the entitlement
     *
     * @return the offerId
     */
    public String getPDPOfferId() {
        return specialEntitlementEnum.getPDPOfferId();
    }

    /**
     * To get the list of Game Library offerId for the entitlement
     *
     * @return the Game Library Offer Ids of the entitlement
     */
    public List<String> getGameLibraryOfferIds() {
        return specialEntitlementEnum.getGameLibraryOfferIds();
    }

    /**
     * Get all Entitlements in the Entitlement Enum Set
     *
     * @return the list of entitlement ids in Enum Set
     */
    private static List<SpecialEntitlementInfo> getAllSpecialEntitlements() {
        List<SpecialEntitlement> entitlementsIds = Arrays.asList(SpecialEntitlement.values());
        List<SpecialEntitlementInfo> entitlementInfos = new ArrayList<>();

        for (SpecialEntitlement entitlementId : entitlementsIds) {
            entitlementInfos.add(new SpecialEntitlementInfo(entitlementId));
        }

        return entitlementInfos;

    }

    /**
     * To get the list of all offerIds of the entitlement which are special
     * entitlements
     *
     * @return list of offer ids
     */
    public static List<String> getAllPDPOfferIds() {
        List<SpecialEntitlementInfo> specialEntitlementInfos = getAllSpecialEntitlements();
        List<String> offerIds = new ArrayList<>();

        for (SpecialEntitlementInfo specialEntitlementInfo : specialEntitlementInfos) {
            offerIds.add(specialEntitlementInfo.getPDPOfferId());
        }

        return offerIds;
    }

}
