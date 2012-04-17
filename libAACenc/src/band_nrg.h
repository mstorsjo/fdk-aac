/******************************** MPEG Audio Encoder **************************

                       (C) Copyright Fraunhofer IIS (1999)
                               All Rights Reserved

    Please be advised that this software and/or program delivery is
    Confidential Information of Fraunhofer and subject to and covered by the

    Fraunhofer IIS Software Evaluation Agreement
    between Google Inc. and  Fraunhofer
    effective and in full force since March 1, 2012.

    You may use this software and/or program only under the terms and
    conditions described in the above mentioned Fraunhofer IIS Software
    Evaluation Agreement. Any other and/or further use requires a separate agreement.


   $Id$
   Author(s):   M. Werner
   Description: Band/Line energy calculation

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/
#ifndef _BAND_NRG_H
#define _BAND_NRG_H

#include "common_fix.h"


void
FDKaacEnc_CalcSfbMaxScaleSpec(
        const FIXP_DBL     *mdctSpectrum,
        const INT          *bandOffset,
        INT                *sfbMaxScaleSpec,
        const INT           numBands
        );

FIXP_DBL
FDKaacEnc_CheckBandEnergyOptim(
        const FIXP_DBL     *mdctSpectrum,
        INT                *sfbMaxScaleSpec,
        const INT          *bandOffset,
        const INT           numBands,
        FIXP_DBL           *bandEnergy,
        FIXP_DBL           *bandEnergyLdData,
        INT                 minSpecShift
        );

INT
FDKaacEnc_CalcBandEnergyOptimLong(
        const FIXP_DBL     *mdctSpectrum,
        INT                *sfbMaxScaleSpec,
        const INT          *bandOffset,
        const INT           numBands,
        FIXP_DBL           *bandEnergy,
        FIXP_DBL           *bandEnergyLdData
        );

void
FDKaacEnc_CalcBandEnergyOptimShort(
        const FIXP_DBL     *mdctSpectrum,
        INT                *sfbMaxScaleSpec,
        const INT          *bandOffset,
        const INT           numBands,
        FIXP_DBL           *bandEnergy
        );


void FDKaacEnc_CalcBandNrgMSOpt(
        const FIXP_DBL   *RESTRICT mdctSpectrumLeft,
        const FIXP_DBL   *RESTRICT mdctSpectrumRight,
        INT              *RESTRICT sfbMaxScaleSpecLeft,
        INT              *RESTRICT sfbMaxScaleSpecRight,
        const INT        *RESTRICT bandOffset,
        const INT         numBands,
        FIXP_DBL         *RESTRICT bandEnergyMid,
        FIXP_DBL         *RESTRICT bandEnergySide,
        INT               calcLdData,
        FIXP_DBL         *RESTRICT bandEnergyMidLdData,
        FIXP_DBL         *RESTRICT bandEnergySideLdData);

#endif
