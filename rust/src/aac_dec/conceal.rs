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
//! AAC core concealment
//!
//! AAC core implementation includes a concealment functions.
//! There are various tests inside the core, starting with simple CRC tests and
//! ending in a variety of plausibility checks. If such a check indicates an
//! invalid bitstream, then concealment is applied.
//!
//! Concealment is also applied when the calling main program indicates a
//! distorted or missing data frame using the is_frame_ok flag. This is used for error
//! detection on the transport layer. (See below)
//!
//! There are three concealment-modes:
//!
//! 1) Muting: The spectral data is simply set to zero in case of an detected error.
//!
//! 2) Noise substitution: In case of an detected error, concealment copies the last frame and adds
//!    attenuates the spectral data. Noise substitution adds no additional delay.
//!
//! 3) Interpolation: The interpolation routine swaps the spectral data from the previous and the
//!    current frame just before the final frequency to time conversion. In case a single frame is
//!    corrupted, concealmant interpolates between the last good and the first good frame to create
//!    the spectral data for the missing frame. If multiple frames are corrupted, concealment
//!    implements first a fade out based on slightly modified spectral values from the last good
//!    frame. As soon as good frames are available, concealmant fades in the new spectral data. This
//!    mode basically adds approriate delay in the SBR decoder. Note that the
//!    'Interpolating-Concealment' increases the delay of your decoder by one frame and that it does
//!    require additional resources such as memory and computational complexity.
//!
//!    How concealment can be used with errors on the transport layer.
//!
//!    Many errors can or have to be detected on the transport layer. For example in
//!    IP based systems packet loss can occur. The transport protocol used should
//!    indicate such packet loss by inserting an empty frame with is_frame_ok=false.

mod conceal_constants;
mod conceal_data;
mod conceal_info;
mod conceal_params;

// re-exports
pub(crate) use conceal_constants::A_CONCEAL_AU;
pub use conceal_constants::{AacDecoderRenderMode, ConcealmentState, MAX_NUM_FADE_FACTORS};
pub use conceal_data::ConcealmentData;
pub use conceal_params::{ConcealmentMethod, ConcealmentParams};
