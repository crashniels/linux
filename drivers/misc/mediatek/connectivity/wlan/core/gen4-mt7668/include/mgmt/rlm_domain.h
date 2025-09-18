/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright(C) 2016 MediaTek Inc.
 */

/*! \file   "rlm_domain.h"
 *    \brief
 */

#ifndef _RLM_DOMAIN_H
#define _RLM_DOMAIN_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define MAX_SUBBAND_NUM              6
#define MAX_SUBBAND_NUM_5G           8

#define COUNTRY_CODE_NULL            ((u16)0x0)

/* ISO/IEC 3166-1 two-character country codes */

#define COUNTRY_CODE_AD              (((u16)'A' << 8) | (u16)'D')  /* Andorra */
#define COUNTRY_CODE_AE              (((u16)'A' << 8) | (u16)'E')  /* UAE */
#define COUNTRY_CODE_AF              (((u16)'A' << 8) | (u16)'F')  /* Afghanistan */
#define COUNTRY_CODE_AG              (((u16)'A' << 8) | (u16)'G')  /* Antigua & Barbuda */
#define COUNTRY_CODE_AI              (((u16)'A' << 8) | (u16)'I')  /* Anguilla */
#define COUNTRY_CODE_AL              (((u16)'A' << 8) | (u16)'L')  /* Albania */
#define COUNTRY_CODE_AM              (((u16)'A' << 8) | (u16)'M')  /* Armenia */
#define COUNTRY_CODE_AN              (((u16)'A' << 8) | (u16)'N')  /* Netherlands Antilles */
#define COUNTRY_CODE_AO              (((u16)'A' << 8) | (u16)'O')  /* Angola */
#define COUNTRY_CODE_AR              (((u16)'A' << 8) | (u16)'R')  /* Argentina */
#define COUNTRY_CODE_AS              (((u16)'A' << 8) | (u16)'S')  /* American Samoa (USA) */
#define COUNTRY_CODE_AT              (((u16)'A' << 8) | (u16)'T')  /* Austria */
#define COUNTRY_CODE_AU              (((u16)'A' << 8) | (u16)'U')  /* Australia */
#define COUNTRY_CODE_AW              (((u16)'A' << 8) | (u16)'W')  /* Aruba */
#define COUNTRY_CODE_AZ              (((u16)'A' << 8) | (u16)'Z')  /* Azerbaijan */
#define COUNTRY_CODE_BA \
    (((u16)'B' << 8) | (u16)'A')  /* Bosnia and Herzegovina */
#define COUNTRY_CODE_BB              (((u16)'B' << 8) | (u16)'B')  /* Barbados */
#define COUNTRY_CODE_BD              (((u16)'B' << 8) | (u16)'D')  /* Bangladesh */
#define COUNTRY_CODE_BE              (((u16)'B' << 8) | (u16)'E')  /* Belgium */
#define COUNTRY_CODE_BF              (((u16)'B' << 8) | (u16)'F')  /* Burkina Faso */
#define COUNTRY_CODE_BG              (((u16)'B' << 8) | (u16)'G')  /* Bulgaria */
#define COUNTRY_CODE_BH              (((u16)'B' << 8) | (u16)'H')  /* Bahrain */
#define COUNTRY_CODE_BI              (((u16)'B' << 8) | (u16)'I')  /* Burundi */
#define COUNTRY_CODE_BJ              (((u16)'B' << 8) | (u16)'J')  /* Benin */
#define COUNTRY_CODE_BM              (((u16)'B' << 8) | (u16)'M')  /* Bermuda */
#define COUNTRY_CODE_BN              (((u16)'B' << 8) | (u16)'N')  /* Brunei */
#define COUNTRY_CODE_BO              (((u16)'B' << 8) | (u16)'O')  /* Bolivia */
#define COUNTRY_CODE_BR              (((u16)'B' << 8) | (u16)'R')  /* Brazil */
#define COUNTRY_CODE_BS              (((u16)'B' << 8) | (u16)'S')  /* Bahamas */
#define COUNTRY_CODE_BT              (((u16)'B' << 8) | (u16)'T')  /* Bhutan */
#define COUNTRY_CODE_BW              (((u16)'B' << 8) | (u16)'W')  /* Botswana */
#define COUNTRY_CODE_BY              (((u16)'B' << 8) | (u16)'Y')  /* Belarus */
#define COUNTRY_CODE_BZ              (((u16)'B' << 8) | (u16)'Z')  /* Belize */
#define COUNTRY_CODE_CA              (((u16)'C' << 8) | (u16)'A')  /* Canada */
#define COUNTRY_CODE_CD \
    (((u16)'C' << 8) | (u16)'D')  /* Congo. Democratic Republic of the */
#define COUNTRY_CODE_CF \
    (((u16)'C' << 8) | (u16)'F')  /* Central African Republic */
#define COUNTRY_CODE_CG \
    (((u16)'C' << 8) | (u16)'G')  /* Congo. Republic of the */
#define COUNTRY_CODE_CH              (((u16)'C' << 8) | (u16)'H')  /* Switzerland */
#define COUNTRY_CODE_CI              (((u16)'C' << 8) | (u16)'I')  /* Cote d'lvoire */
#define COUNTRY_CODE_CK              (((u16)'C' << 8) | (u16)'K')  /* Cook Island */
#define COUNTRY_CODE_CL              (((u16)'C' << 8) | (u16)'L')  /* Chile */
#define COUNTRY_CODE_CM              (((u16)'C' << 8) | (u16)'M')  /* Cameroon */
#define COUNTRY_CODE_CN              (((u16)'C' << 8) | (u16)'N')  /* China */
#define COUNTRY_CODE_CO              (((u16)'C' << 8) | (u16)'O')  /* Columbia */
#define COUNTRY_CODE_CR              (((u16)'C' << 8) | (u16)'R')  /* Costa Rica */
#define COUNTRY_CODE_CU              (((u16)'C' << 8) | (u16)'U')  /* Cuba */
#define COUNTRY_CODE_CV              (((u16)'C' << 8) | (u16)'V')  /* Cape Verde */
#define COUNTRY_CODE_CX \
    (((u16)'C' << 8) | (u16)'X')  /* "Christmas Island(Australia) */
#define COUNTRY_CODE_CY              (((u16)'C' << 8) | (u16)'Y')  /* Cyprus */
#define COUNTRY_CODE_CZ              (((u16)'C' << 8) | (u16)'Z')  /* Czech */
#define COUNTRY_CODE_DE              (((u16)'D' << 8) | (u16)'E')  /* Germany */
#define COUNTRY_CODE_DJ              (((u16)'D' << 8) | (u16)'J')  /* Djibouti */
#define COUNTRY_CODE_DK              (((u16)'D' << 8) | (u16)'K')  /* Denmark */
#define COUNTRY_CODE_DM              (((u16)'D' << 8) | (u16)'M')  /* Dominica */
#define COUNTRY_CODE_DO              (((u16)'D' << 8) | (u16)'O')  /* Dominican Republic */
#define COUNTRY_CODE_DZ              (((u16)'D' << 8) | (u16)'Z')  /* Algeria */
#define COUNTRY_CODE_EC              (((u16)'E' << 8) | (u16)'C')  /* Ecuador */
#define COUNTRY_CODE_EE              (((u16)'E' << 8) | (u16)'E')  /* Estonia */
#define COUNTRY_CODE_EG              (((u16)'E' << 8) | (u16)'G')  /* Egypt */
#define COUNTRY_CODE_EH \
    (((u16)'E' << 8) | (u16)'H')  /* Western Sahara (Morocco) */
#define COUNTRY_CODE_ER              (((u16)'E' << 8) | (u16)'R')  /* Eritrea */
#define COUNTRY_CODE_ES              (((u16)'E' << 8) | (u16)'S')  /* Spain */
#define COUNTRY_CODE_ET              (((u16)'E' << 8) | (u16)'T')  /* Ethiopia */
#define COUNTRY_CODE_EU              (((u16)'E' << 8) | (u16)'U')  /* Europe */
#define COUNTRY_CODE_FI              (((u16)'F' << 8) | (u16)'I')  /* Finland */
#define COUNTRY_CODE_FJ              (((u16)'F' << 8) | (u16)'J')  /* Fiji */
#define COUNTRY_CODE_FK              (((u16)'F' << 8) | (u16)'K')  /* Falkland Island */
#define COUNTRY_CODE_FM              (((u16)'F' << 8) | (u16)'M')  /* Micronesia */
#define COUNTRY_CODE_FO              (((u16)'F' << 8) | (u16)'O')  /* Faroe Island */
#define COUNTRY_CODE_FR              (((u16)'F' << 8) | (u16)'R')  /* France */
#define COUNTRY_CODE_FR \
    (((u16)'F' << 8) | (u16)'R')  /* Wallis and Futuna (France) */
#define COUNTRY_CODE_GA              (((u16)'G' << 8) | (u16)'A')  /* Gabon */
#define COUNTRY_CODE_GB              (((u16)'G' << 8) | (u16)'B')  /* United Kingdom */
#define COUNTRY_CODE_GD              (((u16)'G' << 8) | (u16)'D')  /* Grenada */
#define COUNTRY_CODE_GE              (((u16)'G' << 8) | (u16)'E')  /* Georgia */
#define COUNTRY_CODE_GF              (((u16)'G' << 8) | (u16)'F')  /* French Guiana */
#define COUNTRY_CODE_GG              (((u16)'G' << 8) | (u16)'G')  /* Guernsey */
#define COUNTRY_CODE_GH              (((u16)'G' << 8) | (u16)'H')  /* Ghana */
#define COUNTRY_CODE_GI              (((u16)'G' << 8) | (u16)'I')  /* Gibraltar */
#define COUNTRY_CODE_GM              (((u16)'G' << 8) | (u16)'M')  /* Gambia */
#define COUNTRY_CODE_GN              (((u16)'G' << 8) | (u16)'N')  /* Guinea */
#define COUNTRY_CODE_GP              (((u16)'G' << 8) | (u16)'P')  /* Guadeloupe */
#define COUNTRY_CODE_GQ              (((u16)'G' << 8) | (u16)'Q')  /* Equatorial Guinea */
#define COUNTRY_CODE_GR              (((u16)'G' << 8) | (u16)'R')  /* Greece */
#define COUNTRY_CODE_GT              (((u16)'G' << 8) | (u16)'T')  /* Guatemala */
#define COUNTRY_CODE_GU              (((u16)'G' << 8) | (u16)'U')  /* Guam */
#define COUNTRY_CODE_GW              (((u16)'G' << 8) | (u16)'W')  /* Guinea-Bissau */
#define COUNTRY_CODE_GY              (((u16)'G' << 8) | (u16)'Y')  /* Guyana */
#define COUNTRY_CODE_HK              (((u16)'H' << 8) | (u16)'K')  /* Hong Kong */
#define COUNTRY_CODE_HN              (((u16)'H' << 8) | (u16)'N')  /* Honduras */
#define COUNTRY_CODE_HR              (((u16)'H' << 8) | (u16)'R')  /* Croatia */
#define COUNTRY_CODE_HT              (((u16)'H' << 8) | (u16)'T')  /* Haiti */
#define COUNTRY_CODE_HU              (((u16)'H' << 8) | (u16)'U')  /* Hungary */
#define COUNTRY_CODE_ID              (((u16)'I' << 8) | (u16)'D')  /* Indonesia */
#define COUNTRY_CODE_IE              (((u16)'I' << 8) | (u16)'E')  /* Ireland */
#define COUNTRY_CODE_IL              (((u16)'I' << 8) | (u16)'L')  /* Israel */
#define COUNTRY_CODE_IM              (((u16)'I' << 8) | (u16)'M')  /* Isle of Man */
#define COUNTRY_CODE_IN              (((u16)'I' << 8) | (u16)'N')  /* India */
#define COUNTRY_CODE_IQ              (((u16)'I' << 8) | (u16)'Q')  /* Iraq */
#define COUNTRY_CODE_IR              (((u16)'I' << 8) | (u16)'R')  /* Iran */
#define COUNTRY_CODE_IS              (((u16)'I' << 8) | (u16)'S')  /* Iceland */
#define COUNTRY_CODE_IT              (((u16)'I' << 8) | (u16)'T')  /* Italy */
#define COUNTRY_CODE_JE              (((u16)'J' << 8) | (u16)'E')  /* Jersey */
#define COUNTRY_CODE_JM              (((u16)'J' << 8) | (u16)'M')  /* Jameica */
#define COUNTRY_CODE_JO              (((u16)'J' << 8) | (u16)'O')  /* Jordan */
#define COUNTRY_CODE_JP              (((u16)'J' << 8) | (u16)'P')  /* Japan */
#define COUNTRY_CODE_KE              (((u16)'K' << 8) | (u16)'E')  /* Kenya */
#define COUNTRY_CODE_KG              (((u16)'K' << 8) | (u16)'G')  /* Kyrgyzstan */
#define COUNTRY_CODE_KH              (((u16)'K' << 8) | (u16)'H')  /* Cambodia */
#define COUNTRY_CODE_KI              (((u16)'K' << 8) | (u16)'I')  /* Kiribati */
#define COUNTRY_CODE_KM              (((u16)'K' << 8) | (u16)'M')  /* Comoros */
#define COUNTRY_CODE_KN              (((u16)'K' << 8) | (u16)'N')  /* Saint Kitts and Nevis */
#define COUNTRY_CODE_KP              (((u16)'K' << 8) | (u16)'P')  /* North Korea */
#define COUNTRY_CODE_KR              (((u16)'K' << 8) | (u16)'R')  /* South Korea */
#define COUNTRY_CODE_KW              (((u16)'K' << 8) | (u16)'W')  /* Kuwait */
#define COUNTRY_CODE_KY              (((u16)'K' << 8) | (u16)'Y')  /* Cayman Islands */
#define COUNTRY_CODE_KZ              (((u16)'K' << 8) | (u16)'Z')  /* Kazakhstan */
#define COUNTRY_CODE_LA              (((u16)'L' << 8) | (u16)'A')  /* Laos */
#define COUNTRY_CODE_LB              (((u16)'L' << 8) | (u16)'B')  /* Lebanon */
#define COUNTRY_CODE_LC              (((u16)'L' << 8) | (u16)'C')  /* Saint Lucia */
#define COUNTRY_CODE_LI              (((u16)'L' << 8) | (u16)'I')  /* Liechtenstein */
#define COUNTRY_CODE_LK              (((u16)'L' << 8) | (u16)'K')  /* Sri Lanka */
#define COUNTRY_CODE_LR              (((u16)'L' << 8) | (u16)'R')  /* Liberia */
#define COUNTRY_CODE_LS              (((u16)'L' << 8) | (u16)'S')  /* Lesotho */
#define COUNTRY_CODE_LT              (((u16)'L' << 8) | (u16)'T')  /* Lithuania */
#define COUNTRY_CODE_LU              (((u16)'L' << 8) | (u16)'U')  /* Luxemburg */
#define COUNTRY_CODE_LV              (((u16)'L' << 8) | (u16)'V')  /* Latvia */
#define COUNTRY_CODE_LY              (((u16)'L' << 8) | (u16)'Y')  /* Libya */
#define COUNTRY_CODE_MA              (((u16)'M' << 8) | (u16)'A')  /* Morocco */
#define COUNTRY_CODE_MC              (((u16)'M' << 8) | (u16)'C')  /* Monaco */
#define COUNTRY_CODE_MD              (((u16)'M' << 8) | (u16)'D')  /* Moldova */
#define COUNTRY_CODE_ME              (((u16)'M' << 8) | (u16)'E')  /* Montenegro */
#define COUNTRY_CODE_MF \
    (((u16)'M' << 8) | (u16)'F')  /* Saint Martin / Sint Marteen */
/*(Added on window's list) */
#define COUNTRY_CODE_MG              (((u16)'M' << 8) | (u16)'G')  /* Madagascar */
#define COUNTRY_CODE_MH              (((u16)'M' << 8) | (u16)'H')  /* Marshall Islands */
#define COUNTRY_CODE_MK              (((u16)'M' << 8) | (u16)'K')  /* Macedonia */
#define COUNTRY_CODE_ML              (((u16)'M' << 8) | (u16)'L')  /* Mali */
#define COUNTRY_CODE_MM              (((u16)'M' << 8) | (u16)'M')  /* Myanmar */
#define COUNTRY_CODE_MN              (((u16)'M' << 8) | (u16)'N')  /* Mongolia */
#define COUNTRY_CODE_MO              (((u16)'M' << 8) | (u16)'O')  /* Macao */
#define COUNTRY_CODE_MP              (((u16)'M' << 8) | (u16)'P')
/* Northern Mariana Islands (Rota Island / Saipan and Tinian Island) */
#define COUNTRY_CODE_MQ              (((u16)'M' << 8) | (u16)'Q')  /* Martinique (France) */
#define COUNTRY_CODE_MR              (((u16)'M' << 8) | (u16)'R')  /* Mauritania */
#define COUNTRY_CODE_MS              (((u16)'M' << 8) | (u16)'S')  /* Montserrat (UK) */
#define COUNTRY_CODE_MT              (((u16)'M' << 8) | (u16)'T')  /* Malta */
#define COUNTRY_CODE_MU              (((u16)'M' << 8) | (u16)'U')  /* Mauritius */
#define COUNTRY_CODE_MV              (((u16)'M' << 8) | (u16)'V')  /* Maldives */
#define COUNTRY_CODE_MW              (((u16)'M' << 8) | (u16)'W')  /* Malawi */
#define COUNTRY_CODE_MX              (((u16)'M' << 8) | (u16)'X')  /* Mexico */
#define COUNTRY_CODE_MY              (((u16)'M' << 8) | (u16)'Y')  /* Malaysia */
#define COUNTRY_CODE_MZ              (((u16)'M' << 8) | (u16)'Z')  /* Mozambique */
#define COUNTRY_CODE_NA              (((u16)'N' << 8) | (u16)'A')  /* Namibia */
#define COUNTRY_CODE_NC              (((u16)'N' << 8) | (u16)'C')  /* New Caledonia */
#define COUNTRY_CODE_NE              (((u16)'N' << 8) | (u16)'E')  /* Niger */
#define COUNTRY_CODE_NF              (((u16)'N' << 8) | (u16)'F')  /* Norfolk Island */
#define COUNTRY_CODE_NG              (((u16)'N' << 8) | (u16)'G')  /* Nigeria */
#define COUNTRY_CODE_NI              (((u16)'N' << 8) | (u16)'I')  /* Nicaragua */
#define COUNTRY_CODE_NL              (((u16)'N' << 8) | (u16)'L')  /* Netherlands */
#define COUNTRY_CODE_NO              (((u16)'N' << 8) | (u16)'O')  /* Norway */
#define COUNTRY_CODE_NP              (((u16)'N' << 8) | (u16)'P')  /* Nepal */
#define COUNTRY_CODE_NR              (((u16)'N' << 8) | (u16)'R')  /* Nauru */
#define COUNTRY_CODE_NU              (((u16)'N' << 8) | (u16)'U')  /* Niue */
#define COUNTRY_CODE_NZ              (((u16)'N' << 8) | (u16)'Z')  /* New Zealand */
#define COUNTRY_CODE_OM              (((u16)'O' << 8) | (u16)'M')  /* Oman */
#define COUNTRY_CODE_PA              (((u16)'P' << 8) | (u16)'A')  /* Panama */
#define COUNTRY_CODE_PE              (((u16)'P' << 8) | (u16)'E')  /* Peru */
#define COUNTRY_CODE_PF              (((u16)'P' << 8) | (u16)'F')  /* "French Polynesia */
#define COUNTRY_CODE_PG              (((u16)'P' << 8) | (u16)'G')  /* Papua New Guinea */
#define COUNTRY_CODE_PH              (((u16)'P' << 8) | (u16)'H')  /* Philippines */
#define COUNTRY_CODE_PK              (((u16)'P' << 8) | (u16)'K')  /* Pakistan */
#define COUNTRY_CODE_PL              (((u16)'P' << 8) | (u16)'L')  /* Poland */
#define COUNTRY_CODE_PM \
    (((u16)'P' << 8) | (u16)'M')  /* Saint Pierre and Miquelon */
#define COUNTRY_CODE_PN              (((u16)'P' << 8) | (u16)'N')  /* Pitcairn Islands */
#define COUNTRY_CODE_PR              (((u16)'P' << 8) | (u16)'R')  /* Puerto Rico (USA) */
#define COUNTRY_CODE_PS              (((u16)'P' << 8) | (u16)'S')
/* Palestinian Authority */
#define COUNTRY_CODE_PT              (((u16)'P' << 8) | (u16)'T')  /* Portugal */
#define COUNTRY_CODE_PW              (((u16)'P' << 8) | (u16)'W')  /* Palau */
#define COUNTRY_CODE_PY              (((u16)'P' << 8) | (u16)'Y')  /* Paraguay */
#define COUNTRY_CODE_QA              (((u16)'Q' << 8) | (u16)'A')  /* Qatar */
#define COUNTRY_CODE_RE              (((u16)'R' << 8) | (u16)'E')  /* Reunion (France) */
#define COUNTRY_CODE_RKS \
    (((u16)'R' << 8) | (u16)'K')  /* Kosvo (Added on window's list) */
#define COUNTRY_CODE_RO              (((u16)'R' << 8) | (u16)'O')  /* Romania */
#define COUNTRY_CODE_RS              (((u16)'R' << 8) | (u16)'S')  /* Serbia */
#define COUNTRY_CODE_RU              (((u16)'R' << 8) | (u16)'U')  /* Russia */
#define COUNTRY_CODE_RW              (((u16)'R' << 8) | (u16)'W')  /* Rwanda */
#define COUNTRY_CODE_SA              (((u16)'S' << 8) | (u16)'A')  /* Saudi Arabia */
#define COUNTRY_CODE_SB              (((u16)'S' << 8) | (u16)'B')  /* Solomon Islands */
#define COUNTRY_CODE_SC              (((u16)'S' << 8) | (u16)'C')  /* Seychelles */
#define COUNTRY_CODE_SD              (((u16)'S' << 8) | (u16)'D')  /* Sudan */
#define COUNTRY_CODE_SE              (((u16)'S' << 8) | (u16)'E')  /* Sweden */
#define COUNTRY_CODE_SG              (((u16)'S' << 8) | (u16)'G')  /* Singapole */
#define COUNTRY_CODE_SI              (((u16)'S' << 8) | (u16)'I')  /* Slovenia */
#define COUNTRY_CODE_SK              (((u16)'S' << 8) | (u16)'K')  /* Slovakia */
#define COUNTRY_CODE_SL              (((u16)'S' << 8) | (u16)'L')  /* Sierra Leone */
#define COUNTRY_CODE_SM              (((u16)'S' << 8) | (u16)'M')  /* San Marino */
#define COUNTRY_CODE_SN              (((u16)'S' << 8) | (u16)'N')  /* Senegal */
#define COUNTRY_CODE_SO              (((u16)'S' << 8) | (u16)'O')  /* Somalia */
#define COUNTRY_CODE_SR              (((u16)'S' << 8) | (u16)'R')  /* Suriname */
#define COUNTRY_CODE_SS              (((u16)'S' << 8) | (u16)'S')  /* South_Sudan */
#define COUNTRY_CODE_ST              (((u16)'S' << 8) | (u16)'T')
/* Sao Tome and Principe */
#define COUNTRY_CODE_SV              (((u16)'S' << 8) | (u16)'V')  /* El Salvador */
#define COUNTRY_CODE_SY              (((u16)'S' << 8) | (u16)'Y')  /* Syria */
#define COUNTRY_CODE_SZ              (((u16)'S' << 8) | (u16)'Z')  /* Swaziland */
#define COUNTRY_CODE_TC \
    (((u16)'T' << 8) | (u16)'C')  /* Turks and Caicos Islands (UK) */
#define COUNTRY_CODE_TD              (((u16)'T' << 8) | (u16)'D')  /* Chad */
#define COUNTRY_CODE_TF \
    (((u16)'T' << 8) | (u16)'F')  /* French Southern and Antarctic Lands */
#define COUNTRY_CODE_TG              (((u16)'T' << 8) | (u16)'G')  /* Togo */
#define COUNTRY_CODE_TH              (((u16)'T' << 8) | (u16)'H')  /* Thailand */
#define COUNTRY_CODE_TJ              (((u16)'T' << 8) | (u16)'J')  /* Tajikistan */
#define COUNTRY_CODE_TL              (((u16)'T' << 8) | (u16)'L')  /* East Timor */
#define COUNTRY_CODE_TM              (((u16)'T' << 8) | (u16)'M')  /* Turkmenistan */
#define COUNTRY_CODE_TN              (((u16)'T' << 8) | (u16)'N')  /* Tunisia */
#define COUNTRY_CODE_TO              (((u16)'T' << 8) | (u16)'O')  /* Tonga */
#define COUNTRY_CODE_TR              (((u16)'T' << 8) | (u16)'R')  /* Turkey */
#define COUNTRY_CODE_TT              (((u16)'T' << 8) | (u16)'T')  /* Trinidad and Tobago */
#define COUNTRY_CODE_TV              (((u16)'T' << 8) | (u16)'V')  /* Tuvalu */
#define COUNTRY_CODE_TW              (((u16)'T' << 8) | (u16)'W')  /* Taiwan */
#define COUNTRY_CODE_TZ              (((u16)'T' << 8) | (u16)'Z')  /* Tanzania */
#define COUNTRY_CODE_UA              (((u16)'U' << 8) | (u16)'A')  /* Ukraine */
#define COUNTRY_CODE_UG              (((u16)'U' << 8) | (u16)'G')  /* Ugnada */
#define COUNTRY_CODE_US              (((u16)'U' << 8) | (u16)'S')  /* US */
#define COUNTRY_CODE_UY              (((u16)'U' << 8) | (u16)'Y')  /* Uruguay */
#define COUNTRY_CODE_UZ              (((u16)'U' << 8) | (u16)'Z')  /* Uzbekistan */
#define COUNTRY_CODE_VA              (((u16)'V' << 8) | (u16)'A')  /* Vatican (Holy See) */
#define COUNTRY_CODE_VC \
    (((u16)'V' << 8) | (u16)'C')  /* Saint Vincent and the Grenadines */
#define COUNTRY_CODE_VE              (((u16)'V' << 8) | (u16)'E')  /* Venezuela */
#define COUNTRY_CODE_VG \
    (((u16)'V' << 8) | (u16)'G')  /* British Virgin Islands */
#define COUNTRY_CODE_VI              (((u16)'V' << 8) | (u16)'I')  /* US Virgin Islands */
#define COUNTRY_CODE_VN              (((u16)'V' << 8) | (u16)'N')  /* Vietnam */
#define COUNTRY_CODE_VU              (((u16)'V' << 8) | (u16)'U')  /* Vanuatu */
#define COUNTRY_CODE_WS              (((u16)'W' << 8) | (u16)'S')  /* Samoa */
#define COUNTRY_CODE_YE              (((u16)'Y' << 8) | (u16)'E')  /* Yemen */
#define COUNTRY_CODE_YT              (((u16)'Y' << 8) | (u16)'T')  /* Mayotte (France) */
#define COUNTRY_CODE_ZA              (((u16)'Z' << 8) | (u16)'A')  /* South Africa */
#define COUNTRY_CODE_ZM              (((u16)'Z' << 8) | (u16)'M')  /* Zambia */
#define COUNTRY_CODE_ZW              (((u16)'Z' << 8) | (u16)'W')  /* Zimbabwe */
#define COUNTRY_CODE_DF \
    (((u16)'D' << 8) | (u16)'F')  /* Default country domain  */
#define COUNTRY_CODE_WW              (((u16)'0' << 8) | (u16)'0')  /* World Wide  */

/* dot11RegDomainsSupportValue */
#define MIB_REG_DOMAIN_FCC           0x10  /* FCC (US) */
#define MIB_REG_DOMAIN_IC            0x20  /* IC or DOC (Canada) */
#define MIB_REG_DOMAIN_ETSI          0x30  /* ETSI (Europe) */
#define MIB_REG_DOMAIN_SPAIN             0x31  /* Spain */
#define MIB_REG_DOMAIN_FRANCE            0x32  /* France */
#define MIB_REG_DOMAIN_JAPAN             0x40  /* MPHPT (Japan) */
#define MIB_REG_DOMAIN_OTHER             0x00  /* other */

/*2.4G*/
#define BAND_2G4_LOWER_BOUND             1
#define BAND_2G4_UPPER_BOUND             14
/*5G SubBand FCC spec*/
#define UNII1_LOWER_BOUND            36
#define UNII1_UPPER_BOUND            48
#define UNII2A_LOWER_BOUND           52
#define UNII2A_UPPER_BOUND           64
#define UNII2C_LOWER_BOUND           100
#define UNII2C_UPPER_BOUND           144
#define UNII3_LOWER_BOUND            149
#define UNII3_UPPER_BOUND            173

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY

#define POWER_LIMIT_TABLE_NULL           0xFFFF
#define MAX_TX_POWER                 63
#define MIN_TX_POWER                 -64
#define MAX_CMD_SUPPORT_CHANNEL_NUM      64

#endif

#if (CFG_SUPPORT_SINGLE_SKU == 1)
#define MAX_SUPPORTED_CH_COUNT           MAX_CHN_NUM
#define REG_RULE_LIGHT(start, end, bw, reg_flags) \
    REG_RULE(start, end, bw, 0, 0, reg_flags)
#define TX_PWR_LIMIT_CMD_CH_NUM_THRESHOLD    32
#endif
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY

typedef enum _ENUM_POWER_LIMIT_T {
    PWR_LIMIT_CCK  = 0,
    PWR_LIMIT_20M  = 1,
    PWR_LIMIT_40M  = 2,
    PWR_LIMIT_80M  = 3,
    PWR_LIMIT_160M = 4,
    PWR_LIMIT_NUM
} ENUM_POWER_LIMIT_T,
*P_ENUM_POWER_LIMIT_T;

#endif

typedef enum _ENUM_POWER_LIMIT_SUBBAND_T {
    POWER_LIMIT_2G4    = 0,
    POWER_LIMIT_UNII1  = 1,
    POWER_LIMIT_UNII2A = 2,
    POWER_LIMIT_UNII2C = 3,
    POWER_LIMIT_UNII3  = 4,
    POWER_LIMIT_SUBAND_NUM
} ENUM_POWER_LIMIT_SUBBAND_T,
*P_ENUM_POWER_LIMIT_SUBBAND_T;

/* Define channel offset in unit of 5MHz bandwidth */
typedef enum _ENUM_CHNL_SPAN_T {
    CHNL_SPAN_5  = 1,
    CHNL_SPAN_10 = 2,
    CHNL_SPAN_20 = 4,
    CHNL_SPAN_40 = 8
} ENUM_CHNL_SPAN_T,
*P_ENUM_CHNL_SPAN_T;

/* Define BSS operating bandwidth */
typedef enum _ENUM_CHNL_BW_T {
    CHNL_BW_20,
    CHNL_BW_20_40,
    CHNL_BW_10,
    CHNL_BW_5
} ENUM_CHNL_BW_T,
*P_ENUM_CHNL_BW_T;

/* In all bands, the first channel will be SCA and the second channel is SCB,
 * then iteratively.
 * Note the final channel will not be SCA.
 */
typedef struct _DOMAIN_SUBBAND_INFO {
    /* Note1: regulation class depends on operation bandwidth and RF band.
     *  For example: 2.4GHz, 1~13, 20MHz ==> regulation class = 81
     *               2.4GHz, 1~13, SCA   ==> regulation class = 83
     *               2.4GHz, 1~13, SCB   ==> regulation class = 84
     * Note2: TX power limit is not specified here because path loss is
     * unknown
     */
    u8 ucRegClass;  /* Regulation class for 20MHz */
    u8 ucBand;  /* Type: ENUM_BAND_T */
    u8 ucChannelSpan;  /* Type: ENUM_CHNL_SPAN_T */
    u8 ucFirstChannelNum;
    u8 ucNumChannels;
    u8 fgDfs;  /* Type: u8*/
} DOMAIN_SUBBAND_INFO, *P_DOMAIN_SUBBAND_INFO;

/* Use it as all available channel list for STA */
typedef struct _DOMAIN_INFO_ENTRY {
    u16 *pu2CountryGroup;
    u32 u4CountryNum;

    /* If different attributes, put them into different rSubBands.
     * For example, DFS shall be used or not.
     */
    DOMAIN_SUBBAND_INFO rSubBand[MAX_SUBBAND_NUM];
} DOMAIN_INFO_ENTRY, *P_DOMAIN_INFO_ENTRY;

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY

#if (CFG_SUPPORT_SINGLE_SKU == 1)
/*
 * MT_TxPwrLimit.dat format
 */
#define SECTION_PREFIX             (0x23232323)
#define ELEMENT_PREFIX             (0xffff)
#define VERSION                (0x00000001)
#define SIZE_OF_VERSION            4
#if CFG_SUPPORT_LARGE_TX_PWR_LIMIT_TABLE
#define WLAN_TX_PWR_LIMIT_FILE_BUF_SIZE    (204800 * 2)
#else
#define WLAN_TX_PWR_LIMIT_FILE_BUF_SIZE    204800
#endif
#define WLAN_TX_PWR_LIMIT_FILE_NAME    "TxPwrLimit_MT76x8.dat"

struct tx_pwr_element {
    u16 prefix;
    u8 channel_num;
    u8 reserved;

    /*the followings are in unit: 0.5 dbm*/

    u8 tx_pwr_dsss_cck;
    u8 tx_pwr_dsss_bpsk;

    u8 tx_pwr_ofdm_bpsk;  /* 6M, 9M */
    u8 tx_pwr_ofdm_qpsk;  /* 12M, 18M */
    u8 tx_pwr_ofdm_16qam;  /* 24M, 36M */
    u8 tx_pwr_ofdm_48m;
    u8 tx_pwr_ofdm_54m;

    u8 tx_pwr_ht20_bpsk;  /* MCS0*/
    u8 tx_pwr_ht20_qpsk;  /* MCS1, MCS2*/
    u8 tx_pwr_ht20_16qam;  /* MCS3, MCS4*/
    u8 tx_pwr_ht20_mcs5;  /* MCS5*/
    u8 tx_pwr_ht20_mcs6;  /* MCS6*/
    u8 tx_pwr_ht20_mcs7;  /* MCS7*/

    u8 tx_pwr_ht40_bpsk;  /* MCS0*/
    u8 tx_pwr_ht40_qpsk;  /* MCS1, MCS2*/
    u8 tx_pwr_ht40_16qam;  /* MCS3, MCS4*/
    u8 tx_pwr_ht40_mcs5;  /* MCS5*/
    u8 tx_pwr_ht40_mcs6;  /* MCS6*/
    u8 tx_pwr_ht40_mcs7;  /* MCS7*/

    u8 tx_pwr_vht20_bpsk;  /* MCS0*/
    u8 tx_pwr_vht20_qpsk;  /* MCS1, MCS2*/
    u8 tx_pwr_vht20_16qam;  /* MCS3, MCS4*/
    u8 tx_pwr_vht20_64qam;  /* MCS5, MCS6*/
    u8 tx_pwr_vht20_mcs7;
    u8 tx_pwr_vht20_mcs8;
    u8 tx_pwr_vht20_mcs9;

    u8 tx_pwr_vht_40;
    u8 tx_pwr_vht_80;
    u8 tx_pwr_vht_160nc;
    u8 tx_pwr_lg_40;
    u8 tx_pwr_lg_80;

    u8 tx_pwr_1ss_delta;
    u8 reserved_3[3];
};

struct tx_pwr_section {
    u32 prefix;
    u32 country_code;
};
#endif

/* CMD_SET_PWR_LIMIT_TABLE */
typedef struct _CHANNEL_POWER_LIMIT {
    u8 ucCentralCh;
    s8 cPwrLimitCCK;
    s8 cPwrLimit20;
    s8 cPwrLimit40;
    s8 cPwrLimit80;
    s8 cPwrLimit160;
    u8 ucFlag;
    u8 aucReserved[1];
} CHANNEL_POWER_LIMIT, *P_CHANNEL_POWER_LIMIT;

typedef struct _COUNTRY_CHANNEL_POWER_LIMIT {
    u8 aucCountryCode[2];
    u8 ucCountryFlag;
    u8 ucChannelNum;
    u8 aucReserved[4];
    CHANNEL_POWER_LIMIT rChannelPowerLimit[80];
} COUNTRY_CHANNEL_POWER_LIMIT, *P_COUNTRY_CHANNEL_POWER_LIMIT;

#define CHANNEL_PWR_LIMIT(_channel, _pwrLimit_cck, _pwrLimit_bw20,         \
                          _pwrLimit_bw40, _pwrLimit_bw80, _pwrLimit_bw160, \
                          _ucFlag)                                         \
    {                                                                      \
        .ucCentralCh = (_channel), .cPwrLimitCCK = (_pwrLimit_cck),        \
        .cPwrLimit20 = (_pwrLimit_bw20),                                   \
        .cPwrLimit40 = (_pwrLimit_bw40),                                   \
        .cPwrLimit80 = (_pwrLimit_bw80),                                   \
        .cPwrLimit160 = (_pwrLimit_bw160), .ucFlag = (_ucFlag),            \
        .aucReserved = {                                                   \
            0                                                              \
        }                                                                  \
    }

typedef struct _COUNTRY_POWER_LIMIT_TABLE_DEFAULT {
    u8 aucCountryCode[2];
    /* 0: ch 1 ~14 , 1: ch 36 ~48, 2: ch 52 ~64, 3: ch 100 ~144, 4: ch 149
     * ~165 */
    s8 aucPwrLimitSubBand[POWER_LIMIT_SUBAND_NUM];
    /* bit0: cPwrLimit2G4, bit1: cPwrLimitUnii1; bit2: cPwrLimitUnii2A;*/
    /* bit3: cPwrLimitUnii2C; bit4: cPwrLimitUnii3; mW: 0, mW\MHz : 1 */
    u8 ucPwrUnit;
} COUNTRY_POWER_LIMIT_TABLE_DEFAULT, *P_COUNTRY_POWER_LIMIT_TABLE_DEFAULT;

typedef struct _COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION {
    u8 aucCountryCode[2];
    u8 ucCentralCh;
    s8 aucPwrLimit[PWR_LIMIT_NUM];
} COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION,
*P_COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION;

typedef struct _SUBBAND_CHANNEL_T {
    u8 ucStartCh;
    u8 ucEndCh;
    u8 ucInterval;
    u8 ucReserved;
} SUBBAND_CHANNEL_T, *P_SUBBAND_CHANNEL_T;

#endif

#if (CFG_SUPPORT_SINGLE_SKU == 1)
/*
 * Event from chip for single sku
 */
typedef struct _SINGLE_SKU_INFO {
    u32 u4EfuseCountryCode;
    u8 isEfuseValid;
    u8 ucReserved[7];
} SINGLE_SKU_INFO, *P_SINGLE_SKU_INFO;

/*
 * channel structure
 */
struct channel {
    u16 chNum;
    u8 reserved[2];
    u32 flags;  /*enum ieee80211_channel_flags*/
};

struct acctive_channel_list {
    u8 n_channels_2g;
    u8 n_channels_5g;
    u8 ucReserved[2];
    struct channel channels[0];
};

/*
 * single sku control structure
 */
enum regd_state {
    REGD_STATE_UNDEFINED,
    REGD_STATE_INIT,
    REGD_STATE_SET_WW_CORE,
    REGD_STATE_SET_COUNTRY_USER,
    REGD_STATE_SET_COUNTRY_DRIVER,
    REGD_STATE_SET_COUNTRY_IE,
    REGD_STATE_INVALID
};

enum regd_control_flag {
    REGD_CTRL_FLAG_SUPPORT_LOCAL_REGD_DB = (0x1 << 0)
};

typedef struct mtk_regd_control_t {
    u8 en;
    u8 isEfuseCountryCodeUsed;
    enum regd_state state;
    u32 alpha2;
    u32 tmp_alpha2;  /*store country code set by iwpriv "country XX"*/
    u32 flag;  /*enum regd_control_flag*/
    P_GLUE_INFO_T pGlueInfo;  /*wlan GlueInfo*/
    u8 n_channel_active_2g;
    u8 n_channel_active_5g;
    struct channel channels[MAX_SUPPORTED_CH_COUNT];
    enum nl80211_dfs_regions dfs_region;
} mtk_regd_control;

#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
struct mtk_regdomain {
    char country_code[4];
    const struct ieee80211_regdomain *prRegdRules;
};
#endif

#endif
/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#define CAL_CH_OFFSET_80M(_PRIMARY_CH, _CENTRAL_CH) \
    (((_PRIMARY_CH - _CENTRAL_CH) + 6) >> 2)

#define CAL_CH_OFFSET_160M(_PRIMARY_CH, _CENTRAL_CH) \
    (((_PRIMARY_CH - _CENTRAL_CH) + 14) >> 2)

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

P_DOMAIN_INFO_ENTRY rlmDomainGetDomainInfo(P_ADAPTER_T prAdapter);

void rlmDomainGetChnlList(P_ADAPTER_T prAdapter,
                          ENUM_BAND_T eSpecificBand,
                          u8 fgNoDfs,
                          u8 ucMaxChannelNum,
                          u8 *pucNumOfChannel,
                          P_RF_CHANNEL_INFO_T paucChannelList);

#ifdef CFG_SUPPORT_SAP_DFS_CHANNEL
void rlmDomainGetDfsChnls(P_ADAPTER_T prAdapter,
                          u8 ucMaxChannelNum,
                          u8 *pucNumOfChannel,
                          P_RF_CHANNEL_INFO_T paucChannelList);
#endif

void rlmDomainSendCmd(P_ADAPTER_T prAdapter, u8 fgIsOid);

void rlmDomainSendDomainInfoCmd(P_ADAPTER_T prAdapter, u8 fgIsOid);

void rlmDomainSendPassiveScanInfoCmd(P_ADAPTER_T prAdapter, u8 fgIsOid);

u32 rlmDomainSupOperatingClassIeFill(u8 *pBuf);

u8 rlmDomainCheckChannelEntryValid(P_ADAPTER_T prAdapter, u8 ucCentralCh);

u8 rlmDomainGetCenterChannel(ENUM_BAND_T eBand,
                             u8 ucPriChannel,
                             ENUM_CHNL_EXT_T eExtend);

u8 rlmDomainIsValidRfSetting(P_ADAPTER_T prAdapter,
                             ENUM_BAND_T eBand,
                             u8 ucPriChannel,
                             ENUM_CHNL_EXT_T eExtend,
                             ENUM_CHANNEL_WIDTH_T eChannelWidth,
                             u8 ucChannelS1,
                             u8 ucChannelS2);

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY

u8 rlmDomainCheckPowerLimitValid(P_ADAPTER_T prAdapter,
                                 COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION
                                 rPowerLimitTableConfiguration,
                                 u8 ucPwrLimitNum);

void rlmDomainCheckCountryPowerLimitTable(P_ADAPTER_T prAdapter);

u16 rlmDomainPwrLimitDefaultTableDecision(P_ADAPTER_T prAdapter,
                                          u16 u2CountryCode);

void rlmDomainSendPwrLimitCmd(P_ADAPTER_T prAdapter);
#endif

#if (CFG_SUPPORT_SINGLE_SKU == 1)
extern struct ieee80211_supported_band mtk_band_2ghz;
extern struct ieee80211_supported_band mtk_band_5ghz;

u8 rlmDomainIsCtrlStateEqualTo(enum regd_state state);
u8 rlmDomainIsUsingLocalRegDomainDataBase(void);
enum regd_state rlmDomainStateTransition(enum regd_state request_state,
                                         struct regulatory_request *pRequest);
void rlmDomainSetCountryCode(char *alpha2, u8 size_of_alpha2);
void rlmDomainSetDfsRegion(enum nl80211_dfs_regions dfs_region);
enum nl80211_dfs_regions rlmDomainGetDfsRegion(void);
void rlmDomainResetCtrlInfo(u8 force);
void rlmDomainAddActiveChannel(u8 band);
u8 rlmDomainGetActiveChannelCount(u8 band);
void rlmDomainParsingChannel(IN struct wiphy *pWiphy);
struct channel *rlmDomainGetActiveChannels(void);
void rlmExtractChannelInfo(u32 max_ch_count,
                           struct acctive_channel_list *prBuff);
void regd_set_using_local_regdomain_db(void);
void rlmDomainSetDefaultCountryCode(void);
enum regd_state rlmDomainGetCtrlState(void);
bool rlmDomainIsSameCountryCode(char *alpha2, u8 size_of_alpha2);
const struct ieee80211_regdomain *
rlmDomainSearchRegdomainFromLocalDataBase(char *alpha2);
P_GLUE_INFO_T rlmDomainGetGlueInfo(void);
bool rlmDomainIsEfuseUsed(void);
u8 rlmDomainGetChannelBw(u8 channelNum);
#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
extern const struct mtk_regdomain *g_prRegRuleTable[];
#endif

#endif

const struct ieee80211_regdomain *rlmDomainGetLocalDefaultRegd(void);
void rlmDomainSendInfoToFirmware(IN P_ADAPTER_T prAdapter);
WLAN_STATUS rlmDomainExtractSingleSkuInfoFromFirmware(IN P_ADAPTER_T prAdapter,
                                                      IN u8 *pucEventBuf,
                                                      IN u32 u4EventBufLen);
u8 regd_is_single_sku_en(void);
#ifdef CFG_SUPPORT_SAP_DFS_CHANNEL
u8 rlmDomainIsLegalDfsChannel(P_ADAPTER_T prAdapter,
                              ENUM_BAND_T eBand,
                              u8 ucChannel);
#endif
u8 rlmDomainIsLegalChannel(P_ADAPTER_T prAdapter, ENUM_BAND_T eBand,
                           u8 ucChannel);
ENUM_CHNL_EXT_T rlmSelectSecondaryChannelType(P_ADAPTER_T prAdapter,
                                              ENUM_BAND_T band,
                                              u8 primary_ch);
extern void mtk_reg_notify(IN struct wiphy *pWiphy,
                           IN struct regulatory_request *pRequest);
void rlmDomainOidSetCountry(IN P_ADAPTER_T prAdapter,
                            char *country,
                            u8 size_of_country);
u32 rlmDomainGetCountryCode(void);
u32 rlmDomainGetTempCountryCode(void);
void rlmDomainAssert(u8 cond);
void rlmDomainOverridePwrLimitFileName(const char *);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif
