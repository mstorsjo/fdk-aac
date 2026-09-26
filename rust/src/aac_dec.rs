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
//! Advanced audio coding (AAC) decoder

// Modules
pub mod aacdecoder;
pub mod ancillary_data;
pub mod block;
pub mod callbacks;
pub mod channel;
pub mod channel_info;
pub mod conceal;
pub mod config;
pub mod constants;
pub mod drc;
pub mod error_codes;
pub mod ext_data;
pub mod hcr;
pub mod huff_dec;
pub mod intensity;
pub mod interleaver;
pub mod inverse_quantization;
pub mod ipf;
pub mod lpd;
pub mod ms_stereo;
pub mod noise_filling;
pub mod output_info;
pub mod params;
pub mod pns;
pub mod process;
pub mod pulse_data;
pub mod rvlc;
pub mod signal_delay;
pub mod sr_info;
pub mod tns;
pub mod utils;

// Re-exports
pub use crate::{
    aac_dec::{
        conceal::ConcealmentMethod,
        constants::MAX_CHANNELS,
        drc::{AacDrcParameterHandling, AacDrcPresentationMode},
        error_codes::AacDecoderError,
        ext_data::MAX_ANC_ELEMENTS,
        output_info::{BitstreamInfo, DecoderInfo, MetadataInfo, OutputInfo, StreamInfo},
        params::{AacMdProfile, LimiterMode, Param},
    },
    common::{
        aot::AudioObjectType, audio_channel_type::AudioChannelType,
        bs_element_id::ChannelElementId, channel_order::ChannelOrder, flags::ACFlags,
        transport_type::TransportType,
    },
    drc_dec::DrcEffectTypeRequest,
    pcm_dmx::DualChannelMode,
    tp_dec::{MAX_CONF_SIZE, TRANSPORTDEC_INBUF_SIZE},
};

// Imports
use crate::{
    aac_dec::{aacdecoder::AacDecoder, conceal::A_CONCEAL_AU, ipf::IpfData, params::Params},
    common::{bitstream::Bitstream, flags::AACDecFlags},
    tp_dec::{callbacks::TpDecCb, TpDecParam, TransportDec},
};
use std::{cell::RefCell, ops::DerefMut, rc::Rc};

/// AAC decoder instance.
#[repr(C)]
#[derive(Debug)]
pub struct AacDecoderInstance {
    /// Parameters for the AAC decoder.
    params: Params,

    /// AAC decoder handle.
    aac_decoder: Rc<RefCell<AacDecoder>>,

    /// Immediate Playout Frame (IPF) data.
    ipf_data: Option<Box<IpfData>>,

    /// Transport decoder handle.
    transport_decoder: Box<TransportDec>,
}

impl AacDecoderInstance {
    /// Creates a new `AacDecoderInstance` instance.
    pub fn new(transport_type: TransportType) -> AacDecoderInstance {
        let mut aac_decoder_instance = AacDecoderInstance {
            params: Params::new(),
            aac_decoder: Rc::new(RefCell::new(AacDecoder::new())),
            transport_decoder: Box::new(TransportDec::new(transport_type)),
            ipf_data: None,
        };

        aac_decoder_instance.transport_decoder.init(transport_type);
        aac_decoder_instance.params.init();
        aac_decoder_instance.register_callbacks();

        aac_decoder_instance
    }

    /// Registers callback data.
    fn register_callbacks(&mut self) {
        let aac_dec_clone = self.aac_decoder.clone() as Rc<RefCell<dyn TpDecCb>>;
        let cb = self.transport_decoder.callback();
        cb.register_callback_data(Some(aac_dec_clone));
    }

    /// Reads one ancillary data element into the provided buffer.
    ///
    /// # Parameters
    ///
    /// - `index`: Index value of ancillary data.
    /// - `buffer`: Buffer receiving the requested ancillary data.
    ///
    /// # Return
    ///
    /// - `Result<usize, AacDecoderError>`
    ///    - `Ok(usize)` if success, number of bytes written to buffer.
    ///    - `AacDecoderError` in case of error.
    pub fn anc_data(&mut self, index: usize, buffer: &mut [u8]) -> Result<usize, AacDecoderError> {
        let mut refcell_aac_dec = self.aac_decoder.borrow_mut();
        let aac_dec = refcell_aac_dec.deref_mut();

        let ext_data = &mut aac_dec.extension_data;
        if ext_data.ancillary_data.is_empty() {
            return Ok(0);
        }

        if index > (ext_data.ancillary_data.len()).min(ext_data.ancillary_data.capacity()) {
            return Err(AacDecoderError::AncDataError);
        }

        if let Some(anc_data) = ext_data.ancillary_data_mut(index) {
            let bs = self.transport_decoder.bs_mut();
            anc_data.read(bs, buffer)
        } else {
            Ok(0)
        }
    }

    /// Decodes an `AAC` frame. The decoded output signal is stored in `time_data`.
    ///
    /// # Parameters
    ///
    /// - `time_data`: External output buffer, where the decoded PCM samples will be stored into.
    ///
    /// # Return
    ///
    /// - `Result<DecoderInfo, (AacDecoderError, DecoderInfo)>`
    ///   - `Ok(dec_info)`: If decoding is successful, it returns important information about the
    ///     bitstream (i.e. `StreamInfo`), any potential metadata present in the bitstream (i.e.
    ///     `MetadataInfo`) and output information, which describes the output signal (i.e.
    ///     `OutputInfo`).
    ///   - `Err((error, dec_info))`: If decoding fails, it returns an error and the output
    ///     information, which describes the output signal (i.e. `OutputInfo`).
    #[allow(clippy::result_large_err)]
    pub fn decode(
        &mut self,
        time_data: &mut [f32],
    ) -> Result<DecoderInfo, (AacDecoderError, DecoderInfo)> {
        let mut error_status = AacDecoderError::Ok;
        let mut output_info = OutputInfo::default();

        let bs = self.transport_decoder.bs_mut();
        let bs_anchor = bs.valid_bits();

        if Self::is_concealment_au(bs) {
            // Conceal frame if concealment byte sequence matches.
            match self.conceal(time_data) {
                Ok(out_info) => output_info = out_info,
                Err((e, out_info)) => {
                    output_info = out_info;
                    error_status = e;
                }
            }
        } else {
            //  Read transport header.
            match self.transport_decoder.read_access_unit() {
                Ok(_) => {
                    error_status = AacDecoderError::Ok;
                }
                Err(e) => {
                    error_status = AacDecoderError::from(e);
                }
            }

            let is_ipf_possible = {
                let mut refcell_aac_dec = self.aac_decoder.borrow_mut();
                let aac_dec = refcell_aac_dec.deref_mut();
                aac_dec.is_ipf_possible()
            };

            // Allocate ipf module if immediate playout frame supported in decoder configuration.
            if is_ipf_possible && self.ipf_data.is_none() && error_status == AacDecoderError::Ok {
                self.ipf_data = Some(Box::new(IpfData::new()));
            }

            if error_status == AacDecoderError::TransportSyncError {
                let mut refcell_aac_dec = self.aac_decoder.borrow_mut();
                let aac_dec = refcell_aac_dec.deref_mut();

                // Signal bitstream discontinuity.
                aac_dec.signal_interruption(false);
            } else if error_status == AacDecoderError::Ok {
                let mut ipf_data = if is_ipf_possible {
                    self.ipf_data.as_deref_mut()
                } else {
                    None
                };

                // Decode bistream
                match AacDecoder::process(
                    self.aac_decoder.clone(),
                    &mut ipf_data,
                    &mut Some(&mut self.transport_decoder),
                    time_data,
                    &mut self.params,
                    AACDecFlags::empty(),
                ) {
                    Ok(out_info) => {
                        output_info = out_info;
                    }
                    Err((e, out_info)) => {
                        output_info = out_info;
                        error_status = e;
                    }
                }
            }

            // Finalize transport frame in case this has not yet been done.
            if (error_status != AacDecoderError::NotEnoughBits)
                && error_status != AacDecoderError::TransportSyncError
            {
                if let Err(err) = self.transport_decoder.end_access_unit() {
                    error_status = err.into();
                }
            }
        }

        let mut refcell_aac_dec = self.aac_decoder.borrow_mut();
        let aac_dec = refcell_aac_dec.deref_mut();

        // Create `BitstreamInfo`, with valid internal data.
        let bs_info = BitstreamInfo {
            stream_info: aac_dec.stream_info(
                &mut self.transport_decoder,
                bs_anchor,
                error_status == AacDecoderError::Ok,
            ),
            metadata_info: aac_dec.metadata_info(),
        };

        if error_status == AacDecoderError::Ok {
            Ok(DecoderInfo {
                output_info,
                bs_info,
            })
        } else {
            Err((
                error_status,
                DecoderInfo {
                    output_info,
                    bs_info,
                },
            ))
        }
    }

    /// Fills AAC decoder's internal input buffer with bitstream data from the
    /// external input buffer. The function only copies such data as long as the
    /// decoder-internal input buffer is not full. So it grabs whatever it can from
    /// buffer and returns information (`bytes_valid`) so that at a subsequent call of
    /// fill(), the right position in buffer can be determined to grab next data.
    ///
    /// # Parameters
    ///
    /// - `buffer`: External input buffer.
    /// - `bytes_valid`: Number of bitstream bytes in the external bitstream buffer that have not
    ///   yet been copied into the decoder's internal bitstream buffer by calling this function.
    ///
    /// # Return
    ///
    /// - `Result<usize, AacDecoderError>`.
    ///   - `Ok(usize)`: Number of remaining valid bytes in the external bitstream buffer.
    ///   - `AacDecoderError`: AAC decoder error.
    pub fn fill(&mut self, buffer: &[u8], bytes_valid: usize) -> Result<usize, AacDecoderError> {
        match self.transport_decoder.fill_data(buffer, bytes_valid) {
            Ok(valid_bytes) => Ok(valid_bytes),
            Err((_remain_bytes, e)) => Err(e.into()),
        }
    }

    /// Signals an input bit stream data discontinuity.
    /// Resyncs any internals as necessary. Clears all signal delay lines and history
    /// buffers. This can cause discontinuities in the output signal.
    ///
    /// # Return
    ///
    /// - `Result<(), AacDecoderError>`.
    pub fn interrupt(&mut self) -> Result<(), AacDecoderError> {
        let mut refcell_aac_dec = self.aac_decoder.borrow_mut();
        let aac_decoder = refcell_aac_dec.deref_mut();
        aac_decoder.signal_interruption(true);
        self.params.restore();
        Ok(())
    }

    /// Clears internal bit stream buffer of transport layers.
    /// The decoder starts decoding at new data passed after this event
    /// and any previous bit stream data is discarded.
    ///
    /// # Return
    ///
    /// - `Result<(), AacDecoderError>`.
    pub fn clear(&mut self) -> Result<(), AacDecoderError> {
        if self
            .transport_decoder
            .set_param(TpDecParam::Reset, true)
            .is_err()
        {
            Err(AacDecoderError::SetParamFail)
        } else {
            Ok(())
        }
    }

    /// Returns true if the AU concealment byte sequence is found in the given bitstream. See
    /// `aac::aac_dec::conceal::conceal_constants::A_CONCEAL_AU`.
    fn is_concealment_au(bs: &mut Bitstream) -> bool {
        if bs.valid_bits() >= (8 * A_CONCEAL_AU.len()) as isize {
            let mut is_conceal_au = true;
            for (i, &byte) in A_CONCEAL_AU.iter().enumerate() {
                if bs.read(8) != u32::from(byte) {
                    is_conceal_au = false;
                    bs.push(-8 * (i + 1) as isize);
                    break;
                }
            }
            is_conceal_au
        } else {
            false
        }
    }

    /// Explicitly configures the decoder by passing a raw AudioSpecificConfig
    /// (ASC) or a StreamMuxConfig (SMC), contained in a binary buffer. This is
    /// required for MPEG-4 and Raw Packets file format bitstreams as well as for
    /// LATM bitstreams with no in-band SMC. If the transport format is LATM with or
    /// without LOAS, configuration is assumed to be an SMC, for all other file
    /// formats an ASC.
    ///
    /// # Parameters
    ///
    /// - `conf`: Buffer containing the binary configuration (either ASC or SMC).
    ///
    /// # Return
    ///
    /// - `Result<(), AacDecoderError>`.
    pub fn config_raw(&mut self, conf: &[u8]) -> Result<(), AacDecoderError> {
        self.transport_decoder
            .out_of_band_config(conf)
            .map_err(|e| e.into())
    }

    /// Sets one single decoder parameter.
    ///
    /// # Parameters
    ///
    /// - `param`: Parameter type to be set.
    ///
    /// # Return
    ///
    /// - `Result<(), AacDecoderError>`.
    pub fn set_param(&mut self, param: Param) -> Result<(), AacDecoderError> {
        let mut err = Ok(());
        match param {
            Param::TpdecParamIgnoreBufferFullness(value) => {
                if self
                    .transport_decoder
                    .set_param(TpDecParam::IgnoreBufferFullness, value)
                    .is_err()
                {
                    err = Err(AacDecoderError::SetParamFail);
                }
            }
            Param::TpdecCheckTwoSyncs(value) => {
                if self
                    .transport_decoder
                    .set_param(TpDecParam::CheckTwoSyncs, value)
                    .is_err()
                {
                    err = Err(AacDecoderError::SetParamFail);
                }
            }
            _ => {
                err = self.params.set_param(param);
            }
        }
        err
    }

    /// Flushes all filterbanks to get all delayed audio without having new input
    /// data. New input data will not be considered.
    ///
    /// # Parameters
    ///
    /// - `time_data`: Time domain input/ouptut data buffer.
    ///
    /// # Return
    ///
    /// - `Result<OutputInfo, (AacDecoderError, OutputInfo)>`.
    pub fn drain(
        &mut self,
        time_data: &mut [f32],
    ) -> Result<OutputInfo, (AacDecoderError, OutputInfo)> {
        if let Err(e) = self.clear() {
            return Err((e, OutputInfo::default()));
        }

        AacDecoder::process(
            self.aac_decoder.clone(),
            &mut None,
            &mut None,
            time_data,
            &mut self.params,
            AACDecFlags::FLUSH,
        )
    }

    /// Triggers the built-in error concealment to generate substitute signal for
    /// one lost frame. New input data will not be considered.
    ///
    /// # Parameters
    ///
    /// - `time_data`: Time domain input/ouptut data buffer.
    ///
    /// # Return
    ///
    /// - `Result<OutputInfo, (AacDecoderError, OutputInfo)>`.
    pub fn conceal(
        &mut self,
        time_data: &mut [f32],
    ) -> Result<OutputInfo, (AacDecoderError, OutputInfo)> {
        AacDecoder::process(
            self.aac_decoder.clone(),
            &mut None,
            &mut None,
            time_data,
            &mut self.params,
            AACDecFlags::CONCEAL,
        )
    }
}

impl Drop for AacDecoderInstance {
    /// De-initializes the internal heap memory.
    fn drop(&mut self) {
        self.transport_decoder.data.deinit();
        self.ipf_data = None;

        {
            let mut refcell_aac_dec = self.aac_decoder.borrow_mut();
            let aac_dec = refcell_aac_dec.deref_mut();
            aac_dec.close();
        }
    }
}
