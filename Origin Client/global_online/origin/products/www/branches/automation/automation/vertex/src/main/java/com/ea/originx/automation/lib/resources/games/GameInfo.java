package com.ea.originx.automation.lib.resources.games;

import com.ea.originx.automation.lib.resources.games.GameInfoDepot.GameFamilyType;
import com.ea.originx.automation.lib.resources.games.GameInfoDepot.GameInfoType;
import com.ea.vx.originclient.resources.games.EntitlementInfo;

/**
 * GameInfo object containing game attributes (represents a row of gameinfo.csv)
 *
 * @author palui
 */
public class GameInfo {

    protected String family;
    protected String name;
    protected String type; // base, base_deluxe, DLC, ULC
    protected String offerId;
    protected String processName;
    protected String partialPdpUrl; //partial store pdp url
    protected String automatableInstall;
    protected String automatablePlay;

    /**
     * Constructor
     *
     * @param family game family string
     * @param name game name string
     * @param type game type string
     * @param offerId game offerId string
     * @param processName game process name
     * @param partialPdpUrl game partial PDP URL
     */
    public GameInfo(String family, String name, String type, String offerId, String processName, String partialPdpUrl) {
        this.family = family;
        this.name = name;
        this.type = type;
        this.offerId = offerId;
        this.processName = processName;
        this.partialPdpUrl = partialPdpUrl;
    }

    /**
     * Constructor
     *
     * @param family game family string
     * @param name game name string
     * @param type game type string
     * @param offerId game offerId string
     * @param processName game process name
     * @param partialPdpUrl game partial PDP URL
     * @param automatableInstall whether or not install is completely
     * automatable
     * @param automatablePlay whether or not play is completely (game executable
     * or launcher is launched) automatable
     */
    public GameInfo(String family, String name, String type, String offerId, String processName, String partialPdpUrl, String automatableInstall, String automatablePlay) {
        this.family = family;
        this.name = name;
        this.type = type;
        this.offerId = offerId;
        this.processName = processName;
        this.partialPdpUrl = partialPdpUrl;
        this.automatableInstall = automatableInstall;
        this.automatablePlay = automatablePlay;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Getters for instance attributes
    ////////////////////////////////////////////////////////////////////////////
    public String getFamily() {
        return family;
    }

    /**
     * Get game name
     *
     * @return game name
     */
    public String getName() {
        return name;
    }

    /**
     * Get game type string
     *
     * @return game type string
     */
    public String getType() {
        return type;
    }

    /**
     * Get game offer id
     *
     * @return game offer id, or empty string if not (yet) defined
     */
    public String getOfferId() {
        return offerId;
    }

    /**
     * @return game process name (e.g. target.exe for fake games), or empty
     * string if not (yet) defined
     */
    public String getProcessName() {
        return processName;
    }

    /**
     * @return game partial pdp url, or empty string if not (yet) defined
     */
    public String getPartialPdpUrl() {
        return partialPdpUrl;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Getters for derived attributes
    ////////////////////////////////////////////////////////////////////////////
    /**
     * @return game type enum from type string
     */
    public GameInfoType getGameInfoType() {
        return GameInfoType.parseGameInfoType(type);
    }

    /**
     * @return game family type enum from family string
     */
    public GameFamilyType getFamilyType() {
        return GameFamilyType.parseGameFamilyType(family);
    }

    /**
     * Returns the automatable install information.
     *
     * @return String that contains automatable install information.
     */
    public String getAutomatableInstall() {
        return automatableInstall;
    }

    /**
     * Returns the automatable play information. Automatable play is determined
     * by whether or not a game's process is launched or a launcher for the game
     * is launched
     *
     * @return String that contains automatable play information.
     */
    public String getAutomatablePlay() {
        return automatablePlay;
    }

    /**
     * @return the game's corresponding EntitlementInfo
     */
    public EntitlementInfo getEntitlementInfo() {
        return EntitlementInfo.configure()
                .name(getName())
                .offerId(getOfferId())
                .processName(getProcessName())
                .partialPdpUrl(getPartialPdpUrl())
                .build();
    }

    public static EntitlementInfo getEntitlementInfoFor(GameInfo gameInfo) {
        return EntitlementInfo.configure()
                .name(gameInfo.getName())
                .offerId(gameInfo.getOfferId())
                .processName(gameInfo.getProcessName())
                .partialPdpUrl(gameInfo.getPartialPdpUrl())
                .build();
    }

    /**
     * @param gameName the name of the game to retrieve from the csv file
     * @return {@link GameInfo} with matching name
     */
    public static GameInfo getGameInfo(String gameName) {
        return GameInfoDepot.getGameInfo(gameName);
    }

    /**
     * @return String representation of GameInfo object
     */
    @Override
    public String toString() {
        return String.format("GameInfo[family: '%s', name: '%s', type: '%s', offerId: '%s', processName: '%s', partialPdpUrl: '%s'])",
                family, name, type, offerId, processName, partialPdpUrl);
    }
}
