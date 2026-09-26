/* -----------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright 2025 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung
e.V. All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software
that implements the MPEG Advanced Audio Coding ("AAC") encoding and decoding
scheme for digital audio. This FDK AAC Codec software is intended to be used on
a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient
general perceptual audio codecs. AAC-ELD is considered the best-performing
full-bandwidth communications codec by independent studies and is widely
deployed. AAC has been standardized by ISO and IEC as part of the MPEG
specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including
those of Fraunhofer) may be obtained through Via Licensing
(www.vialicensing.com) or through the respective patent owners individually for
the purpose of encoding or decoding bit streams in products that are compliant
with the ISO/IEC MPEG audio standards. Please note that most manufacturers of
Android devices already license these patent claims through Via Licensing or
directly from the patent owners, and therefore FDK AAC Codec software may
already be covered under those patent licenses when it is used for those
licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions
with enhanced sound quality, are also available from Fraunhofer. Users are
encouraged to check the Fraunhofer website for additional applications
information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees provided that you
satisfy the following conditions:

You must retain the complete text of this software license in redistributions of
the FDK AAC Codec or your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation
and/or other materials provided with redistributions of the FDK AAC Codec or
your modifications thereto in binary form. You must make available free of
charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived
from this library without prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute
the FDK AAC Codec software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating
that you changed the software and the date of any change. For modified versions
of the FDK AAC Codec, the term "Fraunhofer FDK AAC Codec Library for Android"
must be replaced by the term "Third-Party Modified Version of the Fraunhofer FDK
AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty of patent non-infringement with respect to this
software.

You may use this FDK AAC Codec software or modifications thereto only for
purposes that are authorized by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright
holders and contributors "AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
including but not limited to the implied warranties of merchantability and
fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary,
or consequential damages, including but not limited to procurement of substitute
goods or services; loss of use, data, or profits, or business interruption,
however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of
this software, even if advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------- */
//! Tables used by the arithmetic decoder

/// Probability models of the cumulative frequencies for the least significant
/// bit planes symbol r.
pub(super) static ARI_LSB2: [[i16; 4]; 3] = [
    [12571, 10569, 3696, 0],
    [12661, 5700, 3751, 0],
    [10827, 6884, 2929, 0],
];

/// Probability models of the cumulative frequencies for the concatenated 2 most significant bit (MSB)
/// planes and the ARITH_ESCAPE symbol
#[rustfmt::skip]
pub(super) static ARI_PK: [[i16; 17]; 64] = [
  [708   , 706   , 579   , 569   , 568   , 567   , 479   , 469   , 297   , 138   , 97    , 91    , 72    , 52    , 38    , 34    , 0] ,
  [7619  , 6917  , 6519  , 6412  , 5514  , 5003  , 4683  , 4563  , 3907  , 3297  , 3125  , 3060  , 2904  , 2718  , 2631  , 2590  , 0] ,
  [7263  , 4888  , 4810  , 4803  , 1889  , 415   , 335   , 327   , 195   , 72    , 52    , 49    , 36    , 20    , 15    , 14    , 0] ,
  [3626  , 2197  , 2188  , 2187  , 582   , 57    , 47    , 46    , 30    , 12    , 9     , 8     , 6     , 4     , 3     , 2     , 0] ,
  [7806  , 5541  , 5451  , 5441  , 2720  , 834   , 691   , 674   , 487   , 243   , 179   , 167   , 139   , 98    , 77    , 70    , 0] ,
  [6684  , 4101  , 4058  , 4055  , 1748  , 426   , 368   , 364   , 322   , 257   , 235   , 232   , 228   , 222   ,  217  , 215   , 0] ,
  [9162  , 5964  , 5831  , 5819  , 3269  , 866   , 658   , 638   , 535   , 348   , 258   , 244   , 234   , 214   , 195   , 186   , 0] ,
  [10638 , 8491  , 8365  , 8351  , 4418  , 2067  , 1859  , 1834  , 1190  , 601   , 495   , 478   , 356   , 217   , 174   , 164   , 0] ,
  [13389 , 10514 , 10032 , 9961  , 7166  , 3488  , 2655  , 2524  , 2015  , 1140  , 760   , 672   , 585   , 426   , 325   , 283   , 0] ,
  [14861 , 12788 , 12115 , 11952 , 9987  , 6657  , 5323  , 4984  , 4324  , 3001  , 2205  , 1943  , 1764  , 1394  , 1115  , 978   , 0] ,
  [12876 , 10004 , 9661  , 9610  , 7107  , 3435  , 2711  , 2595  , 2257  , 1508  , 1059  , 952   , 893   , 753   , 609   , 538   , 0] ,
  [15125 , 13591 , 13049 , 12874 , 11192 , 8543  , 7406  , 7023  , 6291  , 4922  , 4104  , 3769  , 3465  , 2890  , 2486  , 2275  , 0] ,
  [14574 , 13106 , 12731 , 12638 , 10453 , 7947  , 7233  , 7037  , 6031  , 4618  , 4081  , 3906  , 3465  , 2802  , 2476  , 2349  , 0] ,
  [15070 , 13179 , 12517 , 12351 , 10742 , 7657  , 6200  , 5825  , 5264  , 3998  , 3014  , 2662  , 2510  , 2153  , 1799  , 1564  , 0] ,
  [15542 , 14466 , 14007 , 13844 , 12489 , 10409 , 9481  , 9132  , 8305  , 6940  , 6193  , 5867  , 5458  , 4743  , 4291  , 4047  , 0] ,
  [15165 , 14384 , 14084 , 13934 , 12911 , 11485 , 10844 , 10513 , 10002 , 8993  , 8380  , 8051  , 7711  , 7036  , 6514  , 6233  , 0] ,
  [15642 , 14279 , 13625 , 13393 , 12348 , 9971  , 8405  , 7858  , 7335  , 6119  , 4918  , 4376  , 4185  , 3719  , 3231  , 2860  , 0] ,
  [13408 , 13407 , 11471 , 11218 , 11217 , 11216 , 9473  , 9216  , 6480  , 3689  , 2857  , 2690  , 2256  , 1732  , 1405  , 1302  , 0] ,
  [16098 , 15584 , 15191 , 14931 , 14514 , 13578 , 12703 , 12103 , 11830 , 11172 , 10475 , 9867  , 9695  , 9281  , 8825  , 8389  , 0] ,
  [15844 , 14873 , 14277 , 13996 , 13230 , 11535 , 10205 , 9543  , 9107  , 8086  , 7085  , 6419  , 6214  , 5713  , 5195  , 4731  , 0] ,
  [16131 , 15720 , 15443 , 15276 , 14848 , 13971 , 13314 , 12910 , 12591 , 11874 , 11225 , 10788 , 10573 , 10077 , 9585  , 9209  , 0] ,
  [16331 , 16330 , 12283 , 11435 , 11434 , 11433 , 8725  , 8049  , 6065  , 4138  , 3187  , 2842  , 2529  , 2171  , 1907  , 1745  , 0] ,
  [16011 , 15292 , 14782 , 14528 , 14008 , 12767 , 11556 , 10921 , 10591 , 9759  , 8813  , 8043  , 7855  , 7383  , 6863  , 6282  , 0] ,
  [16380 , 16379 , 15159 , 14610 , 14609 , 14608 , 12859 , 12111 , 11046 , 9536  , 8348  , 7713  , 7216  , 6533  , 5964  , 5546  , 0] ,
  [16367 , 16333 , 16294 , 16253 , 16222 , 16143 , 16048 , 15947 , 15915 , 15832 , 15731 , 15619 , 15589 , 15512 , 15416 , 15310 , 0] ,
  [15967 , 15319 , 14937 , 14753 , 14010 , 12638 , 11787 , 11360 , 10805 , 9706  , 8934  , 8515  , 8166  , 7456  , 6911  , 6575  , 0] ,
  [4906  , 3005  , 2985  , 2984  , 875   , 102   , 83    , 81    , 47    , 17    , 12    , 11    , 8     , 5     , 4     , 3     , 0] ,
  [7217  , 4346  , 4269  , 4264  , 1924  , 428   , 340   , 332   , 280   , 203   , 179   , 175   , 171   , 164   , 159   , 157   , 0] ,
  [16010 , 15415 , 15032 , 14805 , 14228 , 13043 , 12168 , 11634 , 11265 , 10419 , 9645  , 9110  , 8892  , 8378  , 7850  , 7437  , 0] ,
  [8573  , 5218  , 5046  , 5032  , 2787  , 771   , 555   , 533   , 443   , 286   , 218   , 205   , 197   , 181   , 168   , 162   , 0] ,
  [11474 , 8095  , 7822  , 7796  , 4632  , 1443  , 1046  , 1004  , 748   , 351   , 218   , 194   , 167   , 121   , 93    , 83    , 0] ,
  [16152 , 15764 , 15463 , 15264 , 14925 , 14189 , 13536 , 13070 , 12846 , 12314 , 11763 , 11277 , 11131 , 10777 , 10383 , 10011 , 0] ,
  [14187 , 11654 , 11043 , 10919 , 8498  , 4885  , 3778  , 3552  , 2947  , 1835  , 1283  , 1134  , 998   , 749   , 585   , 514   , 0] ,
  [14162 , 11527 , 10759 , 10557 , 8601  , 5417  , 4105  , 3753  , 3286  , 2353  , 1708  , 1473  , 1370  , 1148  , 959   , 840   , 0] ,
  [16205 , 15902 , 15669 , 15498 , 15213 , 14601 , 14068 , 13674 , 13463 , 12970 , 12471 , 12061 , 11916 , 11564 , 11183 , 10841 , 0] ,
  [15043 , 12972 , 12092 , 11792 , 10265 , 7446  , 5934  , 5379  , 4883  , 3825  , 3036  , 2647  , 2507  , 2185  , 1901  , 1699  , 0] ,
  [15320 , 13694 , 12782 , 12352 , 11191 , 8936  , 7433  , 6671  , 6255  , 5366  , 4622  , 4158  , 4020  , 3712  , 3420  , 3198  , 0] ,
  [16255 , 16020 , 15768 , 15600 , 15416 , 14963 , 14440 , 14006 , 13875 , 13534 , 13137 , 12697 , 12602 , 12364 , 12084 , 11781 , 0] ,
  [15627 , 14503 , 13906 , 13622 , 12557 , 10527 , 9269  , 8661  , 8117  , 6933  , 5994  , 5474  , 5222  , 4664  , 4166  , 3841  , 0] ,
  [16366 , 16365 , 14547 , 14160 , 14159 , 14158 , 11969 , 11473 , 8735  , 6147  , 4911  , 4530  , 3865  , 3180  , 2710  , 2473  , 0] ,
  [16257 , 16038 , 15871 , 15754 , 15536 , 15071 , 14673 , 14390 , 14230 , 13842 , 13452 , 13136 , 13021 , 12745 , 12434 , 12154 , 0] ,
  [15855 , 14971 , 14338 , 13939 , 13239 , 11782 , 10585 , 9805  , 9444  , 8623  , 7846  , 7254  , 7079  , 6673  , 6262  , 5923  , 0] ,
  [9492  , 6318  , 6197  , 6189  , 3004  , 652   , 489   , 477   , 333   , 143   , 96    , 90    , 78    , 60    , 50    , 47    , 0] ,
  [16313 , 16191 , 16063 , 15968 , 15851 , 15590 , 15303 , 15082 , 14968 , 14704 , 14427 , 14177 , 14095 , 13899 , 13674 , 13457 , 0] ,
  [8485  , 5473  , 5389  , 5383  , 2411  , 494   , 386   , 377   , 278   , 150   , 117   , 112   , 103   , 89    , 81    , 78    , 0] ,
  [10497 , 7154  , 6959  , 6943  , 3788  , 1004  , 734   , 709   , 517   , 238   , 152   , 138   , 120   , 90    , 72    , 66    , 0] ,
  [16317 , 16226 , 16127 , 16040 , 15955 , 15762 , 15547 , 15345 , 15277 , 15111 , 14922 , 14723 , 14671 , 14546 , 14396 , 14239 , 0] ,
  [16382 , 16381 , 15858 , 15540 , 15539 , 15538 , 14704 , 14168 , 13768 , 13092 , 12452 , 11925 , 11683 , 11268 , 10841 , 10460 , 0] ,
  [5974  , 3798  , 3758  , 3755  , 1275  , 205   , 166   , 162   , 95    , 35    , 26    , 24    , 18    , 11    , 8     , 7     , 0] ,
  [3532  , 2258  , 2246  , 2244  , 731   , 135   , 118   , 115   , 87    , 45    , 36    , 34    , 29    , 21    , 17    , 16    , 0] ,
  [7466  , 4882  , 4821  , 4811  , 2476  , 886   , 788   , 771   , 688   , 531   , 469   , 457   , 437   , 400   , 369   , 361   , 0] ,
  [9580  , 5772  , 5291  , 5216  , 3444  , 1496  , 1025  , 928   , 806   , 578   , 433   , 384   , 366   , 331   , 296   , 273   , 0] ,
  [10692 , 7730  , 7543  , 7521  , 4679  , 1746  , 1391  , 1346  , 1128  , 692   , 495   , 458   , 424   , 353   , 291   , 268   , 0] ,
  [11040 , 7132  , 6549  , 6452  , 4377  , 1875  , 1253  , 1130  , 958   , 631   , 431   , 370   , 346   , 296   , 253   , 227   , 0] ,
  [12687 , 9332  , 8701  , 8585  , 6266  , 3093  , 2182  , 2004  , 1683  , 1072  , 712   , 608   , 559   , 458   , 373   , 323   , 0] ,
  [13429 , 9853  , 8860  , 8584  , 6806  , 4039  , 2862  , 2478  , 2239  , 1764  , 1409  , 1224  , 1178  , 1077  , 979   , 903   , 0] ,
  [14685 , 12163 , 11061 , 10668 , 9101  , 6345  , 4871  , 4263  , 3908  , 3200  , 2668  , 2368  , 2285  , 2106  , 1942  , 1819  , 0] ,
  [13295 , 11302 , 10999 , 10945 , 7947  , 5036  , 4490  , 4385  , 3391  , 2185  , 1836  , 1757  , 1424  , 998   , 833   , 785   , 0] ,
  [4992  , 2993  , 2972  , 2970  , 1269  , 575   , 552   , 549   , 530   , 505   , 497   , 495   , 493   , 489   , 486   , 485   , 0] ,
  [15419 , 13862 , 13104 , 12819 , 11429 , 8753  , 7220  , 6651  , 6020  , 4667  , 3663  , 3220  , 2995  , 2511  , 2107  , 1871  , 0] ,
  [12468 , 9263  , 8912  , 8873  , 5758  , 2193  , 1625  , 1556  , 1187  , 589   , 371   , 330   , 283   , 200   , 149   , 131   , 0] ,
  [15870 , 15076 , 14615 , 14369 , 13586 , 12034 , 10990 , 10423 , 9953  , 8908  , 8031  , 7488  , 7233  , 6648  , 6101  , 5712  , 0] ,
  [1693  , 978   , 976   , 975   , 194   , 18    , 16    , 15    , 11    , 7     , 6     , 5     , 4     , 3     , 2     , 1     , 0] ,
  [7992  , 5218  , 5147  , 5143  , 2152  , 366   , 282   , 276   , 173   , 59    , 38    , 35    , 27    , 16    , 11    , 10    , 0]];
