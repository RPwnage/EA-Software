package com.ea.originx.automation.lib.resources.games;

import com.ea.vx.originclient.helpers.ResultLogger;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.opencsv.CSVReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

/**
 * Game Info Depot with ENUMs and static methods for retrieving game info from
 * csv file. Each CSV row contains (to be finalized):<br>
 *
 * - family, name, type, offerId, processName, partialURL<br>
 *
 * Examples:<br>
 * - the-sims/the-sims-3, The Sims™ 3, base, OFB-EAST:55107, Sims3Launcher.exe,
 * /store/the-sims/the-sims-3/standard-edition?<br>
 * - the-sims/the-sims-3, The Sims™ 3 Into the Future, dlc,
 * Origin.OFR.50.0000167, Sims3Launcher.exe,
 * /store/the-sims/the-sims-3/expansion/the-sims-3-into-the-future?
 *
 * @author palui
 */
public class GameInfoDepot {

    private static final Logger _log = LogManager.getLogger(ResultLogger.class);

    static final String csvFile = SystemUtilities.getCurrentWorkingDirectory() + "/Suites/gameinfo.csv";

    /**
     * Indexes
     */
    static final int familyNameIndex = 0;
    static final int gameNameIndex = 1;
    static final int offerTypeIndex = 2;
    static final int offerIDIndex = 3;
    static final int processNameIndex = 4;
    static final int partialPDPURLIndex = 5;
    static final int automatableInstallIndex = 6;
    static final int automatablePlayIndex = 7;

    /**
     * Constructor - make private to disallow instantiation
     */
    private GameInfoDepot() {
    }

    /**
     * Enum for GameFamilyInfo type. Each game family (e.g. SIMS3) is assigned a
     * unique enum as per unique family type strings in the CSV file
     */
    public enum GameFamilyType {

        BF1("battlefield/battlefield-1"),
        BF3("battlefield/battlefield-3"),
        BF4("battlefield/battlefield-4"),
        BF_HARDLINE("battlefield/battlefield-hardline"),
        MASS_EFFECT_2("mass-effect/mass-effect-2"),
        MASS_EFFECT_3("mass-effect/mass-effect-3"),
        SIMS3("the-sims/the-sims-3"),
        SIMS4("the-sims/the-sims-4"),
        THIS_WAR_OF_MINE("this-war-of-mine/this-war-of-mine"),
        MOH_WAR_FIGHTER("medal-of-honor/medal-of-honor-warfighter"),
        DS3("dead-space/dead-space-3"),
        NFS_RIVAL("need-for-speed/need-for-speed-rival"),
        DAO("dragon-age/dragon-age-origins"),
        DAI("dragon-age/dragon-age-inquisition"),
        THE_BANNER_SAGA("banner-saga/the-banner-saga"),
        PILLARS_OF_ETERNITY("pillars-of-eternity/pillars-of-eternity"),
        ASSASSINS_CREED_3("assassins-creed/assassins-creed-iii"),
        THE_WITCHER_3("witcher/the-witcher-wild-hunt"),
        THE_WITCHER_3_GOTY("witcher/the-witcher-wild-hunt-game-of-the-year"),
        CRYSIS_3("crysis/crysis-3"),
        KINGDOMS_OF_AMALUR_RECKONING("kingdoms-of-amalur-reckoning/kingdoms-of-amalur-reckoning");

        private final String familyString;

        GameFamilyType(final String familyString) {
            this.familyString = familyString;
        }

        /**
         * Get GameFamilyInfo type given the status string
         *
         * @param familyString the type string to match
         *
         * @return {@link GameFamilyType} with matching string, otherwise throw
         * exception
         */
        public static GameFamilyType parseGameFamilyType(String familyString) {
            for (GameFamilyType type : GameFamilyType.values()) {
                if (type.familyString.equals(familyString)) {
                    return type;
                }
            }
            String errMessage = String.format("Cannot find game info family type for '%s'. Perhaps a new type enum should be added?", familyString);
            _log.error(errMessage);  // Exception may not shown in output, log the error
            throw new RuntimeException(errMessage);
        }

        @Override
        public String toString() {
            return familyString;
        }
    }

    /**
     * Enum for GameInfo type. A GameInfo corresponds to a row in the csv file
     */
    public enum GameInfoType {

        // May extend to: BASE_GOLD, DLC_NONPLAYABLE, etc.
        BASE("base"),
        BASE_DELUXE("base_deluxe"),
        BASE_PREMIUM("base_premium"),
        BASE_ULTIMATE("base_ultimate"),
        BASE_GAME_OF_THE_YEAR("base_game_of_the_year"),
        BASE_ROYAL("base_royal"),
        BASE_CHAMPION("base_champion"),
        BASE_HUNTER("base_hunter"),
        DLC("dlc"),
        ULC("ulc"),
        SOUNDTRACK("soundtrack"),
        DIGITALBOOK("digitalbook"),
        IMAGES("images"),
        VIDEOS("videos"),
        MUSIC("music");

        private final String typeString;

        GameInfoType(final String typeString) {
            this.typeString = typeString;
        }

        /**
         * Get GameInfo type given the type string
         *
         * @param typeString the type string to match
         *
         * @return {@link GameInfoType} with matching string, otherwise throw
         * exception
         */
        public static GameInfoType parseGameInfoType(String typeString) {
            for (GameInfoType type : GameInfoType.values()) {
                if (type.typeString.equals(typeString)) {
                    return type;
                }
            }
            String errMessage = String.format("Cannot find game info type for '%s'. Perhaps a new type enum should be added?", typeString);
            System.out.println(errMessage);
            throw new RuntimeException(errMessage);
        }

        @Override
        public String toString() {
            return typeString;
        }
    }

    /**
     * @param familyType the family of GameFamilyInfo to retrieve
     * @return {@link GameFamilyInfo} of the specified type
     */
    public static GameFamilyInfo getGameFamily(GameFamilyType familyType) {
        GameFamilyInfo gameFamilyInfo = null;
        try (CSVReader reader = new CSVReader(new InputStreamReader(new FileInputStream(csvFile), "UTF-8"))) {
            // Read (and ignore) the header row. Throw exception if file is empty
            if (reader.readNext() == null) {
                String errMessage = String.format("Empty csv file : '%s'", csvFile);
                _log.error(errMessage);  // exception may not show in output, need to log first
                throw new RuntimeException(errMessage);
            }
            // Read the csv file and extra all GameInfo for the specified family
            List<GameInfo> baseInfos = new ArrayList<>();
            List<GameInfo> dlcInfos = new ArrayList<>();
            List<GameInfo> ulcInfos = new ArrayList<>();
            List<GameInfo> otherGameInfos = new ArrayList<>();
            String[] line;
            while ((line = reader.readNext()) != null) {
                GameInfo gameInfo = getGameInfoFromCSVRow(line);
                if (gameInfo.getFamilyType() == familyType) {
                    switch (gameInfo.getGameInfoType()) {
                        case BASE:
                        case BASE_CHAMPION:
                        case BASE_DELUXE:
                        case BASE_GAME_OF_THE_YEAR:
                        case BASE_HUNTER:
                        case BASE_PREMIUM:
                        case BASE_ROYAL:
                        case BASE_ULTIMATE:
                            baseInfos.add(gameInfo);
                            break;
                        case DLC:
                            dlcInfos.add(gameInfo);
                            break;
                        case ULC:
                            ulcInfos.add(gameInfo);
                            break;
                        case SOUNDTRACK:
                        case DIGITALBOOK:
                        case IMAGES:
                        case VIDEOS:
                        case MUSIC:
                            otherGameInfos.add(gameInfo);
                            break;
                        default:
                            String errMessage = String.format("Cannot find game info type for '%s'. Perhaps a new type should be added?", line[2].trim());
                            _log.error(errMessage);  // exception not showing in output, need to log first
                            throw new RuntimeException(errMessage);
                    }
                }
            }
            gameFamilyInfo = new GameFamilyInfo(familyType, baseInfos, dlcInfos, ulcInfos, otherGameInfos);
        } catch (IOException e) {
            String errMessage = String.format("IO Error encountered reading csvfile %s: %s", csvFile, e.toString());
            _log.error(errMessage);
            throw new RuntimeException(errMessage);
        }
        return gameFamilyInfo;
    }

    /**
     * @param line line of the CSV file as a String array
     * @return {@link GameInfo} constructed from a line of the CSV file
     */
    private static GameInfo getGameInfoFromCSVRow(String[] line) {
        String family = line[familyNameIndex].trim();
        String name = line[gameNameIndex].trim();
        String type = line[offerTypeIndex].trim();
        String offerId = line[offerIDIndex].trim();
        String processName = line[processNameIndex].trim();
        String partialPdpUrl = line[partialPDPURLIndex].trim();
        String automatableInstall = line[automatableInstallIndex].trim();
        String automatablePlay = line[automatablePlayIndex].trim();
        return new GameInfo(family, name, type, offerId, processName, partialPdpUrl, automatableInstall, automatablePlay);
    }

    /**
     * @param gameName the name of the game to retrieve from the csv file
     * @return {@link GameInfo} with matching name
     */
    public static GameInfo getGameInfo(String gameName) {
        try (CSVReader reader = new CSVReader(new InputStreamReader(new FileInputStream(csvFile), "UTF-8"))) {
            // Read (and ignore) the header row. Throw exception if file is empty
            if (reader.readNext() == null) {
                String errMessage = String.format("Empty csv file : '%s'", csvFile);
                _log.error(errMessage);  // exception may not show in output, need to log first
                throw new RuntimeException(errMessage);
            }
            // Read the csv file and extra all GameInfo for the specified family
            String[] line;
            while ((line = reader.readNext()) != null) {
                GameInfo gameInfo = getGameInfoFromCSVRow(line);
                if (gameInfo.getName().equals(gameName)) {
                    return gameInfo;
                }
            }
        } catch (IOException e) {
            String errMessage = String.format("IO Error encountered reading csvfile %s: %s", csvFile, e.toString());
            _log.error(errMessage);
            throw new RuntimeException(errMessage);
        }
        String errMessage = String.format("Cannot find game '%s' in csvfile %s", gameName, csvFile);
        _log.error(errMessage);
        throw new RuntimeException(errMessage);
    }

    /**
     * Get the game information from the CSV file based on given row
     *
     * @param rowNumber the row in the CSV file
     * @return Game Info corresponding to the row number is the CSV
     */
    public static GameInfo getGameInfo(int rowNumber) {
        GameInfo gameInfo;

        try (CSVReader reader = new CSVReader(new InputStreamReader(new FileInputStream(csvFile), "UTF-8"))) {
            if (reader.readNext() == null) {
                String errMessage = String.format("Empty csv file : '%s'", csvFile);
                _log.error(errMessage);  // exception may not show in output, need to log first
                throw new RuntimeException(errMessage);
            }
            List<String[]> readers = reader.readAll();
            gameInfo = getGameInfoFromCSVRow(readers.get(rowNumber));

        } catch (IOException e) {
            String errMessage = String.format("IO Error encountered reading csvfile %s: %s", csvFile, e.toString());
            _log.error(errMessage);
            throw new RuntimeException(errMessage);
        }

        return gameInfo;
    }
}
