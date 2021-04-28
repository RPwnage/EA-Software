package com.ea.originx.automation.lib.helpers.csv;

import com.ea.originx.automation.lib.helpers.TestScriptHelper;
import com.ea.vx.originclient.utils.SystemUtilities;
import com.opencsv.CSVReader;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

/**
 * To read entitlement information from entitlement.csv file in Suites
 * Each row represents an entitlement
 *
 * @author nvarthakavi
 */
public class OriginEntitlementReader {

    static final String csvFile = SystemUtilities.getCurrentWorkingDirectory() + "/Suites/gdp_full.csv";

    /**
     * To get the list of entitlement infos
     *
     * @return {@link OriginEntitlementHelper} of the specified type
     */
    public static List<OriginEntitlementHelper> getEntitlementInfos() {

        OriginEntitlementHelper entitlementInfo;
        // Read the csv file and all OriginEntitlement
        List<OriginEntitlementHelper> entitlementInfos = new ArrayList<>();

        try (CSVReader reader = new CSVReader(new InputStreamReader(new FileInputStream(csvFile), "UTF-8"))) {
            // Read (and ignore) the header row. Throw exception if file is empty
            if (reader.readNext() == null) {
                String errMessage = String.format("Empty csv file : '%s'", csvFile);
                System.err.println(errMessage);  // exception may not show in output, need to log first
                throw new RuntimeException(errMessage);
            }

            String[] line;
            while ((line = reader.readNext()) != null) {
                entitlementInfo = getEntitlementInfoFromCSVRow(line);
                entitlementInfos.add(entitlementInfo);
            }

        } catch (IOException e) {
            String errMessage = String.format("IO Error encountered reading csvfile %s: %s", csvFile, e.toString());
            System.err.println(errMessage);
            throw new RuntimeException(errMessage);
        }

        return entitlementInfos;
    }

    /**
     * @param line line of the CSV file as a String array
     * @return {@link OriginEntitlementHelper} constructed from a line of the CSV file
     */
    private static OriginEntitlementHelper getEntitlementInfoFromCSVRow(String[] line) {
        String entitlementOfferId = line[0].trim();
        String entitlementName = line[1].trim();
        String entitlementFamilyName = line[2].trim();
        String entitlementPartialPDPUrl = line[3].trim();
        String isBaseGameNeeded = line[4].trim();
        String baseGameRows = line[5].trim();
        String isBundle = line[6].trim();
        String bundleRows = line[7].trim();
        String gameLibraryImageName = line[8].trim();
        return new OriginEntitlementHelper(entitlementOfferId, entitlementName, entitlementFamilyName, entitlementPartialPDPUrl,
                isBaseGameNeeded, baseGameRows, isBundle, bundleRows, gameLibraryImageName);
    }

    /**
     * To get the list of entitlement infos a specific rowNumbers
     *
     * @param rowNumber specific row number to get the details
     * @return the entitlement details of the specific index number(rowNumber value)
     */
    public static OriginEntitlementHelper getEntitlementInfo(int rowNumber) {
        OriginEntitlementHelper entitlementInfo;

        try (CSVReader reader = new CSVReader(new InputStreamReader(new FileInputStream(csvFile), "UTF-8"))) {
            int i;
            if (reader.readNext() == null) {
                String errMessage = String.format("Empty csv file : '%s'", csvFile);
                System.err.println(errMessage);  // exception may not show in output, need to log first
                throw new RuntimeException(errMessage);
            }
            List<String[]> readers = reader.readAll();
            entitlementInfo = getEntitlementInfoFromCSVRow(readers.get(rowNumber));

        } catch (IOException e) {
            String errMessage = String.format("IO Error encountered reading csvfile %s: %s", csvFile, e.toString());
            System.err.println(errMessage);
            throw new RuntimeException(errMessage);
        }

        return entitlementInfo;

    }
}
