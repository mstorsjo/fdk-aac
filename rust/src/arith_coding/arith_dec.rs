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
//! Arithmetic decoder
//!
//! Context adaptive arithmetic decoder. Reduces redundancy of quantized spectrum.

use std::cmp::Ordering;

use crate::common::bitstream::Bitstream;

use super::arith_tables::{ARI_LSB2, ARI_PK};

const AC_MAX_FRAMESIZE: usize = 1024;
const CONTEXT_BITSNEW: u8 = 16;
const STATE_BITSNEW: u8 = 14;
const VAL_ESC: u8 = 16;

// Hash table mapping context states to a cumulative frequencies table index pki
#[rustfmt::skip]
static ARI_MERGED_HASH_PS: [u32;742] = [
  0x00001044 , 0x00003D0A , 0x00005350 , 0x000074D6 , 0x0000A49F , 0x0000F96E , 0x00111000 , 0x01111E83 , 0x01113146 , 0x01114036 ,
  0x01116863 , 0x011194E9 , 0x0111F7EE , 0x0112269B , 0x01124775 , 0x01126DA1 , 0x0112D912 , 0x01131AF0 , 0x011336DD , 0x01135CF5 ,
  0x01139DF8 , 0x01141A5B , 0x01144773 , 0x01146CF5 , 0x0114FDE9 , 0x01166CF5 , 0x0116FDE4 , 0x01174CF3 , 0x011FFDCF , 0x01211CC2 ,
  0x01213B2D , 0x01214036 , 0x01216863 , 0x012194D2 , 0x0122197F , 0x01223AAD , 0x01224036 , 0x01226878 , 0x0122A929 , 0x0122F4AB ,
  0x01232B2D , 0x012347B6 , 0x01237DF8 , 0x0123B929 , 0x012417DD , 0x01245D76 , 0x01249DF8 , 0x0124F912 , 0x01255D75 , 0x0125FDE9 ,
  0x01265D75 , 0x012B8DF7 , 0x01311E2A , 0x01313B5E , 0x0131687B , 0x01321A6D , 0x013237BC , 0x01326863 , 0x0132F4EE , 0x01332B5E ,
  0x01335DA1 , 0x01338E24 , 0x01341A5E , 0x01343DB6 , 0x01348DF8 , 0x01351935 , 0x01355DB7 , 0x0135FE12 , 0x01376DF7 , 0x013FFE29 ,
  0x01400024 , 0x01423821 , 0x014318F6 , 0x01433821 , 0x0143F8E5 , 0x01443DA1 , 0x01486E38 , 0x014FF929 , 0x01543EE3 , 0x015FF912 ,
  0x016F298C , 0x018A5A69 , 0x021007F1 , 0x02112C2C , 0x02114B48 , 0x02117353 , 0x0211F4AE , 0x02122FEA , 0x02124B48 , 0x02127850 ,
  0x0212F72E , 0x02133A9E , 0x02134036 , 0x02138864 , 0x021414AD , 0x0214379E , 0x02145DB6 , 0x0214FE1F , 0x02166DB7 , 0x02200DC4 ,
  0x02212FEA , 0x022147A0 , 0x02218369 , 0x0221F7EE , 0x02222AAD , 0x02224788 , 0x02226863 , 0x02229929 , 0x0222F4AB , 0x02232A9E ,
  0x02234F08 , 0x02237864 , 0x0223A929 , 0x022417DE , 0x02244F36 , 0x02248863 , 0x02251A74 , 0x02256DA1 , 0x0225FE12 , 0x02263DB6 ,
  0x02276DF7 , 0x022FFE29 , 0x0231186D , 0x023137BC , 0x02314020 , 0x02319ED6 , 0x0232196D , 0x023237BC , 0x02325809 , 0x02329429 ,
  0x023317ED , 0x02333F08 , 0x02335809 , 0x023378E4 , 0x02341A7C , 0x02344221 , 0x0234A8D3 , 0x023514BC , 0x02354221 , 0x0235F8DF ,
  0x02364861 , 0x023FFE29 , 0x02400024 , 0x0241583B , 0x024214C8 , 0x02424809 , 0x02427EE6 , 0x02431708 , 0x02434809 , 0x02436ED0 ,
  0x02441A76 , 0x02443821 , 0x024458E3 , 0x0244F91F , 0x02454863 , 0x0246190A , 0x02464863 , 0x024FF929 , 0x02525ED0 , 0x025314E1 ,
  0x025348FB , 0x025419A1 , 0x025458D0 , 0x0254F4E5 , 0x02552861 , 0x025FF912 , 0x02665993 , 0x027F5A69 , 0x029F1481 , 0x02CF28A4 ,
  0x03100AF0 , 0x031120AA , 0x031147A0 , 0x03118356 , 0x031217EC , 0x03123B5E , 0x03124008 , 0x03127350 , 0x031314AA , 0x0313201E ,
  0x03134F08 , 0x03136863 , 0x03141A5E , 0x03143F3C , 0x03200847 , 0x03212AAD , 0x03214F20 , 0x03218ED6 , 0x032218AD , 0x032237BC ,
  0x03225809 , 0x03229416 , 0x032317ED , 0x03234F20 , 0x03237350 , 0x0323FA6B , 0x03243F08 , 0x03246863 , 0x0324F925 , 0x03254221 ,
  0x0325F8DF , 0x03264821 , 0x032FFE29 , 0x03311E47 , 0x03313F08 , 0x0331580D , 0x033214DE , 0x03323F08 , 0x03324020 , 0x03326350 ,
  0x033294E9 , 0x033317DE , 0x03333F08 , 0x0333627B , 0x0333A9A9 , 0x033417FC , 0x03343220 , 0x0334627B , 0x0334A9A9 , 0x0335148A ,
  0x03353220 , 0x033588E4 , 0x03361A4A , 0x03363821 , 0x0336F8D2 , 0x03376863 , 0x03411939 , 0x0341583B , 0x034214C8 , 0x03424809 ,
  0x03426ED0 , 0x03431588 , 0x03434809 , 0x03436ED0 , 0x03441A48 , 0x0344480D , 0x03446ED0 , 0x03451A4A , 0x03453809 , 0x03455EFB ,
  0x034614CA , 0x03463849 , 0x034F8924 , 0x03500A69 , 0x035252D0 , 0x035314E0 , 0x0353324D , 0x03535ED0 , 0x035414E0 , 0x0354324D ,
  0x03545ED0 , 0x0354F4E8 , 0x0355384D , 0x03555ED0 , 0x0355F4DF , 0x03564350 , 0x035969A6 , 0x035FFA52 , 0x036649A6 , 0x036FFA52 ,
  0x037F4F66 , 0x039D7492 , 0x03BF6892 , 0x03DF8A1F , 0x04100B84 , 0x04112107 , 0x0411520D , 0x041214EA , 0x04124F20 , 0x04131EDE ,
  0x04133F08 , 0x04135809 , 0x0413F42B , 0x04142F08 , 0x04200847 , 0x042121FC , 0x04214209 , 0x04221407 , 0x0422203C , 0x04224209 ,
  0x04226350 , 0x04231A7C , 0x04234209 , 0x0423637B , 0x04241A7C , 0x04243220 , 0x0424627B , 0x042514C8 , 0x04254809 , 0x042FF8E9 ,
  0x04311E47 , 0x04313220 , 0x0431527B , 0x043214FC , 0x04323220 , 0x04326250 , 0x043315BC , 0x04333220 , 0x0433527B , 0x04338413 ,
  0x04341488 , 0x04344809 , 0x04346ED0 , 0x04351F48 , 0x0435527B , 0x0435F9A5 , 0x04363809 , 0x04375EFB , 0x043FF929 , 0x04412E79 ,
  0x0441427B , 0x044219B9 , 0x04423809 , 0x0442537B , 0x044314C8 , 0x04432020 , 0x0443527B , 0x044414CA , 0x04443809 , 0x0444537B ,
  0x04448993 , 0x0445148A , 0x04453809 , 0x04455ED0 , 0x0445F4E5 , 0x0446384D , 0x045009A6 , 0x045272D3 , 0x045314A0 , 0x0453324D ,
  0x04535ED0 , 0x045415A0 , 0x0454324D , 0x04545ED0 , 0x04551F60 , 0x0455324D , 0x04562989 , 0x04564350 , 0x045FF4D2 , 0x04665993 ,
  0x047FFF62 , 0x048FF725 , 0x049F44BD , 0x04BFB7E5 , 0x04EF8A25 , 0x04FFFB98 , 0x051131F9 , 0x051212C7 , 0x05134209 , 0x05200247 ,
  0x05211007 , 0x05213E60 , 0x052212C7 , 0x05224209 , 0x052319BC , 0x05233220 , 0x0523527B , 0x052414C8 , 0x05243820 , 0x053112F9 ,
  0x05313E49 , 0x05321439 , 0x05323E49 , 0x0532537B , 0x053314C8 , 0x0533480D , 0x05337413 , 0x05341488 , 0x0534527B , 0x0534F4EB ,
  0x05353809 , 0x05356ED0 , 0x0535F4E5 , 0x0536427B , 0x054119B9 , 0x054212F9 , 0x05423249 , 0x05426ED3 , 0x05431739 , 0x05433249 ,
  0x05435ED0 , 0x0543F4EB , 0x05443809 , 0x05445ED0 , 0x0544F4E8 , 0x0545324D , 0x054FF992 , 0x055362D3 , 0x0553F5AB , 0x05544350 ,
  0x055514CA , 0x0555427B , 0x0555F4E5 , 0x0556327B , 0x055FF4D2 , 0x05665993 , 0x05774F53 , 0x059FF728 , 0x05CC37FD , 0x05EFBA28 ,
  0x05FFFB98 , 0x061131F9 , 0x06121407 , 0x06133E60 , 0x061A72E4 , 0x06211E47 , 0x06214E4B , 0x062214C7 , 0x06223E60 , 0x062312F9 ,
  0x06233E60 , 0x063112F9 , 0x06313E4C , 0x063219B9 , 0x06323E49 , 0x06331439 , 0x06333809 , 0x06336EE6 , 0x0633F5AB , 0x06343809 ,
  0x0634F42B , 0x0635427B , 0x063FF992 , 0x064342FB , 0x0643F4EB , 0x0644427B , 0x064524C9 , 0x06655993 , 0x0666170A , 0x066652E6 ,
  0x067A6F56 , 0x0698473D , 0x06CF67D2 , 0x06EF3A26 , 0x06FFFAD8 , 0x071131CC , 0x07211307 , 0x07222E79 , 0x072292DC , 0x07234E4B ,
  0x073112F9 , 0x07322339 , 0x073632CB , 0x073FF992 , 0x074432CB , 0x075549A6 , 0x0776FF68 , 0x07774350 , 0x0788473D , 0x07CF4516 ,
  0x07EF3A26 , 0x07FFFAD8 , 0x08222E79 , 0x083112F9 , 0x0834330B , 0x0845338B , 0x08756F5C , 0x0887F725 , 0x08884366 , 0x08AF649C ,
  0x08F00898 , 0x08FFFAD8 , 0x091111C7 , 0x0932330B , 0x0945338B , 0x09774F7D , 0x0998C725 , 0x09996416 , 0x09EF87E5 , 0x09FFFAD8 ,
  0x0A34330B , 0x0A45338B , 0x0A77467D , 0x0AA9F52B , 0x0AAA6416 , 0x0ABD67DF , 0x0AFFFA18 , 0x0B33330B , 0x0B4443A6 , 0x0B76467D ,
  0x0BB9751F , 0x0BBB59BD , 0x0BEF5892 , 0x0BFFFAD8 , 0x0C221339 , 0x0C53338E , 0x0C76367D , 0x0CCAF52E , 0x0CCC6996 , 0x0CFFFA18 ,
  0x0D44438E , 0x0D64264E , 0x0DDCF52E , 0x0DDD5996 , 0x0DFFFA18 , 0x0E43338E , 0x0E68465C , 0x0EEE651C , 0x0EFFFA18 , 0x0F33238E ,
  0x0F553659 , 0x0F8F451C , 0x0FAFF8AE , 0x0FF00A2E , 0x0FFF1ACC , 0x0FFF33BD , 0x0FFF7522 , 0x0FFFFAD8 , 0x10002C72 , 0x1111103E ,
  0x11121E83 , 0x11131E9A , 0x1121115A , 0x11221170 , 0x112316F0 , 0x1124175D , 0x11311CC2 , 0x11321182 , 0x11331D42 , 0x11411D48 ,
  0x11421836 , 0x11431876 , 0x11441DF5 , 0x1152287B , 0x12111903 , 0x1212115A , 0x121316F0 , 0x12211B30 , 0x12221B30 , 0x12231B02 ,
  0x12311184 , 0x12321D04 , 0x12331784 , 0x12411D39 , 0x12412020 , 0x12422220 , 0x12511D89 , 0x1252227B , 0x1258184A , 0x12832992 ,
  0x1311171A , 0x13121B30 , 0x1312202C , 0x131320AA , 0x132120AA , 0x132220AD , 0x13232FED , 0x13312107 , 0x13322134 , 0x13332134 ,
  0x13411D39 , 0x13431E74 , 0x13441834 , 0x134812B4 , 0x1352230B , 0x13611E4B , 0x136522E4 , 0x141113C2 , 0x141211C4 , 0x143121F9 ,
  0x143221F9 , 0x143321CA , 0x14351D34 , 0x14431E47 , 0x14441E74 , 0x144612B4 , 0x1452230E , 0x14551E74 , 0x1471130E , 0x151113C2 ,
  0x152121F9 , 0x153121F9 , 0x153221F9 , 0x15331007 , 0x15522E4E , 0x15551E74 , 0x1571130E , 0x161113C7 , 0x162121F9 , 0x163121F9 ,
  0x16611E79 , 0x16661334 , 0x171113C7 , 0x172121F9 , 0x17451E47 , 0x1771130C , 0x181113C7 , 0x18211E47 , 0x18511E4C , 0x1882130C ,
  0x191113C7 , 0x19331E79 , 0x1A111307 , 0x1A311E79 , 0x1F52230E , 0x200003C1 , 0x20001027 , 0x20004467 , 0x200079E7 , 0x2000E5EF ,
  0x21100BC0 , 0x211129C0 , 0x21114011 , 0x211189E7 , 0x2111F5EF , 0x21124011 , 0x21127455 , 0x211325C0 , 0x21134011 , 0x21137455 ,
  0x211425C0 , 0x21212440 , 0x21213001 , 0x2121F9EF , 0x21222540 , 0x21226455 , 0x2122F5EF , 0x21233051 , 0x2123F56F , 0x21244451 ,
  0x21312551 , 0x21323451 , 0x21332551 , 0x21844555 , 0x221125C0 , 0x22113011 , 0x2211F9EF , 0x22123051 , 0x2212F9EF , 0x221329D1 ,
  0x22212541 , 0x22213011 , 0x2221F9EF , 0x22223451 , 0x2222F9EF , 0x22232551 , 0x2223F56F , 0x22312551 , 0x223229D1 , 0x2232F56F ,
  0x22332551 , 0x2233F56F , 0x22875555 , 0x22DAB5D7 , 0x23112BD1 , 0x23115467 , 0x231225D1 , 0x232129D1 , 0x232229D1 , 0x2322F9EF ,
  0x23233451 , 0x2323F9EF , 0x23312551 , 0x233229D1 , 0x2332F9EF , 0x2333F56F , 0x237FF557 , 0x238569D5 , 0x23D955D7 , 0x24100BE7 ,
  0x248789E7 , 0x24E315D7 , 0x24FFFBEF , 0x259869E7 , 0x25DFF5EF , 0x25FFFBEF , 0x268789E7 , 0x26DFA5D7 , 0x26FFFBEF , 0x279649E7 ,
  0x27E425D7 , 0x27FFFBEF , 0x288879E7 , 0x28EFF5EF , 0x28FFFBEF , 0x298439E7 , 0x29F115EF , 0x29FFFBEF , 0x2A7659E7 , 0x2AEF75D7 ,
  0x2AFFFBEF , 0x2B7C89E7 , 0x2BEF95D7 , 0x2BFFFBEF , 0x2C6659E7 , 0x2CD555D7 , 0x2CFFFBEF , 0x2D6329E7 , 0x2DDD55E7 , 0x2DFFFBEB ,
  0x2E8479D7 , 0x2EEE35E7 , 0x2EFFFBEF , 0x2F5459E7 , 0x2FCF85D7 , 0x2FFEFBEB , 0x2FFFA5EF , 0x2FFFEBEF , 0x30001AE7 , 0x30002001 ,
  0x311129C0 , 0x31221015 , 0x31232000 , 0x31332451 , 0x32112540 , 0x32131027 , 0x32212440 , 0x33452455 , 0x4000F9D7 , 0x4122F9D7 ,
  0x43F65555 , 0x43FFF5D7 , 0x44F55567 , 0x44FFF5D7 , 0x45F00557 , 0x45FFF5D7 , 0x46F659D7 , 0x471005E7 , 0x47F449E7 , 0x481005E7 ,
  0x48EFA9D5 , 0x48FFF5EF , 0x49F449E7 , 0x49FFF5EF , 0x4AEA79E7 , 0x4AFFF5EF , 0x4BE9C9D5 , 0x4BFFF5EF , 0x4CE549E7 , 0x4CFFF5EF ,
  0x4DE359E7 , 0x4DFFF5D7 , 0x4EE469E7 , 0x4EFFF5D7 , 0x4FEF39E7 , 0x4FFFF5EF , 0x6000F9E7 , 0x69FFF557 , 0x6FFFF9D7 , 0x811009D7 ,
  0x8EFFF555 , 0xFFFFF9E7];

/// ArithDecoderData structure
#[repr(C)]
#[derive(Debug)]
pub struct ArithDecoderData {
    number_lines_prev: usize, // Number of quantized spectral coefficients from previous window
    c_prev: [u8; (AC_MAX_FRAMESIZE / 2) + 4], // 2-tuple context of previous frame, 4 bit
}

/// Temporary arith decode state structure
/// Keeps context state of decoding within single frame
#[repr(C)]
#[derive(Debug)]
struct Tastat {
    low: u16,
    high: u16,
    vobf: u16, // (bits) value to be decoded
}

/// Default trait for ArithDecoderData
impl Default for ArithDecoderData {
    fn default() -> Self {
        Self {
            number_lines_prev: 0,
            c_prev: [0; (AC_MAX_FRAMESIZE / 2) + 4],
        }
    }
}

impl ArithDecoderData {
    /// Creates a new `ArithDecoderData` instance.
    pub fn new() -> ArithDecoderData {
        Default::default()
    }

    /// Resets `ArithDecoderData` context lines.
    pub fn reset(&mut self) {
        self.number_lines_prev = 0;
    }

    /// Decodes a spectral data element by using an adaptive context dependent
    /// arithmetic coding scheme.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `spectrum_coeff`: Quantized spectral coefficients output
    /// - `lg`: Number of quantized spectral coefficients output by the arithmetic decoder
    /// - `lg_max`: Maximum number of quantized spectral coefficients
    /// - `arith_reset_flag`: Flag which indicates if the spectral noiseless context must be reset
    ///
    ///  Returns false - decoding error, true - success
    ///
    /// # Examples
    /// ```
    /// use aac::arith_coding::arith_dec::ArithDecoderData;
    /// use aac::common::{bitstream::Bitstream, bitstream::Mode, flags};
    ///
    /// // Precondition, should have a valid Bitstream instance
    /// // Refer respective components for instance creation, read/write methods
    /// let buffer = vec![0; 8];
    /// let mut bitstream_reader = Bitstream::new(buffer.len(), Mode::Reader);
    /// bitstream_reader.init(&buffer, 40);
    ///
    /// let lg = 128;
    /// let lg_max = 1024;
    /// let mut spectrum_coeff = vec![0_i16; 1024];
    /// let reset_flag = false;
    ///
    /// let mut arco_data = ArithDecoderData::new();
    /// arco_data.decode(
    ///     &mut bitstream_reader,
    ///     &mut spectrum_coeff,
    ///     lg,
    ///     lg_max,
    ///     reset_flag,
    /// );
    /// ```
    pub fn decode(
        &mut self,
        bs: &mut Bitstream,
        spectrum_coeff: &mut [i16],
        lg: usize,
        lg_max: usize,
        arith_reset_flag: bool,
    ) -> bool {
        // lg - Number of quantized spectral coefficients output by the arithmetic decoder
        // Check lg and lg_max consistency.
        if lg_max < lg {
            return false; // !is_decode_success
        }

        debug_assert!(spectrum_coeff.len() >= lg_max);
        spectrum_coeff[..lg_max].fill(0);

        // arith_map_context
        if arith_reset_flag {
            self.c_prev[..(lg_max / 2) + 4].fill(0);
        } else if lg_max != self.number_lines_prev {
            if self.number_lines_prev == 0 {
                // Cannot decode without a valid AC context.
                return false;
            }

            // Short-to-long or long-to-short block transition
            // Current length differs compared to previous - perform up/downmix of `self.c_prev[]`.
            // amrwb - Adaptive Multi-rate wide band
            self.copy_table_amrwb_arith2(self.number_lines_prev >> 1, lg_max >> 1);
        }

        self.number_lines_prev = lg_max;

        let mut is_decode_success = if lg > 0 {
            self.decode2(bs, spectrum_coeff, lg >> 1, lg_max >> 1)
        } else {
            self.c_prev[2..2 + (lg_max >> 1)].fill(1);
            true
        };

        if bs.valid_bits() < 0 {
            is_decode_success = false;
        }

        is_decode_success
    }

    /// Copies and maps the context elements of the previous frame.
    /// The context elements within `c_prev` are stored on 4 bits per 2-tuple.
    /// amrwb - Adaptive Multi-rate wide band
    ///
    /// # Parameters
    ///
    /// - `size_in`: Half of maximum number of quantized spectral coefficients of previous frame
    /// - `size_out`: Half of maximum number of quantized spectral coefficients of current frame
    #[inline]
    fn copy_table_amrwb_arith2(&mut self, size_in: usize, size_out: usize) {
        let table = &mut self.c_prev[2..];
        if size_in < size_out {
            table[size_out] = table[size_in];
            table[size_out + 1] = table[size_in + 1];

            let k = match size_in.cmp(&(size_out >> 2)) {
                Ordering::Less => 8,
                Ordering::Equal => 4,
                Ordering::Greater => 2,
            };

            let mut i = size_out;
            for item in (0..size_in).rev() {
                let data = table[item];
                table[i - k..i].fill(data);
                i -= k;
            }
        } else {
            let k = match size_out.cmp(&(size_in >> 2)) {
                Ordering::Less => 8,
                Ordering::Equal => 4,
                Ordering::Greater => 2,
            };

            let mut n = 0;
            for i in 0..size_out {
                table[i] = table[n];
                n += k;
            }

            table[size_out] = table[size_in];
            table[size_out + 1] = table[size_in + 1];
        }
    }

    /// Decodes a spectral data element at second level decoding.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `spectrum_coeff`: Quantized spectral coefficients output
    /// - `n`: Half of number of quantized spectral coefficients output by the arithmetic decoder
    /// - `nt`: Half of maximum number of quantized spectral coefficients
    ///
    ///  Returns false - decoding error, true - success
    fn decode2(
        &mut self,
        bs: &mut Bitstream,
        spectrum_coeff: &mut [i16],
        n: usize,
        nt: usize,
    ) -> bool {
        let start_offset = 2;

        // Context of current frame 3 time steps ago
        let mut c_3 = 0;
        // Context of current frame 2 time steps ago
        let mut c_2 = 0;
        // Context of current frame 1 time steps ago
        let mut c_1 = 0;
        // Context of current frame to be calculated
        let mut c_0;

        let mut ari_state = Tastat {
            low: 0,
            // 2^CBITSNEW - 1
            high: 0xFFFF,
            // First context symbol of sequence (from bitstream)
            vobf: bs.read(CONTEXT_BITSNEW) as u16,
        };

        // arith_map_context
        let mut state_inc = u32::from(self.c_prev[start_offset]) << 12;
        let mut count = 0;
        for i in 0..n {
            // arith_get_context
            let mut s = state_inc >> 8;
            s += u32::from(self.c_prev[start_offset + i + 1]) << 8;
            s = (s << 4) + c_1;

            state_inc = s;

            if i > 3 {
                // Cumulative amplitude below 2
                if (c_1 + c_2 + c_3) < 5 {
                    s += 0x10000;
                }
            }

            // MSBs decoding
            let mut lev = 0;
            let mut esc_nb = 0;
            let mut r;
            loop {
                // pki - Index of the probability model
                let pki = self.get_pk(s + (esc_nb << (VAL_ESC + 1)));
                r = self.decode_14bits(bs, &mut ari_state, &ARI_PK[pki], VAL_ESC + 1);
                if r < VAL_ESC as usize {
                    break;
                }

                lev += 1;

                if lev > 23 {
                    // decoding error
                    return false;
                }

                if esc_nb < 7 {
                    esc_nb += 1;
                }
            }

            // Stop symbol
            if r == 0 {
                if esc_nb > 0 {
                    break;
                }
                c_0 = 1;
            } else {
                let mut b = r >> 2;
                let mut a = r & 0x3;

                // LSBs decoding
                for _level in 0..lev {
                    {
                        let lsb_idx = if a == 0 {
                            1
                        } else if b == 0 {
                            0
                        } else {
                            2
                        };
                        r = self.decode_14bits(bs, &mut ari_state, &ARI_LSB2[lsb_idx], 4);
                    }
                    a = (a << 1) | (r & 1);
                    b = (b << 1) | (r >> 1);
                }

                // spectrum_coeff is i16, one additional sign bit will be read out later.
                // Therefore a and b need to fit into 15 bits at this place.
                // 0xFFFF8000 is the same as i16::MAX.
                if ((a as u32 | b as u32) & 0xFFFF8000) != 0 {
                    return false;
                }

                spectrum_coeff[2 * i] = a as i16;
                spectrum_coeff[2 * i + 1] = b as i16;

                c_0 = (a + b + 1).min(0xF);
            }

            // arith_update_context
            c_3 = c_2;
            c_2 = c_1;
            c_1 = c_0 as u32;
            self.c_prev[start_offset + i] = c_0 as u8;

            // Increament the counter to keep track of loop iteration
            count += 1;
        }

        bs.push(-((CONTEXT_BITSNEW - 2) as isize));

        // We need to run only from 0 to i-1 since all other q[i][1].a,b will be cleared later
        for spec_coef in spectrum_coeff.chunks_exact_mut(2).take(count) {
            let mut bits = 0;
            if spec_coef[0] != 0 {
                bits += 1;
            }
            if spec_coef[1] != 0 {
                bits += 1;
            }

            if bits != 0 {
                let r = bs.read(bits);
                if spec_coef[0] != 0 && (r >> (bits - 1) == 0) {
                    spec_coef[0] = -spec_coef[0];
                }
                if spec_coef[1] != 0 && (r & 1 == 0) {
                    spec_coef[1] = -spec_coef[1];
                }
            }
        }

        self.c_prev[start_offset + count..start_offset + nt].fill(1);

        true // decoding success
    }

    /// Gets index of cumulative frequency table by mapping state of context.
    ///
    /// # Parameters
    ///
    /// - `s`: state of context
    ///
    ///  Returns Index of the probability model (cumulative frequency table)
    ///  Index value shall be in range [0..63]
    fn get_pk(&self, s: u32) -> usize {
        let hash_ps = &ARI_MERGED_HASH_PS[..];
        let s12 = (s.max(1_u32) << 12) - 1;
        let mut p = 0_usize;

        if s12 > hash_ps[485] {
            p += 486; // 742 - 256 = 486
        } else if s12 > hash_ps[255] {
            p += 256;
        }

        if s12 > hash_ps[p + 127] {
            p += 128;
        }
        if s12 > hash_ps[p + 63] {
            p += 64;
        }
        if s12 > hash_ps[p + 31] {
            p += 32;
        }
        if s12 > hash_ps[p + 15] {
            p += 16;
        }
        if s12 > hash_ps[p + 7] {
            p += 8;
        }
        if s12 > hash_ps[p + 3] {
            p += 4;
        }
        if s12 > hash_ps[p + 1] {
            p += 2;
        }
        let mut data = hash_ps[p];
        if s12 > data {
            data = hash_ps[p + 1];
        }
        if s != (data >> 12) {
            data >>= 6;
        }
        (data & 0x3F) as usize
    }

    /// Gets index of cumulative frequency table by mapping state of context.
    ///
    /// # Parameters
    ///
    /// - `bs`: Bitstream instance with valid internal data
    /// - `s`: state of arithmetic decoding
    /// - `cum_freq`: Cumulative frequencies table
    /// - `cfl`: Length of cumulative frequencies table
    ///
    ///  Returns  decoded symbol
    fn decode_14bits(
        &self,
        bs: &mut Bitstream,
        s: &mut Tastat,
        cum_freq: &[i16],
        cfl: u8,
    ) -> usize {
        let mut low: i32 = s.low as i32;
        let mut high = s.high as i32;
        let mut value = s.vobf as i32;

        let range = high - low + 1;
        let cum = ((value - low + 1) << STATE_BITSNEW) - 1;

        let mut p = 0;
        if cfl == (VAL_ESC + 1) {
            // In 50% of all cases, the first entry is the right one, so we check it
            // prior to all others.
            // p=0 to point first location in table.
            if (cum_freq[p] as i32 * range) > cum {
                if (cum_freq[p + 8] as i32 * range) > cum {
                    p += 8;
                }
                if (cum_freq[p + 4] as i32 * range) > cum {
                    p += 4;
                }
                if (cum_freq[p + 2] as i32 * range) > cum {
                    p += 2;
                }
                if (cum_freq[p + 1] as i32 * range) > cum {
                    p += 1;
                }
                p += 1;
            }
        } else if cfl == 4 {
            if (cum_freq[p + 1] as i32 * range) > cum {
                p += 2;
            }
            if (cum_freq[p] as i32 * range) > cum {
                p += 1;
            }
        }

        let symbol = p;

        if symbol != 0 {
            high = low + self.mul_sbc_14bits(range, cum_freq[symbol - 1] as i32) - 1;
        }

        low += self.mul_sbc_14bits(range, cum_freq[symbol] as i32);
        let mut us_high = high as u16;
        let mut us_low = low as u16;
        loop {
            if us_high & 0x8000 != 0 && us_low & 0x8000 == 0 {
                if (us_low & 0x4000 != 0) && (us_high & 0x4000 == 0) {
                    us_low -= 0x4000;
                    us_high -= 0x4000;
                    value -= 0x4000;
                } else {
                    break;
                }
            }
            us_low <<= 1;
            us_high = (us_high << 1) | 1;
            value = (value << 1) | bs.read_bit() as i32;
        }
        s.low = us_low;
        s.high = us_high;
        s.vobf = (value & 0xFFFF) as u16;
        symbol
    }

    /// Right shift multiplication results to 14 bits
    #[inline(always)]
    fn mul_sbc_14bits(&self, range: i32, cum_freq: i32) -> i32 {
        (range * cum_freq) >> STATE_BITSNEW
    }
}
