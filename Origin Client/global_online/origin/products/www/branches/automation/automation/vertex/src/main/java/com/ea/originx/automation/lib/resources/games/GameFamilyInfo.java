package com.ea.originx.automation.lib.resources.games;

import com.ea.originx.automation.lib.resources.games.GameInfoDepot.GameFamilyType;
import com.ea.originx.automation.lib.resources.games.GameInfoDepot.GameInfoType;
import java.util.ArrayList;
import java.util.List;

/**
 * Game Family containing GameInfo for base games, DLCs (Downloadable Contents),
 * and ULCs (Unlockable Contents)
 *
 * @author palui
 */
public class GameFamilyInfo {

    private final GameFamilyType familyType;
    private List<GameInfo> baseInfos = new ArrayList<>();   // base games
    private List<GameInfo> dlcInfos = new ArrayList<>();    // downloadable contents
    private List<GameInfo> ulcInfos = new ArrayList<>();    // unlockable contens
    private List<GameInfo> otherGameInfos = new ArrayList<>();  // others e.g. sountrack

    /**
     * Constructor (used from GameInfoDepot)
     *
     * @param familyType GameFamilyInfo type
     * @param baseInfos Base GameInfo's
     * @param dlcInfos DLC GameInfo's
     * @param ulcInfos ULC GameInfo's
     * @param otherGameInfos other GameInfos
     */
    protected GameFamilyInfo(GameFamilyType familyType, List<GameInfo> baseInfos, List<GameInfo> dlcInfos, List<GameInfo> ulcInfos, List<GameInfo> otherGameInfos) {
        this.familyType = familyType;
        this.baseInfos = baseInfos;
        this.dlcInfos = dlcInfos;
        this.ulcInfos = ulcInfos;
        this.otherGameInfos = otherGameInfos;
    }

    /**
     * Getter for an instance of this object
     *
     * @param familyType the family of GameInfo to construct
     * @return GameFamilyInfo for this family type
     */
    public static GameFamilyInfo getGameFamilyInfo(GameFamilyType familyType) {
        return GameInfoDepot.getGameFamily(familyType);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Getters for instance attributes
    ////////////////////////////////////////////////////////////////////////////
    /**
     * @param baseType base GameInfo type (BASE, BASE_DELUXE, etc.)
     *
     * @return GameInfo of the base game with the specified type if found, or
     * throw exception if not found
     */
    public GameInfo getBaseInfo(GameInfoType baseType) {
        for (GameInfo baseGameInfo : baseInfos) {
            if (baseGameInfo.getGameInfoType() == baseType) {
                return baseGameInfo;
            }
        }
        String errMessage = String.format("Cannot find base game info for family '%s'", familyType.toString());
        System.out.println(errMessage);  // Exception may not appear in output. Log the error first
        throw new RuntimeException(errMessage);
    }

    /**
     * @return GameInfo of the (Standard) base game
     */
    public GameInfo getBaseGameInfo() {
        return getBaseInfo(GameInfoType.BASE);
    }

    public List<GameInfo> getBaseGames() {
        return baseInfos;
    }

    /**
     * @return GameInfo of the (Deluxe) base game
     */
    public GameInfo getDeluxeBaseGameInfo() {
        return getBaseInfo(GameInfoType.BASE_DELUXE);
    }

    /**
     * @return GameInfo's of the DLCs
     */
    public List<GameInfo> getDlcInfos() {
        return dlcInfos;
    }

    /**
     * @return GameInfo's of the ULCs
     */
    public List<GameInfo> getUlcInfos() {
        return ulcInfos;
    }

    /**
     * @return GameInfo's of the ULCs
     */
    public List<GameInfo> getOtherGameInfos() {
        return otherGameInfos;
    }

    /**
     * @param gameInfos list of GameInfo's
     * @return String representation of list of GameInfo's, each entry in a new
     * line
     */
    private String gameInfosToString(List<GameInfo> gameInfos) {
        String result = "";
        for (GameInfo gameInfo : gameInfos) {
            result += gameInfo.toString() + "\n";
        }
        return result;
    }

    /**
     * @return String representation of GameFamilyInfo, including family type,
     * base GameInfo's, DLC GameInfo's and ULC GameInfo's
     */
    @Override
    public String toString() {
        String familyResult = "Game Info Family: " + familyType.toString() + "\n";
        familyResult += "baseGames:\n" + gameInfosToString(baseInfos);
        familyResult += "Downloadable Contents (DLCs):\n" + gameInfosToString(dlcInfos);
        familyResult += "Unlockable Contents (ULCs):\n" + gameInfosToString(ulcInfos);
        familyResult += "Other Game Contents:\n" + gameInfosToString(otherGameInfos);
        return familyResult;
    }
}
