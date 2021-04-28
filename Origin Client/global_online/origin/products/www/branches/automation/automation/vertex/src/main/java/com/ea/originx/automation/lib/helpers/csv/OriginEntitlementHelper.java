package com.ea.originx.automation.lib.helpers.csv;

import com.ea.vx.originclient.resources.games.EntitlementInfo;
import java.util.ArrayList;
import java.util.List;

/**
 * Represents each row in entitlement.csv file.
 *
 * @author svaghayenegar
 */
public class OriginEntitlementHelper {

    protected String entitlementOfferId;
    protected String entitlementName;
    protected String entitlementFamilyName;
    protected String entitlementPartialPDPUrl;
    protected boolean isBaseGameNeeded;
    protected List<Integer> baseGameRows = new ArrayList<>();
    protected boolean isBundle;
    protected List<Integer> bundleRows = new ArrayList<>();
    protected String gameLibraryImageName;

    /**
     * Constructor for EntitlementInfo.
     *
     * @param entitlementOfferId       OfferID of the entitlement
     * @param entitlementName          Name of the entitlement
     * @param entitlementPartialPDPUrl Partial PDP URL of the entitlement
     */
    public OriginEntitlementHelper(String entitlementOfferId, String entitlementName, String entitlementFamilyName, String entitlementPartialPDPUrl,
                                   String isBaseGameNeeded, String baseGameRows, String isBundle, String bundleRows, String gameLibraryImageName) {
        this.entitlementFamilyName = entitlementFamilyName;
        this.entitlementName = entitlementName;
        this.entitlementOfferId = entitlementOfferId;
        this.entitlementPartialPDPUrl = entitlementPartialPDPUrl;
        this.isBaseGameNeeded = Boolean.parseBoolean(isBaseGameNeeded);
        if (isBaseGameNeeded()) {
            String[] array = baseGameRows.split(",");
            for (String baseGameRow : array) {
                this.baseGameRows.add(Integer.parseInt(baseGameRow) - 2);// To get the base game, list starts from zero, also first row is header
            }
        }
        this.isBundle = Boolean.parseBoolean(isBundle);
        if (isBundle()) {
            String[] array = bundleRows.split(",");
            for (String bundleRow : array) {
                this.bundleRows.add(Integer.parseInt(bundleRow) - 2);// To get the entitlement, list starts from zero, also first row is header
            }
        }
        this.gameLibraryImageName = gameLibraryImageName;
    }

    /**
     * Gets the entitlement family name.
     *
     * @return Family name of the entitlement
     */
    public String getEntitlementFamilyName() {
        return entitlementFamilyName;
    }

    /**
     * Gets the entitlement name.
     *
     * @return Name of the entitlement
     */
    public String getEntitlementName() {
        return entitlementName;
    }

    /**
     * Gets the entitlement offerID.
     *
     * @return offerId of the entitlement
     */
    public String getEntitlementOfferId() {
        return entitlementOfferId;
    }

    /**
     * Gets the entitlement's partial PDP URL.
     *
     * @return Partial PDP URL of the entitlement
     */
    public String getEntitlementPartialPDPUrl() {
        return entitlementPartialPDPUrl;
    }

    /**
     * Returns true if entitlement needs a base game, false otherwise
     *
     * @return true if entitlement needs a base game, false otherwise
     */
    public boolean isBaseGameNeeded() {
        return isBaseGameNeeded;
    }

    /**
     * Gets a list of entitlement's required base game
     *
     * @return base game rows of the entitlement
     */
    public List<Integer> getBaseGameRows() {
        return baseGameRows;
    }

    /**
     * Returns true if entitlement is a bundle, false otherwise
     *
     * @return true if entitlement is a bundle, false otherwise
     */
    public boolean isBundle() {
        return isBundle;
    }

    /**
     * Gets a list of entitlement's bundle games
     *
     * @return bundle rows of the entitlement bundle
     */
    public List<Integer> getBundleRows() {
        return bundleRows;
    }

    /**
     * Returns this as an EnitlementInfo object
     * @return EnitlementInfo object
     */
    public EntitlementInfo getAsEntitlementInfo(){
        return convertToEntitlementInfo(this);
    }

    /**
     * Returns a list of base games as EnitlementInfo objects
     * @return list of base games as EnitlementInfo objects
     */
    public List<EntitlementInfo> getBaseGamesEntitlementInfo(){
        List<EntitlementInfo> baseGames = new ArrayList<>();
        for (Integer baseGameRow : getBaseGameRows()) {
            OriginEntitlementHelper originEntitlementHelper = OriginEntitlementReader.getEntitlementInfo(baseGameRow);
            baseGames.add(convertToEntitlementInfo(originEntitlementHelper));
        }
        return baseGames;
    }

    /**
     * Returns a list of bundle entitlements as OriginEntitlementHelper objects
     * @return list of bundle entitlements as OriginEntitlementHelper objects
     */
    public List<OriginEntitlementHelper> getBundlesEntitlementInfo(){
        List<OriginEntitlementHelper> bundleEntitlements = new ArrayList<>();
        for (Integer bundleRow : getBundleRows()) {
            OriginEntitlementHelper originEntitlementHelper = OriginEntitlementReader.getEntitlementInfo(bundleRow);
            bundleEntitlements.add(originEntitlementHelper);
        }
        return bundleEntitlements;
    }

    public String getGameLibraryImageName() {
        return gameLibraryImageName;
    }

    /**
     * Converts an OriginEntitlementHelper to an EnitlementInfo objects
     * @param originEntitlementHelper OriginEntitlementHelper object
     * @return EnitlementInfo objects
     */
    private EntitlementInfo convertToEntitlementInfo(OriginEntitlementHelper originEntitlementHelper){
        return EntitlementInfo
                .configure()
                .offerId(originEntitlementHelper.getEntitlementOfferId())
                .name(originEntitlementHelper.getEntitlementName())
                .parentName(originEntitlementHelper.getEntitlementFamilyName())
                .partialPdpUrl(originEntitlementHelper.getEntitlementPartialPDPUrl())
                .build();
    }
}
