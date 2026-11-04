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
//! QMF frequency pre-whitening for SBR

use itertools::izip;

const MAX_LOW_BANDS: usize = 32;
const POLY_ORDER: usize = 3;
// log2(10)/20 * 2^2
const LOG10FAC_INV: f32 = 0.664385618977472;

/// BSD[] begins at index 0 with data for num_bands=5. The correct BSD[] is
/// indexed like BSD[num_bands-BSD_IDX_OFFSET].
const BSD_IDX_OFFSET: usize = 5;
/// Number of `BacksubstData`` elements in bsd.
const N_NUMBANDS: usize = MAX_LOW_BANDS - BSD_IDX_OFFSET + 1;

#[repr(C)]
#[derive(Debug)]
struct BacksubstData {
    /// Normalized `L` matrix.
    l_norm1d: [f32; 3],
    /// The diagonal data points [i][i] of the normalized `L` matrix.
    l_normii: [f32; 3],
    /// To normalize `L*x=b`, `b_mul0` is what we need to multiply `b` with.
    b_mul0: [f32; 4],
    /// Normalized inverted `L` matrix (`L'`).
    l_norm_inv1d: [f32; 6],
    /// To normalize `L'*x=b`, `b_mul1` is what we need to multiply `b` with.
    b_mul1: [f32; 4],
}

impl BacksubstData {
    /// Solves `L*x=b` via back substitution according to the following
    /// structure:
    ///
    ///  x[3] = b[3];
    ///  x[2] = b[2] - L[2][3]*x[3];
    ///  x[1] = b[1] - L[1][2]*x[2] - L[1][3]*x[3];
    ///  x[0] = b[0] - L[0][1]*x[1] - L[0][2]*x[2] - L[0][3]*x[3];
    ///
    /// # Parameters
    ///
    /// - `b`: The `b` in `L*x=b` (one-dimensional).
    /// - `x`: Solution vector.
    fn back_substitution(&self, b: &[f32; POLY_ORDER + 1], x: &mut [f32; POLY_ORDER + 1]) {
        x[POLY_ORDER] = b[POLY_ORDER];
        // The trip counter that indexes incrementally through l_norm_inv1d[].
        let mut m = 0;

        for (i, b_val) in b[..POLY_ORDER].iter().enumerate().rev() {
            let mut sum = *b_val;
            for x_val in x[i + 1..=POLY_ORDER].iter() {
                sum -= self.l_norm_inv1d[m] * *x_val;
                m += 1;
            }
            x[i] = sum;
        }
    }

    /// Solves a system of linear equations (`A*x=b`, with `A=LL*`) with the
    /// Cholesky algorithm.
    ///
    /// # Parameters
    ///
    /// - `b`: Input and output solution vector.
    fn cholesky_solve(&self, b: &mut [f32; POLY_ORDER + 1]) {
        let b_mul0 = self.b_mul0;
        let b_mul1 = self.b_mul1;

        // Normalize b.
        let mut b_normed = [0.0_f32; POLY_ORDER + 1];
        izip!(b_normed.iter_mut(), b.iter(), b_mul0.iter())
            .take(POLY_ORDER + 1)
            .for_each(|(b_nor, p, b_mul0)| *b_nor = *p * *b_mul0);

        self.forward_substitution(&b_normed, b);

        // Normalize b again.
        izip!(b_normed.iter_mut(), b.iter(), b_mul1.iter())
            .take(POLY_ORDER + 1)
            .for_each(|(b_nor, p, b_mul1)| *b_nor = *p * *b_mul1);

        self.back_substitution(&b_normed, b);
    }

    /// Solves `L*x=b` via forward substitution according to the following
    /// structure:
    ///
    ///  x[0] =  b[0];
    ///  x[1] = (b[1]                               - x[0]) / L[1][1];
    ///  x[2] = (b[2] - x[1]*L[2][1]                - x[0]) / L[2][2];
    ///  x[3] = (b[3] - x[2]*L[3][2] - x[1]*L[3][1] - x[0]) / L[3][3];
    ///
    /// # Parameters
    ///
    /// - `b`: The `b` in `L*x=b` (one-dimensional).
    /// - `x`: Output polynomial coefficients.
    fn forward_substitution(&self, b: &[f32; POLY_ORDER + 1], x: &mut [f32; POLY_ORDER + 1]) {
        x[0] = b[0];
        // The trip counter that indexes incrementally through l_norm1d[].
        let mut m = 0;

        for (i, (normii, b_val)) in izip!(self.l_normii.iter(), b[1..].iter()).enumerate() {
            let mut sum = *b_val;
            for x_val in x[1..].iter().take(i).rev() {
                sum -= self.l_norm1d[m] * *x_val;
                m += 1;
            }
            sum -= x[0];

            // Instead of the division /L[i][i], we multiply by the inverse.
            x[i + 1] = sum * *normii;
        }
    }

    /// Find polynomial approximation of vector `y` with implicit abscisas
    /// `x=0,1,2,3..n-1`.
    ///
    ///  The problem (`V^T * V * p = V^T * y`) is solved with Cholesky.
    ///  `V` is the Vandermonde Matrix constructed with `x = 0...n-1`;
    ///  `A = V^T * V; b = V^T * y;`
    ///
    /// # Parameters
    ///
    /// - `y`: Input.
    /// - `p`: Output polynomial coefficients.
    fn polyfit(&self, y: &[f32], p: &mut [f32; POLY_ORDER + 1]) {
        let mut v = [0.0f32; POLY_ORDER + 1];

        // Construct vector b[], temporarily stored in p[].
        p.fill(0.0);

        v[0] = 1.0;
        for (k, y_val) in y.iter().enumerate() {
            for i in 1..=POLY_ORDER {
                v[i] = k as f32 * v[i - 1];
            }

            for (p_val, v_val) in izip!(p.iter_mut(), v.iter().rev()) {
                *p_val += *v_val * *y_val;
            }
        }

        self.cholesky_solve(p);
    }

    /// Calculates the output of a `POLY_ORDER`-degree polynomial function
    /// with Horner scheme:
    ///
    ///    `y(x) = p3 + p2*x + p1*x^2 + p0*x^3`
    ///         `= p3 + x*(p2 + x*(p1 + x*p0))`
    ///
    /// # Parameters
    ///
    /// - `p`: Coefficients as in `y(x)`.
    /// - `x`: Non-fractional integer representation of `x` as in `y(x)`.
    ///
    /// # Return
    ///
    ///    `y(x)`
    fn polyval(p: &[f32; POLY_ORDER + 1], x: usize) -> f32 {
        if x == 0 {
            return p[POLY_ORDER];
        }

        let mut result = p[0];

        for p_val in p.iter().take(POLY_ORDER + 1).skip(1) {
            result = x as f32 * result + *p_val;
        }
        result
    }
}

/// Calculates pre-whitening gains, to flatten the QMF frequency bands whithout loosing
/// the fine structure.
///
/// # Parameters
///
/// - `source_buffer_real`: Real part of QMF domain data.
/// - `source_buffer_imag`: Imaginary part of QMF domain data.
/// - `pre_whitening_gains`: Buffer for gain values (one for each QMF band).
/// - `num_bands`: Number of subsamples in low frequency subbands (in QMF data).
/// - `start_sample`: Time slot start.
/// - `stop_sample`: Time slot stop.
pub(super) fn calc_pre_whitening_gains(
    source_buffer_real: &[&[f32]],
    source_buffer_imag: &[&[f32]],
    pre_whitening_gains: &mut [f32],
    num_bands: usize,
) {
    let mut mean_nrg = 0.0;
    let mut low_env = [0.0f32; MAX_LOW_BANDS];
    let mut poly = [0.0f32; POLY_ORDER + 1];
    let inverse_of_num_slots = 1.0 / (source_buffer_real.len() as f32);

    // Calculate the spectral envelope in dB over the current copy-up frame.
    for (re, im) in izip!(source_buffer_real.iter(), source_buffer_imag.iter(),) {
        // Calculate the energy of each sample in the slot and accumulate it in low_env[].
        for (le_nrg, re_val, im_val) in
            izip!(low_env.iter_mut(), re.iter(), im.iter()).take(num_bands)
        {
            *le_nrg += (*re_val * *re_val) + (*im_val * *im_val);
        }
    }

    for le_nrg in low_env.iter_mut().take(num_bands) {
        // Calculate energy per sample in dB.
        let mut nrg = *le_nrg * inverse_of_num_slots;
        nrg = 0.25 * 10.0 * f32::log10(nrg + 1.0);

        *le_nrg = nrg;
        mean_nrg += nrg;
    }
    mean_nrg /= num_bands as f32;

    // Subtract mean before polynomial approximation to reduce dynamic of p[].
    for le_val in low_env.iter_mut().take(num_bands) {
        *le_val = mean_nrg - *le_val;
    }

    if num_bands > POLY_ORDER + 1 {
        // Find polynomial approximation of low_env.
        let bsd = &BSD[num_bands - BSD_IDX_OFFSET];
        bsd.polyfit(&low_env[..num_bands], &mut poly);

        for (index, gv) in pre_whitening_gains.iter_mut().enumerate().take(num_bands) {
            // GainVec = 10^((mean(y)-y)/20) = 2^( (mean(y)-y) * log2(10)/20 ).
            let tmp = BacksubstData::polyval(&poly, index) * LOG10FAC_INV;
            *gv = (2.0_f32).powf(tmp);
        }
    } else {
        // num_bands <= POLY_ORDER+1.
        for (le, gv) in izip!(low_env.iter(), pre_whitening_gains.iter_mut()).take(num_bands) {
            // GainVec = 10^((mean(y)-y)/20) = 2^( (mean(y)-y) * log2(10)/20 ).
            let tmp = *le * LOG10FAC_INV;
            *gv = (2.0_f32).powf(tmp);
        }
    }
}

/// `BSD[]` holds Vandermonde matrix coefficients, which are used to solve polynomial equations
/// with a degree of `POLY_ORDER`.
#[rustfmt::skip]
static BSD: [BacksubstData; N_NUMBANDS] = [
    BacksubstData {
        // num_bands=5
        l_norm1d: [0.401494641, 0.519314471, 0.824151396],
        l_normii: [6.415549790, 10.290879990, 1.419780136],
        b_mul0: [0.014300314, 0.053791181, 0.197538234, 0.699285349],
        l_norm_inv1d: [1.509661836, 0.701412567, 0.406721870, 0.265848671,0.072392638,0.020449898],
        b_mul1: [0.014300314, 0.345099998, 2.032842263, 0.992831449],
    },
    BacksubstData {
        // num_bands=6
        l_norm1d: [0.408511312, 0.564508767, 0.831118199],
        l_normii: [6.235735171, 9.820522490, 1.539409527],
        b_mul0: [0.006981749, 0.032368493, 0.146302945, 0.636580370],
        l_norm_inv1d: [1.274104683, 0.563587259, 0.263523711, 0.215695832,0.047721180,0.010967585],
        b_mul1: [0.006981749, 0.201841353, 1.436771363, 0.979957887],
    },
    BacksubstData {
        // num_bands=7
        l_norm1d: [0.412525906, 0.590192817, 0.835116662],
        l_normii: [6.139281546, 9.599887820, 1.639665810],
        b_mul0: [0.003858416, 0.021242004, 0.113922500, 0.587695435],
        l_norm_inv1d: [1.098290598, 0.472231281, 0.185313843, 0.181640887,0.033868783,0.006565333],
        b_mul1: [0.003858416, 0.130410641, 1.093643218, 0.963624112],
    },
    BacksubstData {
        // num_bands=8
        l_norm1d: [0.415036780, 0.606247216, 0.837621042],
        l_normii: [6.081160513, 9.476758192, 1.724231670],
        b_mul0: [0.002326085, 0.014820290, 0.091939043, 0.548350723],
        l_norm_inv1d: [0.963276836, 0.406845735, 0.137667784, 0.156952711,0.025300292,0.004241965],
        b_mul1: [0.002326085, 0.090124561, 0.871284082, 0.945483683],
    },
    BacksubstData {
        // num_bands=9
        l_norm1d: [0.416711422, 0.616963875, 0.839292656],
        l_normii: [6.043298424, 9.400374312, 1.796224413],
        b_mul0: [0.001495766, 0.010822223, 0.076214508, 0.515859308],
        l_norm_inv1d: [0.856862745, 0.357592403, 0.106407608, 0.138212473,0.019625742,0.002899562],
        b_mul1: [0.001495766, 0.065401925, 0.716444902, 0.926599082],
    },
    BacksubstData {
        // num_bands=10
        l_norm1d: [0.417884011, 0.624475947, 0.840463674],
        l_normii: [6.017204854, 9.349456262, 1.858105301],
        b_mul0: [0.001010976, 0.008186580, 0.064510766, 0.488465960],
        l_norm_inv1d: [0.771080928, 0.319095650, 0.084758247, 0.123491806,0.015671424,0.002069695],
        b_mul1: [0.001010976, 0.049260331, 0.603140581, 0.907621190],
    },
    BacksubstData {
        // num_bands=11
        l_norm1d: [0.418736978, 0.629946017, 0.841315758],
        l_normii: [5.998435758, 9.313691006, 1.911784700],
        b_mul0: [0.000710955, 0.006369559, 0.055522751, 0.464977803],
        l_norm_inv1d: [ 0.700589971, 0.288149394, 0.069131213, 0.111617692,0.012804759,0.001529009],
        b_mul1: [0.000710955, 0.038207390, 0.517121748, 0.888937450],
    },
    BacksubstData {
        // num_bands=12
        l_norm1d: [0.419376799, 0.634052832, 0.841955051],
        l_normii: [5.984472363, 9.287546819, 1.958745696],
        b_mul0: [0.000516400, 0.005070973, 0.048443561, 0.444555302],
        l_norm_inv1d: [0.641706924, 0.262715387, 0.057475172, 0.101834523,0.010659830,0.001161611],
        b_mul1: [0.000516400, 0.030347098, 0.449921838, 0.870770784],
    },
    BacksubstData {
        // num_bands=13
        l_norm1d: [0.419869025, 0.637214637, 0.842446945],
        l_normii: [5.973796786, 9.267824125, 2.000147497],
        b_mul0: [0.000385301, 0.004115012, 0.042750303, 0.426589559],
        l_norm_inv1d: [0.591823899, 0.241432746, 0.048546047, 0.093633118,0.009012834,0.000903213],
        b_mul1: [0.000385301, 0.024582244, 0.396202287, 0.853242040],
    },
    BacksubstData {
        // num_bands=14
        l_norm1d: [0.420255808, 0.639700677, 0.842833510],
        l_normii: [5.965448484, 9.252560822, 2.036904596],
        b_mul0: [0.000294083, 0.003393615, 0.038090821, 0.410627426],
        l_norm_inv1d: [0.549049049, 0.223356743, 0.041552726, 0.086657605,0.007720562,0.000716179],
        b_mul1: [0.000294083, 0.020244436, 0.352437641, 0.836408890],
    },
    BacksubstData {
        // num_bands=15
        l_norm1d: [0.420565255, 0.641690680, 0.843142810],
        l_normii: [5.958794815, 9.240496072, 2.069745705],
        b_mul0: [0.000228861, 0.002837643, 0.034220187, 0.396324089],
        l_norm_inv1d: [0.511979926, 0.207810333, 0.035972184, 0.080651645,0.006687881,0.000577458],
        b_mul1: [0.000228861, 0.016908930, 0.316211508, 0.820290081],
    },
    BacksubstData {
        // num_bands=16
        l_norm1d: [0.420816696, 0.643308351, 0.843394147],
        l_normii: [5.953404977, 9.230788034, 2.099257344],
        b_mul0: [0.000181122, 0.002401328, 0.030963334, 0.383412079],
        l_norm_inv1d: [0.479556618, 0.194295261, 0.031447183, 0.075425845,0.005849571,0.000472396],
        b_mul1: [0.000181122, 0.014296080, 0.285815969, 0.804880623],
    },
    BacksubstData {
        // num_bands=17
        l_norm1d: [0.421023773, 0.644641100, 0.843601152],
        l_normii: [5.948977195, 9.222856348, 2.125916154],
        b_mul0: [0.000145463, 0.002053483, 0.028192156, 0.371680406],
        l_norm_inv1d: [0.450964187, 0.182436499, 0.027726873, 0.070837206,0.005159697,0.000391366],
        b_mul1: [0.000145463, 0.012216123, 0.260012202, 0.790161379],
    },
    BacksubstData {
        // num_bands=18
        l_norm1d: [0.421196345, 0.645752112, 0.843773669],
        l_normii: [5.945294881, 9.216289919, 2.150113038],
        b_mul0: [0.000118347, 0.001772308, 0.025810985, 0.360960155],
        l_norm_inv1d: [0.425566343, 0.171946258, 0.024630851, 0.066775718,0.004585147,0.000327868],
        b_mul1: [0.000118347, 0.010536896, 0.237881519, 0.776105135],
    },
    BacksubstData {
        // num_bands=19
        l_norm1d: [0.421341672, 0.646687980, 0.843918957],
        l_normii: [5.942199261, 9.210790672, 2.172171331],
        b_mul0: [0.000097400, 0.001542230, 0.023747086, 0.351114318],
        l_norm_inv1d: [0.402859381, 0.162599946, 0.022026653, 0.063155350,0.004101559,0.000277403],
        b_mul1: [0.000097400, 0.009164237, 0.218729435, 0.762680455],
    },
    BacksubstData {
        // num_bands=20
        l_norm1d: [0.421465204, 0.647483670, 0.844042458],
        l_normii: [5.939571769, 9.206138054, 2.192360620],
        b_mul0: [0.000080989, 0.001351899, 0.021944279, 0.342030463],
        l_norm_inv1d: [0.382439580, 0.154219684, 0.019815220, 0.059907851,0.003690683,0.000236790],
        b_mul1: [0.000080989, 0.008029702, 0.202022062, 0.749854117],
    },
    BacksubstData {
        // num_bands=21
        l_norm1d: [0.421571091, 0.648165841, 0.844148322],
        l_normii: [5.937322397, 9.202165937, 2.210907356],
        b_mul0: [0.000067970, 0.001192904, 0.020358557, 0.333615357],
        l_norm_inv1d: [0.363979704, 0.146662735, 0.017921266, 0.056978374,0.003338631,0.000203737],
        b_mul1: [0.000067970, 0.007082653, 0.187342824, 0.737592647],
    },
    BacksubstData {
        // num_bands=22
        l_norm1d: [0.421662541, 0.648755102, 0.844239753],
        l_normii: [5.935381806, 9.198747204, 2.228003075],
        b_mul0: [0.000057522, 0.001058909, 0.018955011, 0.325790959],
        l_norm_inv1d: [0.347211803, 0.139813216, 0.016286734, 0.054322335,0.003034680,0.000176562],
        b_mul1: [0.000057522, 0.006285028, 0.174362353, 0.725863259],
    },
    BacksubstData {
        // num_bands=23
        l_norm1d: [0.421742064, 0.649267588, 0.844319262],
        l_normii: [5.933695877, 9.195783207, 2.243810823],
        b_mul0: [0.000049053, 0.000945078, 0.017705629, 0.318491382],
        l_norm_inv1d: [0.331914429, 0.133576056, 0.014866272, 0.051903123,0.002770447,0.000154015],
        b_mul1: [0.000049053, 0.005607807, 0.162817130, 0.714634411],
    },
    BacksubstData {
        // num_bands=24
        l_norm1d: [0.421811647, 0.649716079, 0.844388834],
        l_normii: [5.932221865, 9.193196418, 2.258470232],
        b_mul0: [0.000042121, 0.000847672, 0.016587707, 0.311660571],
        l_norm_inv1d: [0.317903009, 0.127872516, 0.013624031, 0.049690386,0.002539298,0.000135151],
        b_mul1: [0.000042121, 0.005028576, 0.152494046, 0.703876122],
    },
    BacksubstData {
        // num_bands=25
        l_norm1d: [0.421872881, 0.650110803, 0.844450059],
        l_normii: [5.930925640, 9.190925214, 2.272101549],
        b_mul0: [0.000036400, 0.000763763, 0.015582662, 0.305250494],
        l_norm_inv1d: [0.305022507, 0.122636822, 0.012531369, 0.047658752,0.002335928,0.000119246],
        b_mul1: [0.000036400, 0.004529820, 0.143219080, 0.693560121],
    },
    BacksubstData {
        // num_bands=26
        l_norm1d: [0.421927050, 0.650460022, 0.844504220],
        l_normii: [5.929779686, 9.188920099, 2.284808873],
        b_mul0: [0.000031640, 0.000691039, 0.014675159, 0.299219729],
        l_norm_inv1d: [0.293141759, 0.117813600, 0.011565182, 0.045786846,0.002156057,0.000105743],
        b_mul1: [0.000031640, 0.004097707, 0.134848868, 0.683659891],
    },
    BacksubstData {
        // num_bands=27
        l_norm1d: [0.421975201, 0.650770471, 0.844552365],
        l_normii: [5.928761617, 9.187140943, 2.296682761],
        b_mul0: [0.000027652, 0.000627653, 0.013852443, 0.293532337],
        l_norm_inv1d: [0.282149070, 0.113355891, 0.010706664, 0.044056529,0.001996197,0.000094205],
        b_mul1: [0.000027652, 0.003721204, 0.127264342, 0.674150658],
    },
    BacksubstData {
        // num_bands=28
        l_norm1d: [0.422018194, 0.651047683, 0.844595353],
        l_normii: [5.927853069, 9.185554927, 2.307802354],
        b_mul0: [0.000024288, 0.000572118, 0.013103822, 0.288156968],
        l_norm_inv1d: [0.271948736, 0.109223615, 0.009940378, 0.042452313,0.001853486,0.000084287],
        b_mul1: [0.000024288, 0.003391434, 0.120365874, 0.665009328],
    },
    BacksubstData {
        // num_bands=29
        l_norm1d: [0.422056739, 0.651296238, 0.844633893],
        l_normii: [5.927038863, 9.184134997, 2.318237113],
        b_mul0: [0.000021432, 0.000523228, 0.012420280, 0.283066131],
        l_norm_inv1d: [0.262458293, 0.105382350, 0.009253564, 0.040960890,0.001725554,0.000075713],
        b_mul1: [0.000021432, 0.003101190, 0.114069527, 0.656214411],
    },
    BacksubstData {
        // num_bands=30
        l_norm1d: [0.422091430, 0.651519955, 0.844668581],
        l_normii: [5.926306364, 9.182858688, 2.328048250],
        b_mul0: [0.000018994, 0.000479992, 0.011794163, 0.278235609],
        l_norm_inv1d: [0.253606306, 0.101802368, 0.008635592, 0.039570760,0.001610429,0.000068265],
        b_mul1: [0.000018994, 0.002844582, 0.108304135, 0.647745922],
    },
    BacksubstData {
        // num_bands=31
        l_norm1d: [0.422122764, 0.651722035, 0.844699912],
        l_normii: [5.925644985, 9.181707217, 2.337289915],
        b_mul0: [0.000016901, 0.000441599, 0.011218938, 0.273643970],
        l_norm_inv1d: [0.245330592, 0.098457863, 0.008077555, 0.038271939,0.001506456,0.000061762],
        b_mul1: [0.000016901, 0.002616757, 0.103009004, 0.639585292],
    },
    BacksubstData {
        // num_bands=32
        l_norm1d: [0.422151160, 0.651905181, 0.844728306],
        l_normii: [5.925045801, 9.180664781, 2.346010178],
        b_mul0: [0.000015095, 0.000407371, 0.010688995, 0.269272174],
        l_norm_inv1d: [0.237576773, 0.095326319, 0.007571942, 0.037055711,0.001412239,0.000056060],
        b_mul1: [0.000015095, 0.002413691, 0.098132084, 0.631715261],
    },
];

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_calc_pre_whitening_gains() {
        // Test Case 1: Normal input.
        let source_buffer_real = &[[1.0; 64].as_slice(); 64];
        let source_buffer_imag = &[[0.1; 64].as_slice(); 64];
        let mut gain_vec = [0.0; 32];
        let start_sample = 0;
        let stop_sample = 64;
        let num_bands = 32;

        calc_pre_whitening_gains(
            &source_buffer_real[start_sample..stop_sample],
            &source_buffer_imag[start_sample..stop_sample],
            &mut gain_vec,
            num_bands,
        );

        // Assert expected values of gain_vec.
        assert_eq!(gain_vec, [1.0; 32]);

        // Test Case 2: num_bands less than or equal to 4.
        let mut gain_vec = [0.0; 4];
        let num_bands = 4;

        calc_pre_whitening_gains(
            &source_buffer_real[start_sample..stop_sample],
            &source_buffer_imag[start_sample..stop_sample],
            &mut gain_vec,
            num_bands,
        );

        // Assert expected values of gain_vec for num_bands <= 4.
        assert_eq!(gain_vec, [1.0; 4]);
    }
}
