package com.ea.originx.automation.lib.resources.games;

import com.ea.vx.originclient.resources.OSInfo;

/**
 * Enum for entitlementIds - including fake and real entitlements for automation
 * tests
 */
public enum EntitlementId {

    /**
     * **********************************************
     * FAKE ENTITLEMENTS (please keep alphabetical)
     * **********************************************
     */
    DIP_SMALL(configure()
            .name(EntitlementInfoConstants.DIP_SMALL_NAME)
            .offerId(EntitlementInfoConstants.DIP_SMALL_OFFER_ID)
            .productCode(OSInfo.isProduction() ? EntitlementInfoConstants.DIP_SMALL_PRODUCT_CODE_PRODUCTION : EntitlementInfoConstants.DIP_SMALL_PRODUCT_CODE)
            .directoryName(EntitlementInfoConstants.DIP_SMALL_DIRECTORY_NAME)
            .processName(EntitlementInfoConstants.FAKE_ENTITLEMENT_PROCESS_NAME)
            .masterTitleId(EntitlementInfoConstants.DIP_SMALL_MASTER_TITLE_ID)
            .contentId(EntitlementInfoConstants.DIP_SMALL_CONTENT_ID)
            .multiplayerId(EntitlementInfoConstants.DIP_SMALL_MULTIPLAYER_ID)
            .downloadSize(EntitlementInfoConstants.DIP_SMALL_DOWNLOAD_SIZE)
            .installSize(EntitlementInfoConstants.DIP_SMALL_INSTALL_SIZE)
            .windowsGUIDCode(EntitlementInfoConstants.DIP_SMALL_WINDOWS_GUID_CODE)
    ),
    DIP_LARGE(configure()
            .name(EntitlementInfoConstants.DIP_LARGE_NAME)
            .offerId(EntitlementInfoConstants.DIP_LARGE_OFFER_ID)
            .productCode(OSInfo.isProduction() ? EntitlementInfoConstants.DIP_LARGE_PRODUCT_CODE_PRODUCTION : EntitlementInfoConstants.DIP_LARGE_PRODUCT_CODE)
            .processName(EntitlementInfoConstants.FAKE_ENTITLEMENT_PROCESS_NAME)
    ),
    BATTLEFIELD_TITANFALL_ULTIMATE_BUNDLE(configure()
            .name(EntitlementInfoConstants.BATTLEFIELD_TITANFALL_ULTIMATE_BUNDLE_NAME)
            .parentName(EntitlementInfoConstants.BATTLEFIELD_TITANFALL_ULTIMATE_BUNDLE_PARENT_NAME)
            .offerId(EntitlementInfoConstants.BATTLEFIELD_TITANFALL_ULTIMATE_BUNDLE_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.BATTLEFIELD_TITANFALL_ULTIMATE_BUNDLE_PARTIAL_PDP_URL)
    ),
    GDP_PRE_RELEASE_TEST_OFFER(configure()
            .name(EntitlementInfoConstants.GDP_PRE_RELEASE_TEST_NAME)
            .parentName(EntitlementInfoConstants.GDP_PRE_RELEASE_TEST_PARENT_NAME)
            .partialPdpUrl(EntitlementInfoConstants.GDP_PRE_RELEASE_TEST_PARTIAL_PDP_URL)
    ),
    GDP_PREMIER_RELEASE_SINGLE_EDITION(configure()
            .name(EntitlementInfoConstants.GDP_PREMIER_RELEASE_TEST_SINGLE_EDITION_PARENT_NAME)
            .parentName(EntitlementInfoConstants.GDP_PREMIER_RELEASE_TEST_SINGLE_EDITION_NAME)
            .offerId(EntitlementInfoConstants.GDP_PREMIER_RELEASE_TEST_SINGLE_EDITION_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.GDP_PREMIER_RELEASE_TEST_SINGLE_EDITION_PARTIAL_PDL_URL)
    ),
    GDP_PREMIER_RELEASE_MULTIPLE_EDITION(configure()
            .name(EntitlementInfoConstants.GDP_PREMIER_RELEASE_TEST_MULTIPLE_EDITION_NAME)
            .parentName(EntitlementInfoConstants.GDP_PREMIER_RELEASE_TEST_MULTIPLE_EDITION_PARENT_NAME)
            .offerId(EntitlementInfoConstants.GDP_PREMIER_RELEASE_TEST_MULTIPLE_EDITION_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.GDP_PREMIER_RELEASE_TEST_MULTIPLE_EDITION_OCD_PATH)
            .partialPdpUrl(EntitlementInfoConstants.GDP_PREMIER_RELEASE_TEST_MULTIPLE_EDITION_PARTIAL_PDL_URL)
    ),
    GDP_PREMIER_VAULT_ENTITLEMENT(configure()
            .name(EntitlementInfoConstants.GDP_PREMIER_VAULT_ENTITLEMENT_NAME)
            .parentName(EntitlementInfoConstants.GDP_PREMIER_VAULT_ENTITLEMENT_PARENT_NAME)
            .offerId(EntitlementInfoConstants.GDP_PREMIER_VAULT_ENTITLEMENT_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.GDP_PREMIER_VAULT_ENTITLEMENT_PARTIAL_PDP_URL)
    ),
    GDP_PRE_RELEASE_TEST_PLAY_FIRST_TRIAL(configure()
            .parentName(EntitlementInfoConstants.GDP_PRE_RELEASE_PLAY_FIRST_TRIAL_PARENT_NAME)
            .name(EntitlementInfoConstants.GDP_PRE_RELEASE_PLAY_FIRST_TRIAL_NAME)
            .partialPdpUrl(EntitlementInfoConstants.GDP_PRE_RELEASE_PLAY_FIRST_TRIAL_PARTIAL_PDL_URL)
    ),
    GDP_PRE_RELEASE_SINGLE_EDITION(configure()
            .parentName(EntitlementInfoConstants.GDP_PRE_RELEASE_TEST_SINGLE_EDITION_PARENT_NAME)
            .name(EntitlementInfoConstants.GDP_PRE_RELEASE_TEST_SINGLE_EDITION_NAME)
            .partialPdpUrl(EntitlementInfoConstants.GDP_PRE_RELEASE_TEST_SINGLE_EDITION_PARTIAL_PDL_URL)
            .ocdPath(EntitlementInfoConstants.GDP_PRE_RELEASE_TEST_SINGLE_EDITION_OCD_PATH)
    ),
    NON_DIP_SMALL(configure()
            .name(EntitlementInfoConstants.NON_DIP_SMALL_NAME)
            .offerId(EntitlementInfoConstants.NON_DIP_SMALL_OFFER_ID)
            .productCode(OSInfo.isProduction() ? EntitlementInfoConstants.NON_DIP_SMALL_PRODUCT_CODE_PRODUCTION : EntitlementInfoConstants.NON_DIP_SMALL_PRODUCT_CODE)
            .processName(EntitlementInfoConstants.FAKE_ENTITLEMENT_PROCESS_NAME)
            .downloadSize(EntitlementInfoConstants.NON_DIP_SMALL_DOWNLOAD_SIZE)
            .installSize(EntitlementInfoConstants.NON_DIP_SMALL_INSTALL_SIZE)
    ),
    NON_DIP_LARGE(configure()
            .name(EntitlementInfoConstants.NON_DIP_LARGE_NAME)
            .offerId(EntitlementInfoConstants.NON_DIP_LARGE_OFFER_ID)
            .productCode(OSInfo.isProduction() ? EntitlementInfoConstants.NON_DIP_LARGE_PRODUCT_CODE_PRODUCTION : EntitlementInfoConstants.NON_DIP_LARGE_PRODUCT_CODE)
            .processName(EntitlementInfoConstants.FAKE_ENTITLEMENT_PROCESS_NAME)
            .installSize(EntitlementInfoConstants.NON_DIP_LARGE_INSTALL_SIZE)
    ),
    PI(configure()
            .name(EntitlementInfoConstants.PI_NAME)
            .offerId(EntitlementInfoConstants.PI_OFFER_ID)
            .productCode(EntitlementInfoConstants.PI_PRODUCT_CODE)
            .directoryName(EntitlementInfoConstants.PI_DIRECTORY_NAME)
            .processName(EntitlementInfoConstants.FAKE_ENTITLEMENT_PROCESS_NAME)
    ),
    QA_DELUXE(configure()
            .name(EntitlementInfoConstants.QA_DELUXE_NAME)
            .offerId(EntitlementInfoConstants.QA_DELUXE_OFFER_ID)
    ),
    SDK_TOOL(configure()
            .name(EntitlementInfoConstants.SDK_TOOL_NAME)
            .offerId(EntitlementInfoConstants.SDK_TOOL_OFFER_ID)
            .productCode(EntitlementInfoConstants.SDK_TOOL_PRODUCT_CODE)
            .processName(EntitlementInfoConstants.SDK_TOOL_PROCESS_NAME)
            .masterTitleId(EntitlementInfoConstants.SDK_TOOL_MASTER_TITLE_ID)
            .contentId(EntitlementInfoConstants.SDK_TOOL_CONTENT_ID)
            .multiplayerId(EntitlementInfoConstants.SDK_TOOL_MULTIPLAYER_ID)
    ),
    BETA_DIP_SMALL(configure()
            .name(EntitlementInfoConstants.BETA_DIP_SMALL_NAME)
            .offerId(EntitlementInfoConstants.BETA_DIP_SMALL_OFFER_ID)
    ),
    GDP_PRE_RELEASE_TEST_OFFER01_STANDARD(configure()
            .parentName(EntitlementInfoConstants.GDP_PRE_RELEASE_TEST_OFFER01_STANDARD_PARENT_NAME)
            .name(EntitlementInfoConstants.GDP_PRE_RELEASE_TEST_OFFER01_STANDARD_NAME)
            .offerId(EntitlementInfoConstants.GDP_PRE_RELEASE_TEST_OFFER01_STANDARD_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.GDP_PRE_RELEASE_TEST_OFFER01_STANDARD_PARTIAL_PDL_URL)
    ),
    GPD_PREMIER_PRE_RELEASE_GAME01_STANDARD(configure()
            .parentName(EntitlementInfoConstants.GDP_PREMIER_PRE_RELEASE_GAME01_STANDARD_PARENT_NAME)
            .name(EntitlementInfoConstants.GDP_PREMIER_PRE_RELEASE_GAME01_STANDARD_NAME)
            .offerId(EntitlementInfoConstants.GDP_PREMIER_PRE_RELEASE_GAME01_STANDARD_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.GDP_PREMIER_PRE_RELEASE_GAME01_STANDARD_PARTIAL_GDP_URL)
    ),
    GDP_PREMIER_PRE_RELEASE_GAME02_STANDARD(configure()
            .parentName(EntitlementInfoConstants.GDP_PREMIER_PRE_RELEASE_GAME02_STANDARD_PARENT_NAME)
            .name(EntitlementInfoConstants.GDP_PREMIER_PRE_RELEASE_GAME02_STANDARD_NAME)
            .offerId(EntitlementInfoConstants.GDP_PREMIER_PRE_RELEASE_GAME02_STANDARD_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.GDP_PREMIER_PRE_RELEASE_GAME02_STANDARD_PARTIAL_GDP_URL)
    ),
    QA_OFFER_2_UNRELEASED_STANDARD(configure()
            .parentName(EntitlementInfoConstants.QA_OFFER_2_UNRELEASED_PARENT_NAME)
            .name(EntitlementInfoConstants.QA_OFFER_2_UNRELEASED_STANDARD_NAME)
            .offerId(EntitlementInfoConstants.QA_OFFER_2_UNRELEASED_STANDARD_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.QA_OFFER_2_UNRELEASED_PARTIAL_PDP_URL)
    ),
    QA_OFFER_2_UNRELEASED_DELUXE(configure()
            .parentName(EntitlementInfoConstants.QA_OFFER_2_UNRELEASED_PARENT_NAME)
            .name(EntitlementInfoConstants.QA_OFFER_2_UNRELEASED_STANDARD_NAME)
            .offerId(EntitlementInfoConstants.QA_OFFER_2_UNRELEASED_DELUXE_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.QA_OFFER_2_UNRELEASED_PARTIAL_PDP_URL)
    ),
    /**
     * ************************************************
     * REAL ENTITLEMENTS (please keep alphabetical)
     * ************************************************
     */
    ANTHEM(configure()
            .parentName(EntitlementInfoConstants.ANTHEM_PARENT_NAME)
            .name(EntitlementInfoConstants.ANTHEM_NAME)
            .partialPdpUrl(EntitlementInfoConstants.ANTHEM_PARTIAL_PDP_URL)
    ),
    ASSASSIN_CREED_SYNDICATE(configure()
            .name(EntitlementInfoConstants.ASSASSIN_CREED_SYNDICATE_NAME)
            .parentName(EntitlementInfoConstants.ASSASSIN_CREED_SYNDICATE_PARENT_NAME)
            .partialPdpUrl(EntitlementInfoConstants.ASSASSIN_CREED_SYNDICATE_PARTIAL_PDP_URL)
            .offerId(EntitlementInfoConstants.ASSASSIN_CREED_SYNDICATE_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.ASSASSIN_CREED_SYNDICATE_OCD_PATH)
    ),
    BATTLEFIELD_1_DELUXE_UPGRADE(configure()
            .parentName(EntitlementInfoConstants.BATTLEFIELD_1_PARENT_NAME)
            .name(EntitlementInfoConstants.BATTLEFIELD_1_DELUXE_UPDGRADE_NAME)
            .offerId(EntitlementInfoConstants.BATTLEFIELD_1_DELUXE_UPGRADE_OFFER_ID)
    ),
    BATTLEFIELD_1_STANDARD(configure()
            .parentName(EntitlementInfoConstants.BATTLEFIELD_1_PARENT_NAME)
            .name(EntitlementInfoConstants.BATTLEFIELD_1_STANDARD_NAME)
            .offerId(EntitlementInfoConstants.BATTLEFIELD_1_STANDARD_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.BATTLEFIELD_1_STANDARD_OCD_PATH)
            .processName(EntitlementInfoConstants.BATTLEFIELD_1_STANDARD_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.BATTLEFIELD_1_STANDARD_PARTIAL_URL)
    ),
    BATTLEFIELD_HARDLINE(configure()
            .name(EntitlementInfoConstants.BATTLEFIELD_HARDLINE_NAME)
            .parentName(EntitlementInfoConstants.BATTLEFIELD_HARDLINE_PARENT_NAME)
            .offerId(EntitlementInfoConstants.BATTLEFIELD_HARDLINE_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.BATTLEFIELD_HARDLINE_PARTIAL_URL)
    ),
    BATTLEFIELD_1_REVOLUTION(configure()
            .name(EntitlementInfoConstants.BATTLEFIELD_1_REVOLUTION_NAME)
    ),
    BEJEWELED3(configure()
            .parentName(EntitlementInfoConstants.BEJEWELED3_PARENT_NAME)
            .name(EntitlementInfoConstants.BEJEWELED3_NAME)
            .offerId(EntitlementInfoConstants.BEJEWELED3_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.BEJEWELED3_PARTIAL_URL)
            .processName(EntitlementInfoConstants.BEJEWELED3_PROCESS_NAME)
            .masterTitleId(EntitlementInfoConstants.BEJEWELED3_MASTERTITLE_ID)
            .localSavePath(EntitlementInfoConstants.BEJEWELED3_LOCAL_SAVE_PATH)
            .windowsGUIDCode(EntitlementInfoConstants.BEJEWELED3_WINDOWS_GUID_CODE)
            .directoryName(EntitlementInfoConstants.BEJEWELED3_DIRECTORY_NAME)
            .activationDirectoryName(EntitlementInfoConstants.BEJEWELED3_ACTIVATION_DIRECTORY_NAME)
    ),
    BF_BAD_COMPANY_2(configure()
            .parentName(EntitlementInfoConstants.BF_BAD_COMPANY_2_PARENT_NAME)
            .name(EntitlementInfoConstants.BF_BAD_COMPANY_2_NAME)
            .offerId(EntitlementInfoConstants.BF_BAD_COMPANY_2_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.BF_BAD_COMPANY_2_OCD_PATH)
            .partialPdpUrl(EntitlementInfoConstants.BF_BAD_COMPANY_2_PARTIAL_PDP_URL)
    ),
    BF_BAD_COMPANY_2_VIETNAM(configure()
            .name(EntitlementInfoConstants.BF_BAD_COMPANY_2_VIETNAM_NAME)
            .offerId(EntitlementInfoConstants.BF_BAD_COMPANY_2_VIETNAM_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.BF_BAD_COMPANY_2_VIETNAM_PARTIAL_PDP_URL)
    ),
    BF4_STANDARD(configure()
            .parentName(EntitlementInfoConstants.BATTLEFIELD_4_PARENT_NAME)
            .name(EntitlementInfoConstants.BF4_STANDARD_NAME)
            .offerId(EntitlementInfoConstants.BF4_STANDARD_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.BF4_STANDARD_OCD_PATH)
            .processName(EntitlementInfoConstants.BF4_STANDARD_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.BATTLEFIELD_4_PARTIAL_PDP_URL)
    ),
    BF4_DIGITAL_DELUXE(configure()
            .parentName(EntitlementInfoConstants.BATTLEFIELD_4_PARENT_NAME)
            .name(EntitlementInfoConstants.BF4_DIGITAL_DELUXE_NAME)
            .offerId(EntitlementInfoConstants.BF4_DIGITAL_DELUXE_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.BF4_DIGITAL_DELUXE_OCD_PATH)
            .processName(EntitlementInfoConstants.BF4_DIGITAL_DELUXE_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.BATTLEFIELD_4_PARTIAL_PDP_URL)
    ),
    BF4_PREMIUM(configure()
            .parentName(EntitlementInfoConstants.BATTLEFIELD_4_PARENT_NAME)
            .name(EntitlementInfoConstants.BF4_PREMIUM_EDITION_NAME)
            .offerId(EntitlementInfoConstants.BF4_PREMIUM_EDITION_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.BF4_PREMIUM_EDITION_OCD_PATH)
            .partialPdpUrl(EntitlementInfoConstants.BATTLEFIELD_4_PARTIAL_PDP_URL)
    ),
    BIG_MONEY(configure()
            .parentName(EntitlementInfoConstants.BIG_MONEY_NAME)
            .name(EntitlementInfoConstants.BIG_MONEY_NAME)
            .offerId(EntitlementInfoConstants.BIG_MONEY_OFFER_ID)
            .processName(EntitlementInfoConstants.BIG_MONEY_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.BIG_MONEY_PARTIAL_PDP_URL)
            .directoryName(EntitlementInfoConstants.BIG_MONEY_DIRECTORY_NAME)
            .activationDirectoryName(EntitlementInfoConstants.BIG_MONEY_ACTIVATION_DIRECTORY_NAME)
    ),
    CRYSIS3(configure()
            .name(EntitlementInfoConstants.CRYSIS3_NAME)
            .offerId(EntitlementInfoConstants.CRYSIS3_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.CRYSIS3_PARTIAL_PDP_URL)
    ),
    CRYSIS3_HUNTER(configure()
            .name(EntitlementInfoConstants.CRYSIS3_HUNTER_EDITION_NAME)
            .offerId(EntitlementInfoConstants.CRYSIS3_HUNTER_EDITION_OFFER_ID)
    ),
    DA_2(configure()
            .name(EntitlementInfoConstants.DA_2_NAME)
            .offerId(EntitlementInfoConstants.DA_2_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.DA_2_PARTIAL_PDP_URL)
            .processName(EntitlementInfoConstants.DA_2_PROCESS_NAME)),
    DA_INQUISITION(configure()
            .parentName(EntitlementInfoConstants.DRAGONAGE_INQUISITION_PARENT_NAME)
            .name(EntitlementInfoConstants.DRAGONAGE_INQUISITION_STANDARD_NAME)
            .offerId(EntitlementInfoConstants.DRAGONAGE_INQUISITION_STANDARD_OFFER_ID)
            .processName(EntitlementInfoConstants.DRAGONAGE_INQUISITION_STANDARD_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.DRAGONAGE_INQUISITION_STANDARD_PARTIAL_URL)
            .directoryName(EntitlementInfoConstants.DRAGONAGE_INQUISITION_STANDARD_DIRECTORY_NAME)
            .activationDirectoryName(EntitlementInfoConstants.DRAGONAGE_INQUISITION_STANDARD_ACTIVATION_DIRECTORY_NAME)
    ),
    DA_INQUISITION_DIGITAL_DELUXE(configure()
            .name(EntitlementInfoConstants.DRAGONAGE_INQUISITION_DIGITAL_DELUXE_NAME)
            .offerId(EntitlementInfoConstants.DRAGONAGE_INQUISTION_DIGITAL_DELUXE_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.DRAGONAGE_INQUISITION_DIGITAL_DELUXE_PARTIAL_URL)
            .directoryName(EntitlementInfoConstants.DRAGONAGE_INQUISITION_DIGITAL_DELUXE_DIRECTORY_NAME)
            .activationDirectoryName(EntitlementInfoConstants.DRAGONAGE_INQUISITION_DIGITAL_DELUXE_ACTIVATION_DIRECTORY_NAME)
    ),
    DA_INQUISITION_GAME_OF_THE_YEAR(configure()
            .name(EntitlementInfoConstants.DRAGONAGE_INQUISITION_GAME_OF_THE_YEAR_NAME)
            .offerId(EntitlementInfoConstants.DRAGONAGE_INQUISTION_GAME_OF_THE_YEAR_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.DRAGONAGE_INQUISITION_GAME_OF_THE_YEAR_PARTIAL_URL)
            .directoryName(EntitlementInfoConstants.DRAGONAGE_INQUISITION_GAME_OF_THE_YEAR_DIRECTORY_NAME)
            .activationDirectoryName(EntitlementInfoConstants.DRAGONAGE_INQUISITION_GAME_OF_THE_YEAR_ACTIVATION_DIRECTORY_NAME)
    ),
    FIFA_15(configure()
            .name(EntitlementInfoConstants.FIFA_15)
            .offerId(EntitlementInfoConstants.FIFA_15_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.FIFA_15_PARTIAL_PDP_URL)
    ),
    FIFA_17(configure()
            .parentName(EntitlementInfoConstants.FIFA_17_PARENT_NAME)
            .name(EntitlementInfoConstants.FIFA_17_STANDARD_NAME)
            .offerId(EntitlementInfoConstants.FIFA_17_STANDARD_OFFER_ID)
            .processName(EntitlementInfoConstants.FIFA_17_STANDARD_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.FIFA_17_STANDARD_PARTIAL_PDP_URL)
    ),
    FIFA_17_DELUXE(configure()
            .name(EntitlementInfoConstants.FIFA_17_DELUXE_NAME)
            .offerId(EntitlementInfoConstants.FIFA_17_DELUXE_OFFER_ID)
            .processName(EntitlementInfoConstants.FIFA_17_DELUXE_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.FIFA_17_DELUXE_PARTIAL_PDP_URL)
    ),
    FIFA_18(configure()
            .parentName(EntitlementInfoConstants.FIFA_18_PARENT_NAME)
            .name(EntitlementInfoConstants.FIFA_18_NAME)
            .offerId(EntitlementInfoConstants.FIFA_18_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.FIFA_18_OCD_PATH)
            .partialPdpUrl(EntitlementInfoConstants.FIFA_18_PARTIAL_PDP_URL)
    ),
    FIFA_18_ICON(configure()
            .parentName(EntitlementInfoConstants.FIFA_18_PARENT_NAME)
            .name(EntitlementInfoConstants.FIFA_18_ICON_NAME)
            .offerId(EntitlementInfoConstants.FIFA_18_ICON_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.FIFA_18_ICON_OCD_PATH)
            .partialPdpUrl(EntitlementInfoConstants.FIFA_18_ICON_PARTIAL_PDP_URL)
    ),
    FIFA_18_RONALDO(configure()
            .parentName(EntitlementInfoConstants.FIFA_18_PARENT_NAME)
            .name(EntitlementInfoConstants.FIFA_18_RONALDO_NAME)
            .offerId(EntitlementInfoConstants.FIFA_18_RONALDO_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.FIFA_18_RONALDO_OCD_PATH)
            .partialPdpUrl(EntitlementInfoConstants.FIFA_18_RONALDO_PARTIAL_PDP_URL)
    ),
    FE(configure()
            .parentName(EntitlementInfoConstants.FE_PARENT_NAME)
            .name(EntitlementInfoConstants.FE_NAME)
            .partialPdpUrl(EntitlementInfoConstants.FE_PARTIAL_PDP_URL)
            .processName(EntitlementInfoConstants.FE_PROCESS_NAME)
            .offerId(EntitlementInfoConstants.FE_OFFER_ID)
    ),
    MASS_EFFECT_TRILOGY(configure()
            .parentName(EntitlementInfoConstants.MASS_EFFECT_TRILOGY_PARENT_NAME)
            .name(EntitlementInfoConstants.MASS_EFFECT_TRILOGY_NAME)
            .offerId(EntitlementInfoConstants.MASS_EFFECT_TRILOGY_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.MASS_EFFECT_TRILOGY_PARTIAL_PDP_URL)),
    ME3(configure()
            .name(EntitlementInfoConstants.ME3_STANDARD_NAME)
            .parentName(EntitlementInfoConstants.ME3_STANDARD_PARENT_NAME)
            .offerId(EntitlementInfoConstants.ME3_STANDARD_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.ME3_STANDARD_OCD_PATH)
            .productCode(EntitlementInfoConstants.ME3_PRODUCT_CODE)
            .partialPdpUrl(EntitlementInfoConstants.ME3_STANDARD_PARTIAL_PDP_URL)
            .directoryName(EntitlementInfoConstants.ME3_DIRECTORY_NAME)
            .activationDirectoryName(EntitlementInfoConstants.ME3_ACTIVATION_DIRECTORY_NAME)
    ),
    ME3_DELUXE(configure()
            .name(EntitlementInfoConstants.ME3_DELUXE_NAME)
            .offerId(EntitlementInfoConstants.ME3_DELUXE_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.ME3_DELUXE_PARTIAL_PDP_URL)
    ),
    PEGGLE(configure()
            .name(EntitlementInfoConstants.PEGGLE_NAME)
            .parentName(EntitlementInfoConstants.PEGGLE_PARENT_NAME)
            .offerId(EntitlementInfoConstants.PEGGLE_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.PEGGLE_PARTIAL_PDP_URL)
            .directoryName(EntitlementInfoConstants.PEGGLE_DIRECTORY_NAME)
            .activationDirectoryName(EntitlementInfoConstants.PEGGLE_ACTIVATION_DIRECTORY_NAME)
            .processName(EntitlementInfoConstants.PEGGLE_PROCESS_NAME)
    ),
    POPCAP_HERO_BUNDLE(configure()
            .name(EntitlementInfoConstants.POPCAP_HERO_BUNDLE_NAME)
            .offerId(EntitlementInfoConstants.POPCAP_HERO_BUNDLE_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.POPCAP_HERO_BUNDLE_PARITAL_URL)
    ),
    PVZ_GW_2_STANDARD_EDITION(configure()
            .name(EntitlementInfoConstants.PVZ_GW_2_STANDARD_EDITION_NAME)
            .offerId(EntitlementInfoConstants.PVZ_GW_2_STANDARD_EDITION_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.PVZ_GW_2_STANDARD_EDITION_PARTIAL_PDP_URL)
    ),
    PVZ_GW_2_DELUXE_EDITION(configure()
            .name(EntitlementInfoConstants.PVZ_GW_2_DELUXE_EDITION_NAME)
            .parentName(EntitlementInfoConstants.PVZ_GW_2_DELUXE_EDITION_PARENT_NAME)
            .offerId(EntitlementInfoConstants.PVZ_GW_2_DELUXE_EDITION_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.PVZ_GW_2_DELUXE_EDITION_OCD_PATH)
            .partialPdpUrl(EntitlementInfoConstants.PVZ_GW_2_DELUXE_EDITION_PARTIAL_PDP_URL)
    ),
    PVZ2_56000_COIN_PACK(configure()
            .parentName(EntitlementInfoConstants.PVZ2_COIN_PACK_FAMILY_NAME)
            .name(EntitlementInfoConstants.PVZ2_56000_COIN_PACK_NAME)
            .offerId(EntitlementInfoConstants.PVZ2_56000_COINS_PACK_OFFERID)
            .partialPdpUrl(EntitlementInfoConstants.PVZ2_56000_COINS_PACK_PARTIAL_PDP_URL)
    ),
    PVZ2_63000_COIN_PACK(configure()
            .parentName(EntitlementInfoConstants.PVZ2_COIN_PACK_FAMILY_NAME)
            .name(EntitlementInfoConstants.PVZ2_63000_COIN_PACK_NAME)
            .offerId(EntitlementInfoConstants.PVZ2_63000_COINS_PACK_OFFERID)
            .partialPdpUrl(EntitlementInfoConstants.PVZ2_63000_COINS_PACK_PARTIAL_PDP_URL)
    ),
    SIM_CITY_COMPLETE(configure()
            .name(EntitlementInfoConstants.SIM_CITY_COMPLETE_NAME)
            .offerId(EntitlementInfoConstants.SIM_CITY_COMPLETE_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.SIM_CITY_COMPLETE_PARTIAL_PDP_URL)
    ),
    SIM_CITY_STANDARD(configure()
            .name(EntitlementInfoConstants.SIM_CITY_STANDARD_NAME)
            .offerId(EntitlementInfoConstants.SIM_CITY_STANDARD_OFFER_ID)
            .processName(EntitlementInfoConstants.SIM_CITY_STANDARD_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.SIM_CITY_STANDARD_PARTIAL_PDP_URL)
    ),
    SIMS3(configure()
            .name(EntitlementInfoConstants.SIMS3_NAME)
            .offerId(EntitlementInfoConstants.SIMS3_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.SIMS3_PARTIAL_PDP_URL)
    ),
    SIMS3_DLC_LATE_NIGHT(configure()
            .name(EntitlementInfoConstants.SIMS3_DLC_LATE_NIGHT_NAME)
            .offerId(EntitlementInfoConstants.SIMS3_DLC_LATE_NIGHT_OFFER_ID)
    ),
    SIMS3_STARTER_PACK(configure()
            .name(EntitlementInfoConstants.SIMS3_STARTER_NAME)
            .parentName(EntitlementInfoConstants.SIMS3_STARTER_PARENT_NAME)
            .offerId(EntitlementInfoConstants.SIMS3_STARTER_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.SIMS3_STARTER_PARTIAL_URL)
    ),
    SIMS4_DIGITAL_DELUXE(configure()
            .name(EntitlementInfoConstants.SIMS4_DDX_NAME)
            .offerId(EntitlementInfoConstants.SIMS4_DDX_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.SIMS4_DDX_OCD_PATH)
            .processName(EntitlementInfoConstants.SIMS4_DDX_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.SIMS4_DDX_PARTIAL_PDP_URL)
            .directoryName(EntitlementInfoConstants.SIMS4_DDX_DIRECTORY_NAME)
            .activationDirectoryName(EntitlementInfoConstants.SIMS4_DDX_ACTIVATION_DIRECTORY_NAME)
    ),
    SIMS4_STANDARD(configure()
            .name(EntitlementInfoConstants.SIMS4_STANDARD_NAME)
            .parentName(EntitlementInfoConstants.SIMS4_STANDARD_PARENT_NAME)
            .offerId(EntitlementInfoConstants.SIMS4_STANDARD_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.SIMS4_STANDARD_OCD_PATH)
            .processName(EntitlementInfoConstants.SIMS4_STANDARD_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.SIMS4_STANDARD_PARTIAL_PDP_URL)
            .directoryName(EntitlementInfoConstants.SIMS4_DIRECTORY_NAME)
            .activationDirectoryName(EntitlementInfoConstants.SIMS4_ACTIVATION_DIRECTORY_NAME)
    ),
    SIMS4_DLC_DIGITAL_SOUNDTRACK(configure()
            .name(EntitlementInfoConstants.SIMS4_DLC_DIGITAL_SOUNDTRACK_NAME)
            .offerId(EntitlementInfoConstants.SIMS4_DLC_DIGITAL_SOUNDTRACK_OFFER_ID)
    ),
    SIMS4_MY_FIRST_PET(configure()
            .name(EntitlementInfoConstants.SIMS4_MY_FIRST_PET_NAME)
            .parentName(EntitlementInfoConstants.SIMS4_MY_FIRST_PET_PARENT_NAME)
            .partialPdpUrl(EntitlementInfoConstants.SIMS4_MY_FIRST_PET_PARTIAL_PDP_URL)
    ),
    SIMS4_CATS_AND_DOGS(configure()
            .name(EntitlementInfoConstants.SIMS4_CATS_AND_DOGS_NAME)
            .parentName(EntitlementInfoConstants.SIMS4_CATS_AND_DOGS_PARENT_NAME)
            .partialPdpUrl(EntitlementInfoConstants.SIMS4_CATS_AND_DOGS_PARTIAL_PDP_URL)
    ),
    STAR_WARS_BATTLEFRONT(configure()
            .parentName(EntitlementInfoConstants.STAR_WARS_BATTLEFRONT_PARENT_NAME)
            .name(EntitlementInfoConstants.STAR_WARS_BATTLEFRONT_NAME)
            .offerId(EntitlementInfoConstants.STAR_WARS_BATTLEFRONT_OFFER_ID)
            .processName(EntitlementInfoConstants.STAR_WARS_BATTLEFRONT_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.STAR_WARS_BATTLEFRONT_PARTIAL_PDP_URL)
    ),
    STAR_WARS_BATTLEFRONT_2(configure()
            .parentName(EntitlementInfoConstants.STAR_WARS_BATTLEFRONT_2_PARENT_NAME)
            .name(EntitlementInfoConstants.STAR_WARS_BATTLEFRONT_2_NAME)
            .offerId(EntitlementInfoConstants.STAR_WARS_BATTLEFRONT_2_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.STAR_WARS_BATTLEFRONT_2_PARTIAL_PDP_URL)
    ),
    THE_LITTLE_ONES(configure()
            .name(EntitlementInfoConstants.THE_LITTLE_ONES_NAME)
            .offerId(EntitlementInfoConstants.THE_LITTLE_ONES_OFFER_ID)
    ),
    THIS_WAR_OF_MINE(configure()
            .parentName(EntitlementInfoConstants.THIS_WAR_OF_MINE_FAMILY_NAME)
            .name(EntitlementInfoConstants.THIS_WAR_OF_MINE_NAME)
            .offerId(EntitlementInfoConstants.THIS_WAR_OF_MINE_OFFER_ID)
            .processName(EntitlementInfoConstants.THIS_WAR_OF_MINE_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.THIS_WAR_OF_MINE_PARTIAL_PDP_URL)
    ),
    UNRAVEL(configure()
            .parentName(EntitlementInfoConstants.UNRAVEL_PARENT_NAME)
            .name(EntitlementInfoConstants.UNRAVEL_NAME)
            .offerId(EntitlementInfoConstants.UNRAVEL_OFFER_ID)
            .processName(EntitlementInfoConstants.UNRAVEL_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.UNRAVEL_PARTIAL_PDP_URL)
    ),
    WATCH_DOGS(configure()
            .name(EntitlementInfoConstants.WATCH_DOGS_NAME)
            .parentName(EntitlementInfoConstants.WATCH_DOGS_PARENT_NAME)
            .offerId(EntitlementInfoConstants.WATCH_DOGS_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.WATCH_DOGS_PARTIAL_PDP_URL)
    ),
    WATCH_DOGS_SEASON_PASS(configure()
            .name(EntitlementInfoConstants.WATCH_DOGS_SEASON_PASS_NAME)
            .offerId(EntitlementInfoConstants.WATCH_DOGS_SEASON_PASS_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.WATCH_DOGS_SEASON_PASS_PARTIAL_PDP_URL)
    ),
    BLACK_MIRROR(configure()
            .parentName(EntitlementInfoConstants.BLACK_MIRROR_PARENT_NAME)
            .name(EntitlementInfoConstants.BLACK_MIRROR_NAME)
            .offerId(EntitlementInfoConstants.BLACK_MIRROR_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.BLACK_MIRROR_PARTIAL_PDP_URL)
    ),
    COMMAND_AND_CONQUER_TIBERIUM_ALLIANCES(configure()
            .parentName(EntitlementInfoConstants.COMMAND_AND_CONQUER_TIBERIUM_ALLIANCES_PARENT_NAME)
            .name(EntitlementInfoConstants.COMMAND_AND_CONQUER_TIBERIUM_ALLIANCES_NAME)
            .offerId(EntitlementInfoConstants.COMMAND_AND_CONQUER_TIBERIUM_ALLIANCES_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.COMMAND_AND_CONQUER_TIBERIUM_ALLIANCES_PARTIAL_PDP_URL)
    ),
    MINI_METRO(configure()
            .parentName(EntitlementInfoConstants.MINI_METRO_PARENT_NAME)
            .name(EntitlementInfoConstants.MINI_METRO_NAME)
            .offerId(EntitlementInfoConstants.MINI_METRO_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.MINI_METRO_PARTIAL_PDP_URL)
    ),
    SUNDERED_STANDARD_EDITION(configure()
            .parentName(EntitlementInfoConstants.SUNDERED_PARENT_NAME)
            .name(EntitlementInfoConstants.SUNDERED_STANDARD_EDITION_NAME)
            .offerId(EntitlementInfoConstants.SUNDERED_STANDARD_EDITION_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.SUNDERED_PARTIAL_PDP_URL)
    ),
    SUPER_DUPER_SIMS_DISCOUNTED(configure()
            .parentName(EntitlementInfoConstants.SUPER_DUPER_SIMS_PARENT_NAME)
            .name(EntitlementInfoConstants.SUPER_DUPER_SIMS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.SUPER_DUPER_SIMS_PDP_URL)
    ),
    DEAD_SPACE(configure()
            .parentName(EntitlementInfoConstants.DEAD_SPACE_PARENT_NAME)
            .name(EntitlementInfoConstants.DEAD_SPACE_NAME)
            .offerId(EntitlementInfoConstants.DEAD_SPACE_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.DEAD_SPACE_PARTIAL_PDP_URL)
    ),
    TITANFALL_2(configure()
            .parentName(EntitlementInfoConstants.TITANFALL_2_PARENT_NAME)
            .name(EntitlementInfoConstants.TITANFALL_2_NAME)
            .offerId(EntitlementInfoConstants.TITANFALL_2_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.TITANFALL_2_OCD_PATH)
            .partialPdpUrl(EntitlementInfoConstants.TITANFALL_2_PARTIAL_PDP_URL)
    ),
    TITANFALL_2_ULTIMATE_EDITION(configure()
            .parentName(EntitlementInfoConstants.TITANFALL_2_ULTIMATE_EDITION_PARENT_NAME)
            .name(EntitlementInfoConstants.TITANFALL_2_ULTIMATE_EDITION_NAME)
            .offerId(EntitlementInfoConstants.TITANFALL_2_ULTIMATE_EDITON_OFFER_ID)
            .ocdPath(EntitlementInfoConstants.TITANFALL_2_ULTIMATE_EDITION_OCD_PATH)
            .partialPdpUrl(EntitlementInfoConstants.TITANFALL_2_ULTIMATE_EDITION_PARTIAL_PDP_URL)
    ),
    TITANFALL_2_DELUXE_EDITION(configure()
            .parentName(EntitlementInfoConstants.TITANFALL_2_DELUXE_EDITION_PARENT_NAME)
            .name(EntitlementInfoConstants.TITANFALL_2_DELUXE_EDITION_NAME)
            .offerId(EntitlementInfoConstants.TITANFALL_2_DELUXE_EDITON_OFFER_ID)
    ),
    FIFA_18_CURRENCY_PACK(configure()
            .parentName(EntitlementInfoConstants.FIFA_18_POINTS_PACK_PARENT_NAME)
            .name(EntitlementInfoConstants.FIFA_18_POINTS_PACK_NAME)
            .offerId(EntitlementInfoConstants.FIFA_18_POINTS_PACK_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.FIFA_18_POINTS_PACK_PDP_URL)
    ),
    DEAD_IN_BERMUDA(configure()
            .parentName(EntitlementInfoConstants.DEAD_IN_BERMUDA_PARENT_NAME)
            .name(EntitlementInfoConstants.DEAD_IN_BERMUDA_NAME)
            .offerId(EntitlementInfoConstants.DEAD_IN_BERMUDA_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.DEAD_IN_BERMUDA_PARTIAL_PDP_URL)
    ),
    NEED_FOR_SPEED_PAYBACK(configure()
            .parentName(EntitlementInfoConstants.NEED_FOR_SPEED_PAYBACK_PARENT_NAME)
            .name(EntitlementInfoConstants.NEED_FOR_SPEED_PAYBACK_NAME)
            .offerId(EntitlementInfoConstants.NEED_FOR_SPEED_PAYBACK_OFFER_ID)
            .partialPdpUrl(EntitlementInfoConstants.NEED_FOR_SPEED_PAYBACK_PARTIAL_PDP_URL)
            .ocdPath(EntitlementInfoConstants.NEED_FOR_SPEED_PAYBACK_OCD_PATH)
    ),
    CLUB_POGO_3MONTH_MEMBERSHIP(configure()
            .parentName(EntitlementInfoConstants.CLUB_POGO_3MONTH_MEMBERSHIP_PARENT_NAME)
            .name(EntitlementInfoConstants.CLUB_POGO_3MONTH_MEMBERSHIP_NAME)
            .partialPdpUrl(EntitlementInfoConstants.CLUB_POGO_3MONTH_MEMBERSHIP_PARTIAL_PDP_URL)
    ),
    Orwell(configure()
            .parentName(EntitlementInfoConstants.ORWELL_NAME)
            .offerId(EntitlementInfoConstants.ORWELL_OFFER_ID)
            .processName(EntitlementInfoConstants.ORWELL_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.ORWELL_PARTIAL_PDP_URL)
    ),
    Limbo(configure()
            .parentName(EntitlementInfoConstants.LIMBO_PARENT_NAME)
            .offerId(EntitlementInfoConstants.LIMBO_OFFER_ID)
            .processName(EntitlementInfoConstants.LIMBO_PROCESS_NAME)
            .partialPdpUrl(EntitlementInfoConstants.LIMBO_PARTIAL_GDP_URL)
    );

    /**
     * Builder object that is used to build the EntitlementId enum with the
     * specified configuration and store the data for later retrieval.
     */
    private final Builder _config;

    EntitlementId(Builder config) {
        _config = config;
    }

    /**
     * Used to start a build chain for the EntitlementId.
     *
     * @return {@link Builder}
     */
    private static Builder configure() {
        return new Builder();
    }

    /**
     * Get entitlement parent name
     *
     * @return entitlement parent name
     */
    public String getParentName() {
        return _config._parentName;
    }

    /**
     * Get entitlement name
     *
     * @return entitlement name
     */
    public String getName() {
        return _config._name;
    }

    /**
     * Get entitlement offer id
     *
     * @return entitlement offer id, or empty string if not (yet) defined
     */
    public String getOfferId() {
        return _config._offerId;
    }

    /**
     * Get entitlement ocd Path
     *
     * @return entitlement ocd Path, or empty string if not (yet) defined
     */
    public String getOcdPath() {
        return _config._ocdPath;
    }

    /**
     * Get entitlement product code
     *
     * @return entitlement product code, or empty string if not (yet) defined
     */
    public String getProductCode() {
        return _config._productCode;
    }

    /**
     * Get entitlement directory name
     *
     * @return entitlement directory name (for installation and log locations),
     * or empty string if not (yet) defined
     */
    public String getDirectoryName() {
        return _config._directoryName;
    }

    /**
     * Get activation directory name
     *
     * @return activation directory name
     */
    public String getActivationDirectoryName() { return _config._activationDirectoryName; }

    /**
     * Get entitlement process name
     *
     * @return entitlement process name (e.g. target.exe for fake entitlements),
     * or empty string if not (yet) defined
     */
    public String getProcessName() {
        return _config._processName;
    }

    /**
     * Get entitlement download size
     *
     * @return entitlement download size as a string, or empty string if not
     * (yet) defined
     */
    public String getDownloadSize() {
        return _config._downloadSize;
    }

    /**
     * Get entitlement install size
     *
     * @return entitlement install size as a string, or empty string if not
     * (yet) defined
     */
    public String getInstallSize() {
        return _config._installSize;
    }

    /**
     * @return the entitlements partial pdp url
     */
    public String getPartialPdpUrl() {
        return _config._partialPdpUrl;
    }

    /**
     * @return the mastertitle Id
     */
    public String getMasterTitleId() {
        return _config._masterTitleId;
    }

    /**
     * @return the content id
     */
    public String getContentId() {
        return _config._contentId;
    }

    /**
     * @return the multi-player id
     */
    public String getMultiplayerId() {
        return _config._multiplayerId;
    }

    /**
     * @return local save path for entitlement
     */
    public String getLocalSavePath() {
        return _config._localSavePath;
    }

    /**
     * @return the entitlements windows GUID code
     */
    public String getWindowsGUIDCode() {
        return _config._windowsProductCode;
    }

    /**
     * Used to create a build chain when building entitlement info and also used
     * to store the entitlement data for an EntitlementInfo instance.
     */
    private static class Builder {

        private String _parentName = "";
        private String _name = "";
        private String _offerId = "";
        private String _ocdPath = "";
        private String _productCode = "";
        private String _directoryName = ""; // game or log directory
        private String _activationDirectoryName = "";
        private String _processName = "";
        private String _downloadSize = "";
        private String _installSize = "";
        private String _partialPdpUrl = ""; //partial store pdp url
        private String _masterTitleId = "";
        private String _contentId = "";
        private String _multiplayerId = "";
        private String _localSavePath = ""; // path for local save files
        private String _windowsProductCode = "";

        /**
         * Set the parent Name for this entitlement.
         *
         * @param parentName parent Name for the entitlement
         * @return {@code this} {@link Builder} object to continue the build
         * chain
         */
        public Builder parentName(String parentName) {
            _parentName = parentName;
            return this;
        }

        /**
         * Set the name for this entitlement.
         *
         * @param name name for the entitlement
         * @return {@code this} {@link Builder} object to continue the build
         * chain
         */
        public Builder name(String name) {
            _name = name;
            return this;
        }

        /**
         * Set the offerId for this entitlement.
         *
         * @param offerId offerId for the entitlement
         * @return {@code this} {@link Builder} object to continue the build
         * chain
         */
        public Builder offerId(String offerId) {
            _offerId = offerId;
            return this;
        }

        /**
         * Set the ocdPath for this entitlement.
         *
         * @param ocdPath ocdPath for the entitlement
         * @return {@code this} {@link Builder} object to continue the build
         * chain
         */
        public Builder ocdPath(String ocdPath) {
            _ocdPath = ocdPath;
            return this;
        }

        /**
         * Set the product code for this entitlement.
         *
         * @param productCode product code for the entitlement
         * @return {@code this} {@link Builder} object to continue the build
         * chain
         */
        public Builder productCode(String productCode) {
            _productCode = productCode;
            return this;
        }

        /**
         * Set the directory name for this entitlement.
         *
         * @param directoryName directory name for the entitlement
         * @return {@code this} {@link Builder} object to continue the build
         * chain
         */
        public Builder directoryName(String directoryName) {
            _directoryName = directoryName;
            return this;
        }

        /**
         * Set the directory name of activation for this entitlement.
         *
         */
        public Builder activationDirectoryName(String activationDirectoryName) {
            _activationDirectoryName = activationDirectoryName;
            return this;
        }

        /**
         * Set the process name for this entitlement.
         *
         * @param processName process name for the entitlement
         * @return {@code this} {@link Builder} object to continue the build
         * chain
         */
        public Builder processName(String processName) {
            _processName = processName;
            return this;
        }

        /**
         * Set the download size for this entitlement.
         *
         * @param downloadSize download size for the entitlement
         * @return {@code this} {@link Builder} object to continue the build
         * chain
         */
        public Builder downloadSize(String downloadSize) {
            _downloadSize = downloadSize;
            return this;
        }

        /**
         * Set the install size for this entitlement.
         *
         * @param installSize install size for the entitlement
         * @return {@code this} {@link Builder} object to continue the build
         * chain
         */
        public Builder installSize(String installSize) {
            _installSize = installSize;
            return this;
        }

        /**
         * Set the partial PDP URL for this entitlement.
         *
         * @param partialPdpUrl partial PDP URL for the entitlement
         * @return {@code this} {@link Builder} object to continue the build
         * chain
         */
        public Builder partialPdpUrl(String partialPdpUrl) {
            _partialPdpUrl = partialPdpUrl;
            return this;
        }

        /**
         * Set the process name for this entitlement.
         *
         * @param masterTitleId masterTitle Id for the entitlement
         * @return {@code this} {@link Builder} object to continue the build
         * chain
         */
        public Builder masterTitleId(String masterTitleId) {
            _masterTitleId = masterTitleId;
            return this;
        }

        /**
         * Set content id for this entitlement
         *
         * @param contentId String content Id for the entitlement
         * @return
         */
        public Builder contentId(String contentId) {
            _contentId = contentId;
            return this;
        }

        /**
         * Set multiplayer id for this entitlement
         *
         * @param multiplayerId String multiplayer Id for the entitlement
         * @return
         */
        public Builder multiplayerId(String multiplayerId) {
            _multiplayerId = multiplayerId;
            return this;
        }

        /**
         * Set the local save path for this entitlement.
         *
         * @param localSavePath local save path for the entitlement
         * @return {@code this} {@link Builder} object to continue the build
         * chain
         */
        public Builder localSavePath(String localSavePath) {
            _localSavePath = localSavePath;
            return this;
        }

        /**
         * Set the Windows GUID code for this entitlement.
         *
         * @param windowsGUIDCode windows GUID code for the entitlement
         * @return {@code this} {@link Builder} object to continue the build
         * chain
         */
        public Builder windowsGUIDCode(String windowsGUIDCode) {
            _windowsProductCode = windowsGUIDCode;
            return this;
        }
    }
}
