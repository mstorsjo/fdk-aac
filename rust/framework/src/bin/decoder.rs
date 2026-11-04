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
use aac::aac_dec::*;
use std::{fs::File, io::Read, process::exit};

struct BsDescr {
    file: File,
    channels: u16,
    sample_rate: u32,
    frame_size: usize,
}

/// Return a hardcoded file handle to the bitstream file and some (also hardcoded) parameters that
/// match the bitstream.
///
/// Raw packets file format
/// first packet is usually the CONFIG */
/// int32      CONFIG_LENGTH;                      4             bytes
/// uchar      CONFIG[CONFIG_LENGTH];              CONFIG_LENGTH bytes
///
/// if (CONFIG_LENGTH==1) {
///   /* if first packet is only 1 byte then it tells us the transport type */
///   transport_type = CONFIG[0];
///   /* second packet is now config */
///   int32      CONFIG_LENGTH;                    4             bytes
///   uchar      CONFIG[CONFIG_LENGTH];            CONFIG_LENGTH bytes
/// }
///
/// /* any further packets are content */
/// while (!eof) {
///   int32     PACKET_LENGTH;                     4            bytes
///   uchar     PACKET[PACKET_LENGTH];             PACKET_LENGTH bytes
/// }
fn open_bs() -> BsDescr {
    let channels = 2;
    let sample_rate = 44100;
    let frame_size: usize = 1024;

    // Open the binary file.
    let bs_file = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("..")
        .join("bitstreams")
        .join("input.rawpkts");
    let bs_file = match bs_file.canonicalize() {
        Ok(path) => {
            let path_str = path.to_str().unwrap_or_default();
            if let Some(stripped) = path_str.strip_prefix(r"\\?\") {
                std::path::PathBuf::from(stripped)
            } else {
                path
            }
        }
        Err(e) => {
            eprintln!("Error resolving file path: {:?}", e);
            exit(1);
        }
    };
    let file = match File::open(&bs_file) {
        Ok(f) => f,
        Err(e) => {
            eprintln!("Error opening input file: {:?}", e);
            exit(1);
        }
    };

    BsDescr {
        file,
        channels,
        sample_rate,
        frame_size,
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut decoder = AacDecoderInstance::new(TransportType::Mp4Raw);

    // Input buffer, containing the Access units (AU)
    let mut au_frames = [0u8; TRANSPORTDEC_INBUF_SIZE];

    // Configure decoder.
    if let Err(e) = decoder.set_param(Param::PcmMaxOutputChannels(8)) {
        eprintln!(
            "Setting decoder parameter PcmMaxOutputChannels failed ({:?})",
            e
        )
    };
    if let Err(e) = decoder.set_param(Param::PcmMinOutputChannels(0)) {
        eprintln!(
            "Setting decoder parameter PcmMaxOutputChannels failed ({:?})",
            e
        )
    };
    if let Err(e) = decoder.set_param(Param::PcmLimiterEnable(LimiterMode::Auto)) {
        eprintln!(
            "Setting decoder parameter PcmMaxOutputChannels failed ({:?})",
            e
        )
    };

    let (mut file, channels, sample_rate, frame_size) = {
        let bs_descr = open_bs();
        (
            bs_descr.file,
            bs_descr.channels,
            bs_descr.sample_rate,
            bs_descr.frame_size,
        )
    };
    println!(
        "Decoding raw packets with {} channels, {} Hz sample rate, {} samples frame size",
        channels, sample_rate, frame_size
    );

    // Parse ASC from file.
    // Read ASC length.
    let mut len_buf = [0u8; 4];
    file.read_exact(&mut len_buf)?;
    let asc_len = i32::from_ne_bytes(len_buf);
    debug_assert!(
        asc_len != 1,
        "Config length of 1 is not implemented for raw packets reading."
    );

    // Ancillary data buffer
    let mut anc_buf = [0u8; 256];

    // Read ASC itself.
    let mut asc = vec![0u8; asc_len as usize];
    file.read_exact(&mut asc)?;

    if let Err(e) = decoder.config_raw(&asc) {
        eprintln!("Config Raw Error: {:?}", e);
        exit(1);
    }

    // Output buffer, supposed to contain one decoded PCM sample.
    let mut time_data: Vec<f32> = vec![0.0f32; frame_size * usize::from(channels)];

    // Open output wav file.
    let spec = hound::WavSpec {
        channels,
        sample_rate,
        bits_per_sample: std::mem::size_of::<f32>() as u16 * 8,
        sample_format: hound::SampleFormat::Float,
    };

    match hound::WavWriter::create("out.wav", spec) {
        Ok(mut wav_writer) => {
            let mut frame_count = 0;
            // Decoding loop
            let mut dec_info = DecoderInfo::default();
            while file.read(&mut len_buf)? > 0 {
                let au_len = i32::from_ne_bytes(len_buf) as usize;

                if file.read(&mut au_frames[0..au_len])? < au_len {
                    eprintln!("EOF file reached!");
                    break;
                }

                // Fill the decoder's internal buffer with one AU.
                match decoder.fill(&au_frames[0..au_len], au_len) {
                    Err(e) => {
                        eprintln!("Decoder fill error: {:?}", e);
                        exit(1);
                    }
                    Ok(bytes_left) => {
                        if bytes_left != 0 {
                            eprintln!("Not all bytes of the frame have been read!");
                            exit(1);
                        }
                    }
                }

                // Decode an AU.
                dec_info = match decoder.decode(&mut time_data) {
                    Ok(dinfo) => {
                        print!("[{frame_count}]\r");
                        if frame_count == 0 {
                            eprintln!("Decoder info: {:?}", dinfo);
                        }
                        dinfo
                    }
                    Err((e, dinfo)) => {
                        println!("Decoded frame {frame_count}");
                        eprintln!("Decode error: {:?}", e);
                        eprintln!("Decoder info: {:?}", dinfo);
                        exit(1);
                    }
                };

                // Decode ancillary data if available.
                match decoder.anc_data(0, &mut anc_buf) {
                    Ok(anc_data_len) => {
                        if anc_data_len > 0 {
                            println!();
                            for anc_data in &mut anc_buf[0..anc_data_len] {
                                print!("{}", *anc_data as char)
                            }
                            println!();
                        }
                    }
                    Err(e) => {
                        eprintln!("Ancillary data extraction error: {:?}", e);
                        exit(1);
                    }
                }

                // Write output WAV file.
                for sample in time_data.iter() {
                    if let Err(e) = wav_writer.write_sample(*sample) {
                        eprintln!("Wav writer write error: {:?}", e);
                        exit(1);
                    };
                }

                frame_count += 1;
            }

            let mut samples_to_flush: u16 =
                dec_info.output_info.output_delay.try_into().unwrap_or(0);

            while samples_to_flush > 0 {
                // Flush the decoder.
                let output_info = match decoder.drain(&mut time_data) {
                    Ok(out_info) => out_info,
                    Err((e, out_info)) => {
                        eprintln!("Flush error: {:?}", e);
                        eprintln!("Decoder info: {:?}", out_info);
                        exit(1);
                    }
                };

                // Write output WAV file.
                samples_to_flush -= output_info.frame_size.min(samples_to_flush);
                for sample in time_data[0..(output_info.frame_size * channels).into()].iter() {
                    if let Err(e) = wav_writer.write_sample(*sample) {
                        eprintln!("Wav writer write error: {:?}", e);
                        exit(1);
                    };
                }
            }

            if let Err(e) = wav_writer.finalize() {
                eprintln!("Wav writer finalize error: {:?}", e);
                exit(1);
            };
        }
        Err(e) => {
            eprintln!("Wav writer open error: {:?}", e);
            exit(1);
        }
    }

    Ok(())
}
