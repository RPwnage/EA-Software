/**************************************************************************************************/
/*! 
    \file locales.h

    \note
        This file is based on lobbylang.h from DirtySDK.
        2-letter language codes are currently available here:
        http://en.wikipedia.org/wiki/ISO_639
        2-letter country codes are currently available here:
        http://en.wikipedia.org/wiki/ISO_3166

        'Locale' should only refer to information needed for localization.
        The 2-letter country code part of a locale is considered a language variant and
        should not be confused with a user's country setting in their account.

    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef LOCALES_H
#define LOCALES_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmultichar"
#endif

namespace Blaze
{

//! Commonly used locale values
const uint32_t LOCALE_UNKNOWN               = 'zzZZ';
const uint32_t LOCALE_EN_US                 = 'enUS';
const uint32_t LOCALE_BLANK                 = '\0\0\0\0';
const uint32_t LOCALE_WILDCARD              = '****';

const uint32_t LOCALE_DEFAULT               = LOCALE_EN_US;
const char8_t* const LOCALE_DEFAULT_STR     = "enUS";

//! Non-specific, commonly used country and language codes
const uint16_t LANGUAGE_UNKNOWN             = 'zz';
const uint16_t COUNTRY_UNKNOWN              = 'ZZ';
const uint16_t LANGUAGE_WILDCARD            = '**';
const uint16_t COUNTRY_WILDCARD             = '**';

//! Languages
const uint16_t LANGUAGE_AFAN_OROMO          = 'om';
const uint16_t LANGUAGE_ABKHAZIAN           = 'ab';
const uint16_t LANGUAGE_AFAR                = 'aa';
const uint16_t LANGUAGE_AFRIKAANS           = 'af';
const uint16_t LANGUAGE_ALBANIAN            = 'sq';
const uint16_t LANGUAGE_AMHARIC             = 'am';
const uint16_t LANGUAGE_ARABIC              = 'ar';
const uint16_t LANGUAGE_ARMENIAN            = 'hy';
const uint16_t LANGUAGE_ASSAMESE            = 'as';
const uint16_t LANGUAGE_AYMARA              = 'ay';
const uint16_t LANGUAGE_AZERBAIJANI         = 'az';
const uint16_t LANGUAGE_BASHKIR             = 'ba';
const uint16_t LANGUAGE_BASQUE              = 'eu';
const uint16_t LANGUAGE_BENGALI             = 'bn';
const uint16_t LANGUAGE_BHUTANI             = 'dz';
const uint16_t LANGUAGE_BIHARI              = 'bh';
const uint16_t LANGUAGE_BISLAMA             = 'bi';
const uint16_t LANGUAGE_BRETON              = 'br';
const uint16_t LANGUAGE_BULGARIAN           = 'bg';
const uint16_t LANGUAGE_BURMESE             = 'my';
const uint16_t LANGUAGE_BYELORUSSIAN        = 'be';
const uint16_t LANGUAGE_CAMBODIAN           = 'km';
const uint16_t LANGUAGE_CATALAN             = 'ca';
const uint16_t LANGUAGE_CHINESE             = 'zh';
const uint16_t LANGUAGE_CORSICAN            = 'co';
const uint16_t LANGUAGE_CROATIAN            = 'hr';
const uint16_t LANGUAGE_CZECH               = 'cs';
const uint16_t LANGUAGE_DANISH              = 'da';
const uint16_t LANGUAGE_DUTCH               = 'nl';
const uint16_t LANGUAGE_ENGLISH             = 'en';
const uint16_t LANGUAGE_ESPERANTO           = 'eo';
const uint16_t LANGUAGE_ESTONIAN            = 'et';
const uint16_t LANGUAGE_FAEROESE            = 'fo';
const uint16_t LANGUAGE_FIJI                = 'fj';
const uint16_t LANGUAGE_FINNISH             = 'fi';
const uint16_t LANGUAGE_FRENCH              = 'fr';
const uint16_t LANGUAGE_FRISIAN             = 'fy';
const uint16_t LANGUAGE_GALICIAN            = 'gl';
const uint16_t LANGUAGE_GEORGIAN            = 'ka';
const uint16_t LANGUAGE_GERMAN              = 'de';
const uint16_t LANGUAGE_GREEK               = 'el';
const uint16_t LANGUAGE_GREENLANDIC         = 'kl';
const uint16_t LANGUAGE_GUARANI             = 'gn';
const uint16_t LANGUAGE_GUJARATI            = 'gu';
const uint16_t LANGUAGE_HAUSA               = 'ha';
const uint16_t LANGUAGE_HEBREW              = 'he';
const uint16_t LANGUAGE_HINDI               = 'hi';
const uint16_t LANGUAGE_HUNGARIAN           = 'hu';
const uint16_t LANGUAGE_ICELANDIC           = 'is';
const uint16_t LANGUAGE_INDONESIAN          = 'id';
const uint16_t LANGUAGE_INTERLINGUA         = 'ia';
const uint16_t LANGUAGE_INTERLINGUE         = 'ie';
const uint16_t LANGUAGE_INUPIAK             = 'ik';
const uint16_t LANGUAGE_INUKTITUT           = 'iu';
const uint16_t LANGUAGE_IRISH               = 'ga';
const uint16_t LANGUAGE_ITALIAN             = 'it';
const uint16_t LANGUAGE_JAPANESE            = 'ja';
const uint16_t LANGUAGE_JAVANESE            = 'jw';
const uint16_t LANGUAGE_KANNADA             = 'kn';
const uint16_t LANGUAGE_KASHMIRI            = 'ks';
const uint16_t LANGUAGE_KAZAKH              = 'kk';
const uint16_t LANGUAGE_KINYARWANDA         = 'rw';
const uint16_t LANGUAGE_KIRGHIZ             = 'ky';
const uint16_t LANGUAGE_KIRUNDI             = 'rn';
const uint16_t LANGUAGE_KOREAN              = 'ko';
const uint16_t LANGUAGE_KURDISH             = 'ku';
const uint16_t LANGUAGE_LAOTHIAN            = 'lo';
const uint16_t LANGUAGE_LATIN               = 'la';
const uint16_t LANGUAGE_LATVIAN_LETTISH     = 'lv';
const uint16_t LANGUAGE_LINGALA             = 'ln';
const uint16_t LANGUAGE_LITHUANIAN          = 'lt';
const uint16_t LANGUAGE_MACEDONIAN          = 'mk';
const uint16_t LANGUAGE_MALAGASY            = 'mg';
const uint16_t LANGUAGE_MALAY               = 'ms';
const uint16_t LANGUAGE_MALAYALAM           = 'ml';
const uint16_t LANGUAGE_MALTESE             = 'mt';
const uint16_t LANGUAGE_MAORI               = 'mi';
const uint16_t LANGUAGE_MARATHI             = 'mr';
const uint16_t LANGUAGE_MOLDAVIAN           = 'mo';
const uint16_t LANGUAGE_MONGOLIAN           = 'mn';
const uint16_t LANGUAGE_NAURU               = 'na';
const uint16_t LANGUAGE_NEPALI              = 'ne';
const uint16_t LANGUAGE_NORWEGIAN           = 'no';
const uint16_t LANGUAGE_OCCITAN             = 'oc';
const uint16_t LANGUAGE_ORIYA               = 'or';
const uint16_t LANGUAGE_PASHTO_PUSHTO       = 'ps';
const uint16_t LANGUAGE_PERSIAN             = 'fa';
const uint16_t LANGUAGE_POLISH              = 'pl';
const uint16_t LANGUAGE_PORTUGUESE          = 'pt';
const uint16_t LANGUAGE_PUNJABI             = 'pa';
const uint16_t LANGUAGE_QUECHUA             = 'qu';
const uint16_t LANGUAGE_RHAETO_ROMANCE      = 'rm';
const uint16_t LANGUAGE_ROMANIAN            = 'ro';
const uint16_t LANGUAGE_RUSSIAN             = 'ru';
const uint16_t LANGUAGE_SAMOAN              = 'sm';
const uint16_t LANGUAGE_SANGRO              = 'sg';
const uint16_t LANGUAGE_SANSKRIT            = 'sa';
const uint16_t LANGUAGE_SCOTS_GAELIC        = 'gd';
const uint16_t LANGUAGE_SERBIAN             = 'sr';
const uint16_t LANGUAGE_SERBO_CROATIAN      = 'sh';
const uint16_t LANGUAGE_SESOTHO             = 'st';
const uint16_t LANGUAGE_SETSWANA            = 'tn';
const uint16_t LANGUAGE_SHONA               = 'sn';
const uint16_t LANGUAGE_SINDHI              = 'sd';
const uint16_t LANGUAGE_SINGHALESE          = 'si';
const uint16_t LANGUAGE_SISWATI             = 'ss';
const uint16_t LANGUAGE_SLOVAK              = 'sk';
const uint16_t LANGUAGE_SLOVENIAN           = 'sl';
const uint16_t LANGUAGE_SOMALI              = 'so';
const uint16_t LANGUAGE_SPANISH             = 'es';
const uint16_t LANGUAGE_SUDANESE            = 'su';
const uint16_t LANGUAGE_SWAHILI             = 'sw';
const uint16_t LANGUAGE_SWEDISH             = 'sv';
const uint16_t LANGUAGE_TAGALOG             = 'tl';
const uint16_t LANGUAGE_TAJIK               = 'tg';
const uint16_t LANGUAGE_TAMIL               = 'ta';
const uint16_t LANGUAGE_TATAR               = 'tt';
const uint16_t LANGUAGE_TEGULU              = 'te';
const uint16_t LANGUAGE_THAI                = 'th';
const uint16_t LANGUAGE_TIBETAN             = 'bo';
const uint16_t LANGUAGE_TIGRINYA            = 'ti';
const uint16_t LANGUAGE_TONGA               = 'to';
const uint16_t LANGUAGE_TSONGA              = 'ts';
const uint16_t LANGUAGE_TURKISH             = 'tr';
const uint16_t LANGUAGE_TURKMEN             = 'tk';
const uint16_t LANGUAGE_TWI                 = 'tw';
const uint16_t LANGUAGE_UIGUR               = 'ug';
const uint16_t LANGUAGE_UKRAINIAN           = 'uk';
const uint16_t LANGUAGE_URDU                = 'ur';
const uint16_t LANGUAGE_UZBEK               = 'uz';
const uint16_t LANGUAGE_VIETNAMESE          = 'vi';
const uint16_t LANGUAGE_VOLAPUK             = 'vo';
const uint16_t LANGUAGE_WELCH               = 'cy';
const uint16_t LANGUAGE_WOLOF               = 'wo';
const uint16_t LANGUAGE_XHOSA               = 'xh';
const uint16_t LANGUAGE_YIDDISH             = 'yi';
const uint16_t LANGUAGE_YORUBA              = 'yo';
const uint16_t LANGUAGE_ZHUANG              = 'za';
const uint16_t LANGUAGE_ZULU                = 'zu';

// Default language
const uint16_t LANGUAGE_DEFAULT             = LANGUAGE_ENGLISH;
const char8_t* const LANGUAGE_DEFAULT_STR   = "en";

//! Countries
const uint16_t COUNTRY_AFGHANISTAN                                      = 'AF';
const uint16_t COUNTRY_ALBANIA                                          = 'AL';
const uint16_t COUNTRY_ALGERIA                                          = 'DZ';
const uint16_t COUNTRY_AMERICAN_SAMOA                                   = 'AS';
const uint16_t COUNTRY_ANDORRA                                          = 'AD';
const uint16_t COUNTRY_ANGOLA                                           = 'AO';
const uint16_t COUNTRY_ANGUILLA                                         = 'AI';
const uint16_t COUNTRY_ANTARCTICA                                       = 'AQ';
const uint16_t COUNTRY_ANTIGUA_BARBUDA                                  = 'AG';
const uint16_t COUNTRY_ARGENTINA                                        = 'AR';
const uint16_t COUNTRY_ARMENIA                                          = 'AM';
const uint16_t COUNTRY_ARUBA                                            = 'AW';
const uint16_t COUNTRY_AUSTRALIA                                        = 'AU';
const uint16_t COUNTRY_AUSTRIA                                          = 'AT';
const uint16_t COUNTRY_AZERBAIJAN                                       = 'AZ';
const uint16_t COUNTRY_BAHAMAS                                          = 'BS';
const uint16_t COUNTRY_BAHRAIN                                          = 'BH';
const uint16_t COUNTRY_BANGLADESH                                       = 'BD';
const uint16_t COUNTRY_BARBADOS                                         = 'BB';
const uint16_t COUNTRY_BELARUS                                          = 'BY';
const uint16_t COUNTRY_BELGIUM                                          = 'BE';
const uint16_t COUNTRY_BELIZE                                           = 'BZ';
const uint16_t COUNTRY_BENIN                                            = 'BJ';
const uint16_t COUNTRY_BERMUDA                                          = 'BM';
const uint16_t COUNTRY_BHUTAN                                           = 'BT';
const uint16_t COUNTRY_BOLIVIA                                          = 'BO';
const uint16_t COUNTRY_BOSNIA_HERZEGOVINA                               = 'BA';
const uint16_t COUNTRY_BOTSWANA                                         = 'BW';
const uint16_t COUNTRY_BOUVET_ISLAND                                    = 'BV';
const uint16_t COUNTRY_BRAZIL                                           = 'BR';
const uint16_t COUNTRY_BRITISH_INDIAN_OCEAN_TERRITORY                   = 'IO';
const uint16_t COUNTRY_BRUNEI_DARUSSALAM                                = 'BN';
const uint16_t COUNTRY_BULGARIA                                         = 'BG';
const uint16_t COUNTRY_BURKINA_FASO                                     = 'BF';
const uint16_t COUNTRY_BURUNDI                                          = 'BI';
const uint16_t COUNTRY_CAMBODIA                                         = 'KH';
const uint16_t COUNTRY_CAMEROON                                         = 'CM';
const uint16_t COUNTRY_CANADA                                           = 'CA';
const uint16_t COUNTRY_CAPE_VERDE                                       = 'CV';
const uint16_t COUNTRY_CAYMAN_ISLANDS                                   = 'KY';
const uint16_t COUNTRY_CENTRAL_AFRICAN_REPUBLIC                         = 'CF';
const uint16_t COUNTRY_CHAD                                             = 'TD';
const uint16_t COUNTRY_CHILE                                            = 'CL';
const uint16_t COUNTRY_CHINA                                            = 'CN';
const uint16_t COUNTRY_CHRISTMAS_ISLAND                                 = 'CX';
const uint16_t COUNTRY_COCOS_KEELING_ISLANDS                            = 'CC';
const uint16_t COUNTRY_COLOMBIA                                         = 'CO';
const uint16_t COUNTRY_COMOROS                                          = 'KM';
const uint16_t COUNTRY_CONGO                                            = 'CG';
const uint16_t COUNTRY_COOK_ISLANDS                                     = 'CK';
const uint16_t COUNTRY_COSTA_RICA                                       = 'CR';
const uint16_t COUNTRY_COTE_DIVOIRE                                     = 'CI';
const uint16_t COUNTRY_CROATIA                                          = 'HR';
const uint16_t COUNTRY_CUBA                                             = 'CU';
const uint16_t COUNTRY_CYPRUS                                           = 'CY';
const uint16_t COUNTRY_CZECH_REPUBLIC                                   = 'CZ';
const uint16_t COUNTRY_DENMARK                                          = 'DK';
const uint16_t COUNTRY_DJIBOUTI                                         = 'DJ';
const uint16_t COUNTRY_DOMINICA                                         = 'DM';
const uint16_t COUNTRY_DOMINICAN_REPUBLIC                               = 'DO';
const uint16_t COUNTRY_EAST_TIMOR                                       = 'TP';
const uint16_t COUNTRY_ECUADOR                                          = 'EC';
const uint16_t COUNTRY_EGYPT                                            = 'EG';
const uint16_t COUNTRY_EL_SALVADOR                                      = 'SV';
const uint16_t COUNTRY_EQUATORIAL_GUINEA                                = 'GQ';
const uint16_t COUNTRY_ERITREA                                          = 'ER';
const uint16_t COUNTRY_ESTONIA                                          = 'EE';
const uint16_t COUNTRY_ETHIOPIA                                         = 'ET';
const uint16_t COUNTRY_EUROPE_SSGFI_ONLY                                = 'EU';
const uint16_t COUNTRY_FALKLAND_ISLANDS                                 = 'FK';
const uint16_t COUNTRY_FAEROE_ISLANDS                                   = 'FO';
const uint16_t COUNTRY_FIJI                                             = 'FJ';
const uint16_t COUNTRY_FINLAND                                          = 'FI';
const uint16_t COUNTRY_FRANCE                                           = 'FR';
const uint16_t COUNTRY_FRANCE_METROPOLITAN                              = 'FX';
const uint16_t COUNTRY_FRENCH_GUIANA                                    = 'GF';
const uint16_t COUNTRY_FRENCH_POLYNESIA                                 = 'PF';
const uint16_t COUNTRY_FRENCH_SOUTHERN_TERRITORIES                      = 'TF';
const uint16_t COUNTRY_GABON                                            = 'GA';
const uint16_t COUNTRY_GAMBIA                                           = 'GM';
const uint16_t COUNTRY_GEORGIA                                          = 'GE';
const uint16_t COUNTRY_GERMANY                                          = 'DE';
const uint16_t COUNTRY_GHANA                                            = 'GH';
const uint16_t COUNTRY_GIBRALTAR                                        = 'GI';
const uint16_t COUNTRY_GREECE                                           = 'GR';
const uint16_t COUNTRY_GREENLAND                                        = 'GL';
const uint16_t COUNTRY_GRENADA                                          = 'GD';
const uint16_t COUNTRY_GUADELOUPE                                       = 'GP';
const uint16_t COUNTRY_GUAM                                             = 'GU';
const uint16_t COUNTRY_GUATEMALA                                        = 'GT';
const uint16_t COUNTRY_GUINEA                                           = 'GN';
const uint16_t COUNTRY_GUINEA_BISSAU                                    = 'GW';
const uint16_t COUNTRY_GUYANA                                           = 'GY';
const uint16_t COUNTRY_HAITI                                            = 'HT';
const uint16_t COUNTRY_HEARD_AND_MC_DONALD_ISLANDS                      = 'HM';
const uint16_t COUNTRY_HONDURAS                                         = 'HN';
const uint16_t COUNTRY_HONG_KONG                                        = 'HK';
const uint16_t COUNTRY_HUNGARY                                          = 'HU';
const uint16_t COUNTRY_ICELAND                                          = 'IS';
const uint16_t COUNTRY_INDIA                                            = 'IN';
const uint16_t COUNTRY_INDONESIA                                        = 'ID';
const uint16_t COUNTRY_INTERNATIONAL_SSGFI_ONLY                         = 'II';
const uint16_t COUNTRY_IRAN                                             = 'IR';
const uint16_t COUNTRY_IRAQ                                             = 'IQ';
const uint16_t COUNTRY_IRELAND                                          = 'IE';
const uint16_t COUNTRY_ISRAEL                                           = 'IL';
const uint16_t COUNTRY_ITALY                                            = 'IT';
const uint16_t COUNTRY_JAMAICA                                          = 'JM';
const uint16_t COUNTRY_JAPAN                                            = 'JP';
const uint16_t COUNTRY_JORDAN                                           = 'JO';
const uint16_t COUNTRY_KAZAKHSTAN                                       = 'KZ';
const uint16_t COUNTRY_KENYA                                            = 'KE';
const uint16_t COUNTRY_KIRIBATI                                         = 'KI';
const uint16_t COUNTRY_KOREA_DEMOCRATIC_PEOPLES_REPUBLIC_OF             = 'KP';
const uint16_t COUNTRY_KOREA_REPUBLIC_OF                                = 'KR';
const uint16_t COUNTRY_KUWAIT                                           = 'KW';
const uint16_t COUNTRY_KYRGYZSTAN                                       = 'KG';
const uint16_t COUNTRY_LAO_PEOPLES_DEMOCRATIC_REPUBLIC                  = 'LA';
const uint16_t COUNTRY_LATVIA                                           = 'LV';
const uint16_t COUNTRY_LEBANON                                          = 'LB';
const uint16_t COUNTRY_LESOTHO                                          = 'LS';
const uint16_t COUNTRY_LIBERIA                                          = 'LR';
const uint16_t COUNTRY_LIBYAN_ARAB_JAMAHIRIYA                           = 'LY';
const uint16_t COUNTRY_LIECHTENSTEIN                                    = 'LI';
const uint16_t COUNTRY_LITHUANIA                                        = 'LT';
const uint16_t COUNTRY_LUXEMBOURG                                       = 'LU';
const uint16_t COUNTRY_MACAU                                            = 'MO';
const uint16_t COUNTRY_MACEDONIA_THE_FORMER_YUGOSLAV_REPUBLIC_OF        = 'MK';
const uint16_t COUNTRY_MADAGASCAR                                       = 'MG';
const uint16_t COUNTRY_MALAWI                                           = 'MW';
const uint16_t COUNTRY_MALAYSIA                                         = 'MY';
const uint16_t COUNTRY_MALDIVES                                         = 'MV';
const uint16_t COUNTRY_MALI                                             = 'ML';
const uint16_t COUNTRY_MALTA                                            = 'MT';
const uint16_t COUNTRY_MARSHALL_ISLANDS                                 = 'MH';
const uint16_t COUNTRY_MARTINIQUE                                       = 'MQ';
const uint16_t COUNTRY_MAURITANIA                                       = 'MR';
const uint16_t COUNTRY_MAURITIUS                                        = 'MU';
const uint16_t COUNTRY_MAYOTTE                                          = 'YT';
const uint16_t COUNTRY_MEXICO                                           = 'MX';
const uint16_t COUNTRY_MICRONESIA_FEDERATED_STATES_OF                   = 'FM';
const uint16_t COUNTRY_MOLDOVA_REPUBLIC_OF                              = 'MD';
const uint16_t COUNTRY_MONACO                                           = 'MC';
const uint16_t COUNTRY_MONGOLIA                                         = 'MN';
const uint16_t COUNTRY_MONTSERRAT                                       = 'MS';
const uint16_t COUNTRY_MOROCCO                                          = 'MA';
const uint16_t COUNTRY_MOZAMBIQUE                                       = 'MZ';
const uint16_t COUNTRY_MYANMAR                                          = 'MM';
const uint16_t COUNTRY_NAMIBIA                                          = 'NA';
const uint16_t COUNTRY_NAURU                                            = 'NR';
const uint16_t COUNTRY_NEPAL                                            = 'NP';
const uint16_t COUNTRY_NETHERLANDS                                      = 'NL';
const uint16_t COUNTRY_NETHERLANDS_ANTILLES                             = 'AN';
const uint16_t COUNTRY_NEW_CALEDONIA                                    = 'NC';
const uint16_t COUNTRY_NEW_ZEALAND                                      = 'NZ';
const uint16_t COUNTRY_NICARAGUA                                        = 'NI';
const uint16_t COUNTRY_NIGER                                            = 'NE';
const uint16_t COUNTRY_NIGERIA                                          = 'NG';
const uint16_t COUNTRY_NIUE                                             = 'NU';
const uint16_t COUNTRY_NORFOLK_ISLAND                                   = 'NF';
const uint16_t COUNTRY_NORTHERN_MARIANA_ISLANDS                         = 'MP';
const uint16_t COUNTRY_NORWAY                                           = 'NO';
const uint16_t COUNTRY_OMAN                                             = 'OM';
const uint16_t COUNTRY_PAKISTAN                                         = 'PK';
const uint16_t COUNTRY_PALAU                                            = 'PW';
const uint16_t COUNTRY_PANAMA                                           = 'PA';
const uint16_t COUNTRY_PAPUA_NEW_GUINEA                                 = 'PG';
const uint16_t COUNTRY_PARAGUAY                                         = 'PY';
const uint16_t COUNTRY_PERU                                             = 'PE';
const uint16_t COUNTRY_PHILIPPINES                                      = 'PH';
const uint16_t COUNTRY_PITCAIRN                                         = 'PN';
const uint16_t COUNTRY_POLAND                                           = 'PL';
const uint16_t COUNTRY_PORTUGAL                                         = 'PT';
const uint16_t COUNTRY_PUERTO_RICO                                      = 'PR';
const uint16_t COUNTRY_QATAR                                            = 'QA';
const uint16_t COUNTRY_REUNION                                          = 'RE';
const uint16_t COUNTRY_ROMANIA                                          = 'RO';
const uint16_t COUNTRY_RUSSIAN_FEDERATION                               = 'RU';
const uint16_t COUNTRY_RWANDA                                           = 'RW';
const uint16_t COUNTRY_SAINT_KITTS_AND_NEVIS                            = 'KN';
const uint16_t COUNTRY_SAINT_LUCIA                                      = 'LC';
const uint16_t COUNTRY_SAINT_VINCENT_AND_THE_GRENADINES                 = 'VC';
const uint16_t COUNTRY_SAMOA                                            = 'WS';
const uint16_t COUNTRY_SAN_MARINO                                       = 'SM';
const uint16_t COUNTRY_SAO_TOME_AND_PRINCIPE                            = 'ST';
const uint16_t COUNTRY_SAUDI_ARABIA                                     = 'SA';
const uint16_t COUNTRY_SENEGAL                                          = 'SN';
const uint16_t COUNTRY_SEYCHELLES                                       = 'SC';
const uint16_t COUNTRY_SIERRA_LEONE                                     = 'SL';
const uint16_t COUNTRY_SINGAPORE                                        = 'SG';
const uint16_t COUNTRY_SLOVAKIA_                                        = 'SK';
const uint16_t COUNTRY_SLOVENIA                                         = 'SI';
const uint16_t COUNTRY_SOLOMON_ISLANDS                                  = 'SB';
const uint16_t COUNTRY_SOMALIA                                          = 'SO';
const uint16_t COUNTRY_SOUTH_AFRICA                                     = 'ZA';
const uint16_t COUNTRY_SOUTH_GEORGIA_AND_THE_SOUTH_SANDWICH_ISLANDS     = 'GS';
const uint16_t COUNTRY_SPAIN                                            = 'ES';
const uint16_t COUNTRY_SRI_LANKA                                        = 'LK';
const uint16_t COUNTRY_ST_HELENA                                        = 'SH';
const uint16_t COUNTRY_ST_PIERRE_AND_MIQUELON                           = 'PM';
const uint16_t COUNTRY_SUDAN                                            = 'SD';
const uint16_t COUNTRY_SURINAME                                         = 'SR';
const uint16_t COUNTRY_SVALBARD_AND_JAN_MAYEN_ISLANDS                   = 'SJ';
const uint16_t COUNTRY_SWAZILAND                                        = 'SZ';
const uint16_t COUNTRY_SWEDEN                                           = 'SE';
const uint16_t COUNTRY_SWITZERLAND                                      = 'CH';
const uint16_t COUNTRY_SYRIAN_ARAB_REPUBLIC                             = 'SY';
const uint16_t COUNTRY_TAIWAN                                           = 'TW';
const uint16_t COUNTRY_TAJIKISTAN                                       = 'TJ';
const uint16_t COUNTRY_TANZANIA_UNITED_REPUBLIC_OF                      = 'TZ';
const uint16_t COUNTRY_THAILAND                                         = 'TH';
const uint16_t COUNTRY_TOGO                                             = 'TG';
const uint16_t COUNTRY_TOKELAU                                          = 'TK';
const uint16_t COUNTRY_TONGA                                            = 'TO';
const uint16_t COUNTRY_TRINIDAD_AND_TOBAGO                              = 'TT';
const uint16_t COUNTRY_TUNISIA                                          = 'TN';
const uint16_t COUNTRY_TURKEY                                           = 'TR';
const uint16_t COUNTRY_TURKMENISTAN                                     = 'TM';
const uint16_t COUNTRY_TURKS_AND_CAICOS_ISLANDS                         = 'TC';
const uint16_t COUNTRY_TUVALU                                           = 'TV';
const uint16_t COUNTRY_UGANDA                                           = 'UG';
const uint16_t COUNTRY_UKRAINE                                          = 'UA';
const uint16_t COUNTRY_UNITED_ARAB_EMIRATES                             = 'AE';
const uint16_t COUNTRY_UNITED_KINGDOM                                   = 'GB';
const uint16_t COUNTRY_UNITED_STATES                                    = 'US';
const uint16_t COUNTRY_UNITED_STATES_MINOR_OUTLYING_ISLANDS             = 'UM';
const uint16_t COUNTRY_URUGUAY                                          = 'UY';
const uint16_t COUNTRY_UZBEKISTAN                                       = 'UZ';
const uint16_t COUNTRY_VANUATU                                          = 'VU';
const uint16_t COUNTRY_VATICAN_CITY_STATE                               = 'VA';
const uint16_t COUNTRY_VENEZUELA                                        = 'VE';
const uint16_t COUNTRY_VIETNAM                                          = 'VN';
const uint16_t COUNTRY_VIRGIN_ISLANDS_BRITISH                           = 'VG';
const uint16_t COUNTRY_VIRGIN_ISLANDS_US                                = 'VI';
const uint16_t COUNTRY_WALLIS_AND_FUTUNA_ISLANDS                        = 'WF';
const uint16_t COUNTRY_WESTERN_SAHARA                                   = 'EH';
const uint16_t COUNTRY_YEMEN                                            = 'YE';
const uint16_t COUNTRY_YUGOSLAVIA                                       = 'YU';
const uint16_t COUNTRY_ZAIRE                                            = 'ZR';
const uint16_t COUNTRY_ZAMBIA                                           = 'ZM';
const uint16_t COUNTRY_ZIMBABWE                                         = 'ZW';

// Default country
const uint16_t COUNTRY_DEFAULT              = COUNTRY_UNITED_STATES;
const char8_t* const COUNTRY_DEFAULT_STR    = "US";

/*** Macros ***************************************************************************************/

//! toupper replacement
#define LocaleTokenToUpper(uCharToModify)                                                   \
    ((((unsigned char)(uCharToModify) >= 'a') && ((unsigned char)(uCharToModify) <= 'z')) ? \
            (((unsigned char)(uCharToModify)) & (~32)) :                                    \
            (uCharToModify))

//! tolower replacement
#define LocaleTokenToLower(uCharToModify)                                                   \
    ((((unsigned char)(uCharToModify) >= 'A') && ((unsigned char)(uCharToModify) <= 'Z')) ? \
            (((unsigned char)(uCharToModify)) | (32)) :                                     \
            (uCharToModify))

//! Create a locale token from shorts representing country and language
#define LocaleTokenCreate(uLanguage, uCountry)                                           \
    (((LocaleTokenShortToLower(uLanguage)) << 16) + (LocaleTokenShortToUpper(uCountry)))

//! Create a locale token from strings containing country and language
#define LocaleTokenCreateFromStrings(strLanguage, strCountry)      \
    (LocaleTokenCreate(LocaleTokenGetShortFromString(strLanguage), \
    LocaleTokenGetShortFromString(strCountry)))

//! Create a locale token from a single string (language + country - ex: "enUS")
#define LocaleTokenCreateFromString(strLocality)                                        \
    (((char *)(strLocality) != nullptr) && strlen((char *)strLocality) >= 4) ?             \
    (LocaleTokenCreateFromStrings(&(strLocality)[0], &(strLocality)[2])) : Blaze::LOCALE_BLANK

//! Create a locale token from a single delimited string (language + delimiter + (optional) country - ex: "en-US" or "en_US")
#define LocaleTokenCreateFromDelimitedString(strLocality)                                        \
    (((char *)(strLocality) != nullptr) && strlen((char *)strLocality) >= 5) ?             \
    (LocaleTokenCreateFromStrings(&(strLocality)[0], &(strLocality)[3])) : \
        (((char *)(strLocality) != nullptr) && strlen((char *)strLocality) >= 2) ? \
        (LocaleTokenCreateFromStrings(&(strLocality)[0], nullptr)) : Blaze::LOCALE_BLANK

//! Get a int16_t integer from a string
#define LocaleTokenGetShortFromString(strInstring) (((((char *)(strInstring) == nullptr) \
    || ((char *)strInstring)[0] == '\0')) ? ((uint16_t)(0)) :                         \
    ((uint16_t)((((char *)strInstring)[0] << 8) + ((char *)strInstring)[1])))

//! Pull the country (int16_t) from a locale token (int32_t)
#define LocaleTokenGetCountry(uToken) ((uint16_t)((uToken) & 0xFFFF))

//! Pull the language (int16_t) from a locale token (int32_t)
#define LocaleTokenGetLanguage(uToken) ((uint16_t)(((uToken) >> 16) & 0xFFFF))

//! Replace the country in a locality value
#define LocaleTokenSetCountry(uToken, uCountry) (uToken) = (((uToken) & 0xFFFF0000) \
    | (uCountry));

//! Replace the language in a locality value
#define LocaleTokenSetLanguage(uToken, uLanguage) (uToken) = (((uToken) & 0x0000FFFF) \
    | ((uLanguage) << 16));

//! Write the country contained in a locale token to a character string
#define LocaleTokenCreateCountryString(strOutstring, uToken) \
    {                                                        \
        (strOutstring)[0] = (char)(((uToken) >> 8) & 0xFF);  \
        (strOutstring)[1] = (char)((uToken) & 0xFF);         \
        (strOutstring)[2]='\0';                              \
    }

//! Write the language contained in a locale token to a character string
#define LocaleTokenCreateLanguageString(strOutstring, uToken) \
    {                                                         \
        (strOutstring)[0]=(char)(((uToken) >> 24) & 0xFF);    \
        (strOutstring)[1]=(char)(((uToken) >> 16) & 0xFF);    \
        (strOutstring)[2]='\0';                               \
    }

//! Write the entire locality string to a character string
#define LocaleTokenCreateLocalityString(strOutstring, uToken) \
    {                                                         \
        (strOutstring)[0]=(char)(((uToken) >> 24) & 0xFF);    \
        (strOutstring)[1]=(char)(((uToken) >> 16) & 0xFF);    \
        (strOutstring)[2]=(char)(((uToken) >> 8) & 0xFF);     \
        (strOutstring)[3]=(char)((uToken) & 0xFF);            \
        (strOutstring)[4]='\0';                               \
    }

//! Macro to provide and easy way to display the token in character format
#define LocaleTokenPrintCharArray(uToken) (char)(((uToken)>>24)&0xFF),             \
    (char)(((uToken)>>16)&0xFF), (char)(((uToken)>>8)&0xFF), (char)((uToken)&0xFF)

//! Provide a way to capitalize the elements in a int16_t
#define LocaleTokenShortToUpper(uShort)                               \
    ((uint16_t)(((LocaleTokenToUpper(((uShort) >> 8) & 0xFF)) << 8) + \
    (LocaleTokenToUpper((uShort) & 0xFF))))

//! Provide a way to lowercase the elements in a int16_t
#define LocaleTokenShortToLower(uShort)                               \
    ((uint16_t)(((LocaleTokenToLower(((uShort) >> 8) & 0xFF)) << 8) + \
    (LocaleTokenToLower((uShort) & 0xFF))))

} // Blaze

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif // LOCALES_H
