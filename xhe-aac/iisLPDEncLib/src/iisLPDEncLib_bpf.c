
/* -----------------------------------------------------------------------------
Software Copyright License for The Fraunhofer FDK Extended High Efficiency AAC
Encoder Software for Android

© Copyright 1995 - 2025 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1.    INTRODUCTION

The Fraunhofer FDK Extended High Efficiency AAC Encoder Software for Android
("FDK Extended High Efficiency AAC Encoder") is software that implements the
encoding of digital audio according to the MPEG-D Unified Speech and Audio
Coding (USAC) standard and MPEG-D Dynamic Range Control (DRC) standard. This FDK
Extended High Efficiency AAC Encoder Software is intended to be used on a wide
variety of Android devices. It is technically not suited to encode content for
digital radio broadcasting services, including DRM and similar standards.

Patent licenses for necessary patent claims for the FDK Extended High Efficiency
AAC Encoder Software (including those of Fraunhofer), for the use in commercial
products and services, may be obtained from the respective patent owners
individually and/or from Via Licensing Alliance (www.via-la.com).

Fraunhofer supports the development of Extended High Efficiency AAC products and
services by offering additional software, documentation, and technical advice.
In addition, it operates the xHE-AAC Trademark Program to ease interoperability
testing of end products. Please visit http://www.xhe-aac.com for more
information.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees, provided that you
satisfy the following conditions:

You must retain the complete text of this software license in redistributions of
the FDK Extended High Efficiency AAC Encoder Software or your modifications
thereto in source code form.

You must retain the complete text of this software license in the documentation
and/or other materials provided with redistributions of the FDK Extended High
Efficiency AAC Encoder Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the
FDK Extended High Efficiency AAC Encoder Software and your modifications thereto
to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived
from this software without prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute
the FDK Extended High Efficiency AAC Encoder Software or your modifications
thereto.

Your modified versions of the FDK Extended High Efficiency AAC Encoder Software
must carry prominent notices stating that you changed the software and the date
of any change. For modified versions of the FDK Extended High Efficiency AAC
Encoder Software, the term "Fraunhofer FDK Extended High Efficiency AAC Encoder
Software for Android" must be replaced by the term "Third-Party Modified Version
of the Fraunhofer FDK Extended High Efficiency AAC Encoder Software for
Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty for patent non-infringement with respect to this
software. You may use this FDK Extended High Efficiency AAC Encoder Software or
modifications thereto only for purposes that are authorized by appropriate
patent licenses.

4.    DISCLAIMER

This FDK Extended High Efficiency AAC Encoder Software is provided by Fraunhofer
on behalf of the copyright holders and contributors "AS IS" and WITHOUT ANY
EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied
warranties of merchantability and fitness for a particular purpose. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if advised
of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - FDK Extended High Efficiency
AAC Encoder
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------- */

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "mathlib.h"
#include "iis_fft.h"

#include "iisLPDEncLib_bpf.h"
#include "iisLPDComLib_constants.h"

#define PEAK_DIFF_THR 6
#define LAG_DIFF_MAX 10.0f

static const float winHamming_768[384] = {
    0.079999983f, 0.080015421f, 0.080061726f, 0.080138884f, 0.08024691f, 0.080385782f, 0.080555528f, 0.080756068f,
    0.080987446f, 0.081249617f, 0.081542581f, 0.081866309f, 0.08222077f, 0.082605965f, 0.083021849f, 0.083468407f,
    0.083945587f, 0.084453404f, 0.084991753f, 0.085560657f, 0.086160041f, 0.086789891f, 0.087450147f, 0.088140786f,
    0.088861741f, 0.089612976f, 0.090394415f, 0.091206051f, 0.092047788f, 0.092919573f, 0.093821384f, 0.094753131f,
    0.09571477f, 0.096706212f, 0.097727396f, 0.098778255f, 0.099858724f, 0.10096874f, 0.10210822f, 0.10327708f,
    0.10447525f, 0.10570265f, 0.10695917f, 0.10824478f, 0.10955934f, 0.1109028f, 0.11227505f, 0.11367601f,
    0.11510556f, 0.11656366f, 0.11805014f, 0.11956496f, 0.12110797f, 0.12267911f, 0.12427825f, 0.12590529f,
    0.12756011f, 0.12924263f, 0.1309527f, 0.13269021f, 0.13445507f, 0.13624711f, 0.13806628f, 0.13991243f,
    0.14178538f, 0.1436851f, 0.14561139f, 0.14756417f, 0.14954327f, 0.15154856f, 0.15357992f, 0.15563725f,
    0.15772034f, 0.1598291f, 0.16196334f, 0.16412297f, 0.16630784f, 0.16851777f, 0.17075261f, 0.17301226f,
    0.17529652f, 0.17760526f, 0.17993833f, 0.18229555f, 0.18467678f, 0.18708184f, 0.1895106f, 0.19196288f,
    0.19443852f, 0.19693732f, 0.19945917f, 0.20200387f, 0.20457125f, 0.20716113f, 0.20977335f, 0.21240774f,
    0.21506408f, 0.21774226f, 0.22044207f, 0.22316329f, 0.22590582f, 0.2286694f, 0.23145385f, 0.23425905f,
    0.23708475f, 0.23993078f, 0.24279693f, 0.24568304f, 0.24858887f, 0.25151432f, 0.25445905f, 0.25742298f,
    0.2604059f, 0.26340756f, 0.26642776f, 0.26946637f, 0.27252311f, 0.27559775f, 0.27869019f, 0.28180015f,
    0.28492746f, 0.28807184f, 0.29123315f, 0.29441115f, 0.29760563f, 0.30081639f, 0.3040432f, 0.30728585f,
    0.31054407f, 0.31381774f, 0.31710657f, 0.32041037f, 0.32372889f, 0.32706192f, 0.33040926f, 0.33377063f,
    0.33714586f, 0.34053472f, 0.34393695f, 0.34735233f, 0.35078064f, 0.35422167f, 0.35767514f, 0.36114085f,
    0.36461857f, 0.36810806f, 0.37160906f, 0.37512141f, 0.37864479f, 0.38217899f, 0.3857238f, 0.38927898f,
    0.39284426f, 0.39641941f, 0.40000418f, 0.40359834f, 0.40720168f, 0.41081393f, 0.41443485f, 0.41806418f,
    0.4217017f, 0.42534715f, 0.42900032f, 0.43266091f, 0.43632871f, 0.44000348f, 0.44368497f, 0.44737288f,
    0.45106703f, 0.45476717f, 0.458473f, 0.46218431f, 0.46590084f, 0.46962234f, 0.47334859f, 0.4770793f,
    0.48081422f, 0.4845531f, 0.48829573f, 0.4920418f, 0.49579111f, 0.4995434f, 0.50329834f, 0.50705582f,
    0.5108155f, 0.51457709f, 0.51834041f, 0.52210522f, 0.52587116f, 0.52963811f, 0.53340572f, 0.53717381f,
    0.54094207f, 0.54471022f, 0.54847813f, 0.55224544f, 0.55601192f, 0.55977732f, 0.56354141f, 0.5673039f,
    0.57106459f, 0.57482314f, 0.57857943f, 0.58233309f, 0.58608389f, 0.58983159f, 0.59357601f, 0.5973168f,
    0.60105371f, 0.60478657f, 0.60851508f, 0.61223894f, 0.61595803f, 0.619672f, 0.6233806f, 0.6270836f,
    0.63078076f, 0.63447183f, 0.63815659f, 0.64183474f, 0.64550602f, 0.64917028f, 0.6528272f, 0.6564765f,
    0.66011804f, 0.66375148f, 0.66737664f, 0.67099327f, 0.67460108f, 0.67819989f, 0.68178934f, 0.68536937f,
    0.68893963f, 0.69249988f, 0.69604987f, 0.69958943f, 0.70311821f, 0.70663613f, 0.71014279f, 0.71363813f,
    0.71712172f, 0.72059351f, 0.72405308f, 0.72750038f, 0.7309351f, 0.73435694f, 0.73776579f, 0.74116135f,
    0.74454343f, 0.74791175f, 0.75126612f, 0.75460637f, 0.75793219f, 0.76124334f, 0.76453966f, 0.76782095f,
    0.77108693f, 0.77433741f, 0.77757215f, 0.78079093f, 0.7839936f, 0.78717989f, 0.79034954f, 0.79350245f,
    0.79663831f, 0.799757f, 0.80285817f, 0.80594176f, 0.80900753f, 0.81205517f, 0.81508458f, 0.81809556f,
    0.8210879f, 0.82406133f, 0.8270157f, 0.82995081f, 0.83286649f, 0.8357625f, 0.8386386f, 0.84149474f,
    0.84433061f, 0.84714609f, 0.84994096f, 0.85271496f, 0.85546803f, 0.85819995f, 0.86091048f, 0.86359954f,
    0.86626679f, 0.86891216f, 0.87153548f, 0.87413657f, 0.87671524f, 0.87927133f, 0.88180459f, 0.88431495f,
    0.8868022f, 0.88926613f, 0.89170671f, 0.89412361f, 0.8965168f, 0.89888602f, 0.90123123f, 0.90355211f,
    0.90584862f, 0.90812063f, 0.91036791f, 0.91259027f, 0.91478771f, 0.91696f, 0.9191069f, 0.92122847f,
    0.92332441f, 0.92539459f, 0.92743897f, 0.92945731f, 0.93144953f, 0.93341547f, 0.93535507f, 0.93726808f,
    0.93915439f, 0.94101399f, 0.94284666f, 0.94465226f, 0.94643074f, 0.94818193f, 0.94990575f, 0.95160204f,
    0.95327073f, 0.95491165f, 0.95652479f, 0.95810992f, 0.95966703f, 0.96119595f, 0.96269661f, 0.96416891f,
    0.96561277f, 0.96702802f, 0.96841466f, 0.96977252f, 0.97110152f, 0.97240162f, 0.97367269f, 0.97491467f,
    0.97612745f, 0.97731102f, 0.97846514f, 0.97958994f, 0.98068517f, 0.98175085f, 0.98278689f, 0.9837932f,
    0.98476976f, 0.98571646f, 0.98663324f, 0.98752004f, 0.9883768f, 0.98920351f, 0.99000007f, 0.99076641f,
    0.99150252f, 0.9922083f, 0.99288374f, 0.99352884f, 0.99414343f, 0.99472761f, 0.99528122f, 0.99580431f,
    0.99629682f, 0.9967587f, 0.99718988f, 0.99759042f, 0.99796027f, 0.99829936f, 0.99860775f, 0.99888527f,
    0.9991321f, 0.99934804f, 0.99953318f, 0.99968749f, 0.99981093f, 0.99990356f, 0.99996525f, 0.99999613f};

static const float winHamming_1024[512] = {
    0.079999983f, 0.080008648f, 0.080034696f, 0.080078073f, 0.080138803f, 0.080216862f, 0.080312304f, 0.080425046f,
    0.080555148f, 0.080702573f, 0.080867328f, 0.081049412f, 0.081248797f, 0.081465513f, 0.081699498f, 0.081950784f,
    0.082219318f, 0.082505152f, 0.082808234f, 0.083128586f, 0.083466157f, 0.083820947f, 0.084192932f, 0.084582128f,
    0.084988497f, 0.085412025f, 0.085852712f, 0.086310543f, 0.08678548f, 0.087277494f, 0.0877866f, 0.088312782f,
    0.088855959f, 0.089416191f, 0.089993402f, 0.090587609f, 0.091198757f, 0.091826826f, 0.092471838f, 0.093133681f,
    0.093812421f, 0.094507962f, 0.095220342f, 0.095949471f, 0.096695356f, 0.09745799f, 0.098237291f, 0.099033274f,
    0.099845864f, 0.10067507f, 0.10152087f, 0.10238319f, 0.10326202f, 0.10415734f, 0.10506907f, 0.10599723f,
    0.10694176f, 0.10790262f, 0.10887978f, 0.10987322f, 0.11088287f, 0.11190872f, 0.11295072f, 0.11400881f,
    0.11508297f, 0.11617317f, 0.11727936f, 0.1184015f, 0.11953954f, 0.12069343f, 0.12186315f, 0.12304863f,
    0.12424984f, 0.12546673f, 0.1266993f, 0.12794742f, 0.1292111f, 0.13049026f, 0.13178489f, 0.13309491f,
    0.13442028f, 0.13576093f, 0.13711685f, 0.13848796f, 0.13987423f, 0.14127557f, 0.14269197f, 0.14412336f,
    0.14556967f, 0.14703086f, 0.14850688f, 0.14999767f, 0.15150316f, 0.15302332f, 0.15455806f, 0.15610735f,
    0.15767115f, 0.15924934f, 0.1608419f, 0.16244876f, 0.16406986f, 0.16570516f, 0.16735455f, 0.16901802f,
    0.17069548f, 0.17238687f, 0.17409211f, 0.17581119f, 0.17754398f, 0.17929046f, 0.18105054f, 0.18282416f,
    0.18461125f, 0.18641175f, 0.1882256f, 0.19005269f, 0.19189301f, 0.19374646f, 0.19561295f, 0.19749244f,
    0.19938485f, 0.20129012f, 0.20320815f, 0.20513891f, 0.20708229f, 0.20903821f, 0.21100664f, 0.21298747f,
    0.21498062f, 0.21698608f, 0.21900369f, 0.22103339f, 0.22307514f, 0.22512887f, 0.22719444f, 0.22927183f,
    0.23136096f, 0.23346171f, 0.23557404f, 0.23769781f, 0.23983303f, 0.24197954f, 0.24413732f, 0.24630626f,
    0.24848625f, 0.25067726f, 0.25287917f, 0.25509194f, 0.25731543f, 0.25954959f, 0.26179433f, 0.26404959f,
    0.26631522f, 0.2685912f, 0.27087739f, 0.27317375f, 0.27548021f, 0.2777966f, 0.28012288f, 0.28245899f,
    0.28480479f, 0.28716025f, 0.28952521f, 0.29189965f, 0.29428345f, 0.29667649f, 0.2990787f, 0.30149007f,
    0.30391037f, 0.30633959f, 0.30877763f, 0.3112244f, 0.31367978f, 0.31614372f, 0.31861609f, 0.32109684f,
    0.32358581f, 0.32608294f, 0.32858819f, 0.33110136f, 0.33362246f, 0.3361513f, 0.33868787f, 0.341232f,
    0.34378365f, 0.34634268f, 0.34890902f, 0.3514826f, 0.35406324f, 0.35665092f, 0.35924554f, 0.36184695f,
    0.3644551f, 0.36706984f, 0.36969113f, 0.37231883f, 0.37495288f, 0.37759313f, 0.38023952f, 0.38289192f,
    0.38555026f, 0.38821444f, 0.39088434f, 0.39355984f, 0.39624089f, 0.39892736f, 0.40161914f, 0.40431616f,
    0.40701827f, 0.4097254f, 0.41243747f, 0.41515434f, 0.41787592f, 0.42060208f, 0.42333278f, 0.42606786f,
    0.42880726f, 0.43155083f, 0.43429852f, 0.43705016f, 0.43980569f, 0.44256502f, 0.44532803f, 0.44809461f,
    0.45086464f, 0.45363802f, 0.45641467f, 0.45919448f, 0.46197736f, 0.46476316f, 0.4675518f, 0.47034317f,
    0.47313717f, 0.47593367f, 0.47873262f, 0.48153389f, 0.48433733f, 0.48714289f, 0.48995045f, 0.49275988f,
    0.49557111f, 0.498384f, 0.50119847f, 0.50401437f, 0.50683165f, 0.50965023f, 0.51246989f, 0.51529062f,
    0.51811224f, 0.52093476f, 0.52375793f, 0.52658176f, 0.52940607f, 0.53223079f, 0.53505582f, 0.53788102f,
    0.54070628f, 0.54353154f, 0.54635662f, 0.54918152f, 0.55200607f, 0.55483013f, 0.55765361f, 0.56047648f,
    0.56329858f, 0.56611979f, 0.56893998f, 0.5717591f, 0.57457703f, 0.57739365f, 0.58020884f, 0.58302253f,
    0.58583462f, 0.58864498f, 0.59145349f, 0.59426004f, 0.59706455f, 0.59986693f, 0.60266703f, 0.60546476f,
    0.60826004f, 0.61105275f, 0.61384279f, 0.61663002f, 0.61941433f, 0.62219572f, 0.62497395f, 0.62774897f,
    0.6305207f, 0.63328904f, 0.6360538f, 0.63881499f, 0.64157242f, 0.64432603f, 0.64707571f, 0.6498214f,
    0.65256286f, 0.65530014f, 0.65803301f, 0.66076148f, 0.66348535f, 0.66620457f, 0.66891909f, 0.67162865f,
    0.67433333f, 0.67703289f, 0.67972732f, 0.68241644f, 0.6851002f, 0.68777853f, 0.6904512f, 0.69311827f,
    0.6957795f, 0.69843489f, 0.70108432f, 0.70372766f, 0.70636481f, 0.7089957f, 0.71162021f, 0.71423823f,
    0.71684974f, 0.71945447f, 0.72205251f, 0.72464371f, 0.72722787f, 0.72980499f, 0.73237497f, 0.73493767f,
    0.73749304f, 0.7400409f, 0.74258131f, 0.74511397f, 0.74763894f, 0.7501561f, 0.75266534f, 0.75516653f,
    0.75765961f, 0.76014447f, 0.76262105f, 0.76508921f, 0.76754886f, 0.76999998f, 0.7724424f, 0.77487606f,
    0.77730083f, 0.77971667f, 0.78212345f, 0.7845211f, 0.78690952f, 0.78928864f, 0.7916584f, 0.79401857f,
    0.79636919f, 0.79871017f, 0.80104142f, 0.80336279f, 0.8056742f, 0.80797559f, 0.81026691f, 0.81254798f,
    0.8148188f, 0.81707925f, 0.81932926f, 0.82156873f, 0.82379758f, 0.82601571f, 0.82822305f, 0.83041954f,
    0.83260506f, 0.8347795f, 0.83694291f, 0.83909506f, 0.84123594f, 0.84336543f, 0.84548348f, 0.84759003f,
    0.84968495f, 0.85176826f, 0.85383976f, 0.85589939f, 0.85794717f, 0.85998291f, 0.86200655f, 0.86401808f,
    0.8660174f, 0.86800444f, 0.86997908f, 0.87194121f, 0.87389094f, 0.87582797f, 0.87775236f, 0.87966406f,
    0.88156289f, 0.88344884f, 0.88532186f, 0.88718182f, 0.88902873f, 0.89086241f, 0.89268291f, 0.89449012f,
    0.89628392f, 0.89806426f, 0.89983112f, 0.90158439f, 0.90332407f, 0.90504998f, 0.90676218f, 0.9084605f,
    0.91014493f, 0.9118154f, 0.91347182f, 0.91511416f, 0.91674238f, 0.91835636f, 0.91995609f, 0.92154151f,
    0.92311251f, 0.92466903f, 0.92621106f, 0.92773849f, 0.92925137f, 0.93074954f, 0.93223292f, 0.93370152f,
    0.93515527f, 0.93659419f, 0.93801802f, 0.9394269f, 0.94082075f, 0.94219947f, 0.94356292f, 0.94491124f,
    0.9462443f, 0.94756198f, 0.94886428f, 0.95015121f, 0.95142263f, 0.95267856f, 0.95391887f, 0.95514357f,
    0.95635265f, 0.95754606f, 0.95872366f, 0.95988548f, 0.96103144f, 0.96216154f, 0.96327567f, 0.96437389f,
    0.96545607f, 0.96652222f, 0.96757227f, 0.96860617f, 0.96962392f, 0.97062546f, 0.97161078f, 0.97257978f,
    0.9735325f, 0.97446883f, 0.97538877f, 0.97629231f, 0.97717941f, 0.97804999f, 0.97890407f, 0.97974157f,
    0.98056245f, 0.98136675f, 0.98215443f, 0.98292536f, 0.98367965f, 0.98441714f, 0.98513794f, 0.98584187f,
    0.98652899f, 0.98719931f, 0.98785275f, 0.98848927f, 0.98910886f, 0.98971158f, 0.99029732f, 0.99086601f,
    0.99141777f, 0.99195242f, 0.99247003f, 0.99297065f, 0.9934541f, 0.99392051f, 0.99436975f, 0.99480188f,
    0.99521679f, 0.99561459f, 0.99599522f, 0.99635857f, 0.99670476f, 0.99703372f, 0.99734545f, 0.99763989f,
    0.99791706f, 0.99817699f, 0.99841964f, 0.99864501f, 0.99885303f, 0.99904376f, 0.99921721f, 0.99937326f,
    0.99951202f, 0.99963349f, 0.99973756f, 0.99982429f, 0.99989372f, 0.99994576f, 0.99998045f, 0.99999785f};

static void findPeaksLowFreq(
    int lowPassBins,
    float *logPowerSpeechSpectrum,
    int *nrPeakIdx,
    int *peakIndices) {
  int i = 0;
  int maxPeakIdx = 0;

  for (i = 2; i < lowPassBins; i++) {
    if ((logPowerSpeechSpectrum[i - 1] > logPowerSpeechSpectrum[i - 2]) && (logPowerSpeechSpectrum[i - 1] > logPowerSpeechSpectrum[i])) {
      peakIndices[maxPeakIdx] = i - 1;
      maxPeakIdx++;
    }
  }

  *nrPeakIdx = maxPeakIdx;
}

static void calculateMedianPitchLag(
    float *pitchLags,
    const int n_div,
    int *medianLag,
    float *meanLag,
    int *nonACELPPitchLags) {
  int nonACELPLags = 0;
  int middleACELPPitchLag = 0;

  shellsortFLOAT(pitchLags, n_div);

  while (nonACELPLags < n_div && pitchLags[nonACELPLags] == 0.0f) {
    nonACELPLags++;
  }

  middleACELPPitchLag = (n_div - nonACELPLags) / 2 + nonACELPLags;

  if (nonACELPLags >= n_div - 1) {
    *meanLag = 0.0f;
  } else {
    *meanLag = (pitchLags[middleACELPPitchLag] + pitchLags[middleACELPPitchLag + 1]) / 2.0f;
  }
  *medianLag = (int)(*meanLag + 0.5f);
  *nonACELPPitchLags = nonACELPLags;
}

static void calculateMedianPitchGain(
    float *pitchGains,
    int n_div,
    float *interharmonicAttenuation,
    int nonACELPPitchLags) {
  int nonACELPPitchGains = 0;
  int middleACELPPitchGain = 0;
  float meanGain = 0.0f;

  *interharmonicAttenuation = 0.0f;

  shellsortFLOAT(pitchGains, n_div);

  while (nonACELPPitchGains < n_div && pitchGains[nonACELPPitchGains] == 0.0f) {
    nonACELPPitchGains++;
  }

  middleACELPPitchGain = (n_div - nonACELPPitchGains) / 2 + nonACELPPitchGains;

  (void)(nonACELPPitchLags);

  if (nonACELPPitchGains >= n_div - 1) {
    meanGain = 0;
  } else {
    meanGain = (pitchGains[middleACELPPitchGain] + pitchGains[middleACELPPitchGain + 1]) / 2.0f;
  }
  *interharmonicAttenuation = min(meanGain * 0.5f, 0.5f);
}

static void createBassPostFilter(
    int lFrame,
    float *bassPostFilter,
    int medianLag) {
  int lowerFilterIdx = lFrame - 2 * medianLag;
  int upperFilterIdx = lFrame + 2 * medianLag;

  setFLOAT(0.0f, bassPostFilter, 2 * lFrame);

  if (lowerFilterIdx >= 0) {
    bassPostFilter[lowerFilterIdx] = -0.5f;
  }
  bassPostFilter[lFrame] = 1.0f;
  if (upperFilterIdx <= 2 * L_FRAME_1024) {
    bassPostFilter[upperFilterIdx] = -0.5f;
  }

  iis_fftf(bassPostFilter, lFrame);
}

static void updatePitchLagsAndGains(
    int mod[],
    int n_subfr,
    float *pitchGains,
    float *pitchLags) {
  int i = 0;
  int j = 0;

  for (i = 0; i < NB_DIV; i++) {
    if (mod[i] != 0) {
      for (j = 0; j < n_subfr; j++) {
        pitchGains[i * n_subfr + j] = 0;
        pitchLags[i * n_subfr + j] = 0;
      }
      if (i > 0 && mod[i - 1] == 0) {
        pitchGains[i * n_subfr + 1] = pitchGains[i * n_subfr] = pitchGains[i * n_subfr - 1];
        pitchLags[i * n_subfr + 1] = pitchLags[i * n_subfr] = pitchLags[i * n_subfr - 1];
      }
      if (i < NB_DIV - 1 && mod[i + 1] == 0) {
        pitchGains[i * n_subfr + n_subfr - 2] = pitchGains[i * n_subfr + n_subfr - 1] = pitchGains[i * n_subfr + n_subfr];
        pitchLags[i * n_subfr + n_subfr - 2] = pitchLags[i * n_subfr + n_subfr - 1] = pitchLags[i * n_subfr + n_subfr];
      }
      if (n_subfr == 3 && i > 0 && mod[i - 1] == 0 && i < NB_DIV - 1 && mod[i + 1] == 0) {
        pitchGains[i * n_subfr + 1] = (pitchGains[i * n_subfr - 1] + pitchGains[i * n_subfr + n_subfr]) / 2.0f;
        pitchLags[i * n_subfr + 1] = (pitchLags[i * n_subfr - 1] + pitchLags[i * n_subfr + n_subfr]) / 2.0f;
      }
    }
  }
}

static void calculateSpectralPeak(
    int lFrame,
    const float *winHamming,
    float *speech,
    float *spectralPeak,
    float *powerSpeechSpectrum) {
  int i = 0;
  float fftSpeech[2 * L_FRAME_1024] = {0};

  for (i = 0; i < (lFrame / 2); i++) {
    fftSpeech[2 * i] = winHamming[i] * speech[i];
    fftSpeech[2 * (i + lFrame / 2)] = winHamming[(lFrame / 2) - i - 1] * speech[(lFrame / 2) + i];
    fftSpeech[2 * i + 1] = 0;
    fftSpeech[2 * (i + lFrame / 2) + 1] = 0;
  }

  iis_fftf(fftSpeech, lFrame);

  for (i = 0; i < lFrame / 2; i++) {
    powerSpeechSpectrum[i] = (float)sqrt(fftSpeech[2 * i] * fftSpeech[2 * i] + fftSpeech[2 * i + 1] * fftSpeech[2 * i + 1]);
    if (powerSpeechSpectrum[i] > *spectralPeak) {
      *spectralPeak = powerSpeechSpectrum[i];
    }
  }
}

int LPDEnc_bpf_Decide(
    int fscale,
    int lFrame,
    float *speech,
    float *pitchLags,
    float *pitchGains,
    int *mod) {
  int i = 0;
  int bpfActive = 1;
  int medianLag = 0;
  int nonACELPPitchLags = 0;
  int nrPeakIdx = 0;
  int maxPeakIdx = 0;
  int pitchIdx = 0;
  float diffPeaks = 0.0f;
  float interharmonicAttenuation = 0.0f;
  float loThrSpectralPeak = 0.0f;
  float upThrSpectralPeak = 0.0f;
  float meanLag = 0.0f;
  float spectralPeak = 0.0f;
  float eps = 1.0e-6f;
  float filteredSpeech = 0.0f;
  int peakIndices[L_FRAME_1024 / 2] = {0};
  float bassPostFilter[2 * L_FRAME_1024] = {0.0f};
  float powerSpeechSpectrum[L_FRAME_1024 / 2] = {0.0f};
  float logPowerSpeechSpectrum[L_FRAME_1024 / 2] = {0.0f};
  float logFilteredSpeech[L_FRAME_1024 / 2] = {0.0f};
  const float *winHamming = NULL;
  int n_div = lFrame / 64;
  int n_subfr = n_div / NB_DIV;
  int lowPassBins = (int)((700.0f / (fscale)) * lFrame + 0.5f);

  switch (lFrame) {
    case 768:
      winHamming = winHamming_768;
      break;
    case 1024:
      winHamming = winHamming_1024;
      break;
    default:
      winHamming = winHamming_1024;
      break;
  }

  updatePitchLagsAndGains(mod, n_subfr, pitchGains, pitchLags);

  for (i = 1; i < n_div; i++) {
    if (pitchGains[i - 1] != 0.0f && pitchGains[i] != 0.0f && fabs(pitchLags[i - 1] - pitchLags[i]) >= LAG_DIFF_MAX) {
      return bpfActive;
    }
  }

  if (n_subfr < 4) {
    for (i = n_subfr * NB_DIV; i < NB_DIV * NB_SUBFR_1024; i++) {
      pitchGains[i] = 0;
      pitchLags[i] = 0;
    }
  }

  calculateMedianPitchLag(pitchLags, n_div, &medianLag, &meanLag, &nonACELPPitchLags);

  pitchIdx = (int)(lFrame / meanLag);

  calculateMedianPitchGain(pitchGains, n_div, &interharmonicAttenuation, nonACELPPitchLags);

  calculateSpectralPeak(lFrame, winHamming, speech, &spectralPeak, powerSpeechSpectrum);

  createBassPostFilter(lFrame, bassPostFilter, medianLag);

  for (i = 0; i < lowPassBins; i++) {
    filteredSpeech = interharmonicAttenuation * powerSpeechSpectrum[i] * (float)fabs(bassPostFilter[2 * i]);
    logPowerSpeechSpectrum[i] = 20 * (float)log10(max(eps, powerSpeechSpectrum[i]));
    logFilteredSpeech[i] = 20 * (float)log10(max(eps, filteredSpeech));
  }

  findPeaksLowFreq(lowPassBins, logPowerSpeechSpectrum, &nrPeakIdx, peakIndices);

  loThrSpectralPeak = 20 * (float)log10(max(eps, spectralPeak)) - 27.0f;
  upThrSpectralPeak = 20 * (float)log10(max(eps, spectralPeak)) - 17.0f;

  for (i = 0; i < nrPeakIdx; i++) {
    maxPeakIdx = peakIndices[i];
    diffPeaks = logPowerSpeechSpectrum[maxPeakIdx] - logFilteredSpeech[maxPeakIdx];

    if (maxPeakIdx < pitchIdx) {
      if (logFilteredSpeech[maxPeakIdx] > loThrSpectralPeak && diffPeaks < PEAK_DIFF_THR) {
        bpfActive = 0;
      }
    } else {
      if (logFilteredSpeech[maxPeakIdx] > upThrSpectralPeak && diffPeaks < PEAK_DIFF_THR) {
        bpfActive = 0;
      }
    }
  }
  return bpfActive;
}
