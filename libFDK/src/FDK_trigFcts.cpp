/***************************  Fraunhofer IIS FDK Tools  **********************

                        (C) Copyright Fraunhofer IIS (2005)
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
   Author(s):   Haricharan Lakshman, Manuel Jander
   Description: Trigonometric functions fixed point fractional implementation.

   This software and/or program is protected by copyright law and international
   treaties. Any reproduction or distribution of this software and/or program,
   or any portion of it, may result in severe civil and criminal penalties, and
   will be prosecuted to the maximum extent possible under law.

******************************************************************************/

#include "FDK_trigFcts.h"

#include "fixpoint_math.h"


// #define CORDIC_SINCOS


#define IMPROVE_ATAN2_ACCURACY  1  // 0 --> 59 dB SNR     1 --> 65 dB SNR
#define MINSFTAB  7
#define MAXSFTAB 25

#if IMPROVE_ATAN2_ACCURACY
static const FIXP_DBL f_atan_expand_range[MAXSFTAB-(MINSFTAB-1)]  =
{
  /*****************************************************************************
   *
   *  Table holds fixp_atan() output values which are outside of input range
   *  of fixp_atan() to improve SNR of fixp_atan2().
   *
   *  This Table might also be used in fixp_atan() [todo] so there a wider input
   *  range can be covered, too.
   *
   *  Matlab (generate table):
   *    for scl = 7:25            % MINSFTAB .. MAXSFTAB
   *      at=atan(0.5 *(2^scl));  % 0.5 because get in 'middle' area of current scale level 'scl'
   *      at/2                    % div at by ATO_SCALE
   *    end
   *
   *  Table divided by 2=ATO_SCALE  <--  SF=ATO_SF
   *****************************************************************************/
   FL2FXCONST_DBL(7.775862990872099e-001), FL2FXCONST_DBL(7.814919928673978e-001), FL2FXCONST_DBL(7.834450483314648e-001),
   FL2FXCONST_DBL(7.844216021392089e-001), FL2FXCONST_DBL(7.849098823026687e-001), FL2FXCONST_DBL(7.851540227918509e-001),
   FL2FXCONST_DBL(7.852760930873737e-001), FL2FXCONST_DBL(7.853371282415015e-001), FL2FXCONST_DBL(7.853676458193612e-001),
   FL2FXCONST_DBL(7.853829046083906e-001), FL2FXCONST_DBL(7.853905340029177e-001), FL2FXCONST_DBL(7.853943487001828e-001),
   FL2FXCONST_DBL(7.853962560488155e-001), FL2FXCONST_DBL(7.853972097231319e-001), FL2FXCONST_DBL(7.853976865602901e-001),
   FL2FXCONST_DBL(7.853979249788692e-001), FL2FXCONST_DBL(7.853980441881587e-001), FL2FXCONST_DBL(7.853981037928035e-001),
   FL2FXCONST_DBL(7.853981335951259e-001)
   //     pi/4 = 0.785398163397448 = pi/2/ATO_SCALE
};
#endif

FIXP_DBL fixp_atan2(FIXP_DBL y, FIXP_DBL x)
{
    FIXP_DBL q;
    FIXP_DBL at;  // atan  out
    FIXP_DBL at2; // atan2 out
    FIXP_DBL ret = FL2FXCONST_DBL(-1.0f);
    INT sf,sfo,stf;

    // --- division

    if      (y > FL2FXCONST_DBL(0.0f))
    {
        if      (x > FL2FXCONST_DBL(0.0f)) {
                                           q =  fDivNormHighPrec( y, x, &sf); // both pos.
        }
        else if (x < FL2FXCONST_DBL(0.0f)) {
                                           q = -fDivNormHighPrec( y,-x, &sf); // x neg.
        }
        else {//(x ==FL2FXCONST_DBL(0.0f))
                                           q =  FL2FXCONST_DBL(+1.0f);  // y/x = pos/zero = +Inf
                                           sf = 0;
        }
    }
    else if (y < FL2FXCONST_DBL(0.0f))
    {
        if      (x > FL2FXCONST_DBL(0.0f)) {
                                           q = -fDivNormHighPrec(-y, x, &sf); // y neg.
        }
        else if (x < FL2FXCONST_DBL(0.0f)) {
                                           q =  fDivNormHighPrec(-y,-x, &sf); // both neg.
        }
        else {//(x ==FL2FXCONST_DBL(0.0f))
                                           q =  FL2FXCONST_DBL(-1.0f);  // y/x = neg/zero = -Inf
                                           sf = 0;
        }
    }
    else { // (y ==FL2FXCONST_DBL(0.0f))
        q = FL2FXCONST_DBL(0.0f);
        sf = 0;
    }
    sfo = sf;
    //write2file(fpF__h,(float)y/(float)x);
    //write2file(fpC__h,(float) q * (float)FDKpow(2,sfo));  // SNR 86 dB  But range not limited to [-64..64] which is the input range of fixp_atan()

    // --- atan()

    if  ( sfo > ATI_SF ) {
        // --- could not calc fixp_atan() here bec of input data out of range
        //     ==> therefore give back boundary values

        #if IMPROVE_ATAN2_ACCURACY
        if (sfo > MAXSFTAB) sfo = MAXSFTAB;
        #endif

        if      (  q > FL2FXCONST_DBL(0.0f) ) {
           #if IMPROVE_ATAN2_ACCURACY
            at = +f_atan_expand_range[sfo-ATI_SF-1];
           #else
            at = FL2FXCONST_DBL( +M_PI/2 / ATO_SCALE);
           #endif
        }
        else if (  q < FL2FXCONST_DBL(0.0f) ) {
           #if IMPROVE_ATAN2_ACCURACY
            at = -f_atan_expand_range[sfo-ATI_SF-1];
           #else
            at = FL2FXCONST_DBL( -M_PI/2 / ATO_SCALE);
           #endif
        }
        else {  // q== FL2FXCONST_DBL(0.0f)
            at = FL2FXCONST_DBL( 0.0f );
        }
    }else{
        // --- calc of fixp_atan() is possible; input data within range
        //     ==> set q on fixed scale level as desired from fixp_atan()
        stf = sfo - ATI_SF;
        if (stf > 0)  q = q << (INT)fMin( stf,DFRACT_BITS-1);
        else          q = q >> (INT)fMin(-stf,DFRACT_BITS-1);
        //write2file(fpF__e,(float)y/(float)x);
        //write2file(fpC__e,(float)q * (float)FDKpow(2,ATI_SF)); // 88.9 dB
        at = fixp_atan(q);  // ATO_SF
    }
    //write2file(fpF__g,(float)FDKatan( (float)y/(float)x ));
    //write2file(fpC__g,(float)   at * (float)FDKpow(2,ATO_SF) ); // todo dB

    // --- atan2()

    at2 = at >> (AT2O_SF - ATO_SF); // now AT2O_SF for atan2
    if      (  x > FL2FXCONST_DBL(0.0f) ) {
        ret = at2;
    }
    else if (  x < FL2FXCONST_DBL(0.0f) ) {
        if (  y >= FL2FXCONST_DBL(0.0f) ) {
            ret = at2 + FL2FXCONST_DBL( M_PI / AT2O_SCALE);
        } else {
            ret = at2 - FL2FXCONST_DBL( M_PI / AT2O_SCALE);
        }
    }
    else {
        // x == 0
        if      ( y >  FL2FXCONST_DBL(0.0f) ) {
            ret = FL2FXCONST_DBL( +M_PI/2 / AT2O_SCALE);
        }
        else if ( y <  FL2FXCONST_DBL(0.0f) ) {
            ret = FL2FXCONST_DBL( -M_PI/2 / AT2O_SCALE);
        }
        else if ( y == FL2FXCONST_DBL(0.0f) ) {
            ret = FL2FXCONST_DBL(0.0f);
        }
    }
    //write2file(fpC__f,(float)ret * (float)FDKpow(2,AT2O_SF));  // 65 dB
    //write2file(fpF__f,(float)FDKatan2(y,x));
    return ret;
}


FIXP_DBL fixp_atan(FIXP_DBL x)
{
    INT sign;
    FIXP_DBL result, temp;

    // SNR of fixp_atan() = 56 dB
    FIXP_DBL ONEBY3P56  = (FIXP_DBL)0x26800000; // 1.0/3.56 in q31
    FIXP_DBL P281       = (FIXP_DBL)0x00013000; // 0.281 in q18
    FIXP_DBL ONEP571    = (FIXP_DBL)0x6487ef00; // 1.571 in q30

    if (x < FIXP_DBL(0)) {
      sign = 1;
      x = - x ;
    } else {
      sign = 0;
    }

    /* calc of arctan */
    if(x < ( Q(Q_ATANINP)-FL2FXCONST_DBL(0.00395)) )
    {
        INT res_e;

        temp = fPow2(x);            // q25 * q25 - (DFRACT_BITS-1) = q19
        temp = fMult(temp, ONEBY3P56);      // q19 * q31 - (DFRACT_BITS-1) = q19
        temp = temp + Q(19);                // q19 + q19 = q19
        result = fDivNorm(x, temp, &res_e);
        result = scaleValue(result, (Q_ATANOUT-Q_ATANINP+19-DFRACT_BITS+1) + res_e  );
    }
    else if( x < FL2FXCONST_DBL(1.28/64.0) )
    {
        FIXP_DBL delta_fix;
        FIXP_DBL PI_BY_4 = FL2FXCONST_DBL(3.1415926/4.0) >> 1; /* pi/4 in q30 */

        delta_fix = (x - FL2FXCONST_DBL(1.0/64.0)) << 5; /* q30 */
        result = PI_BY_4 + (delta_fix >> 1) - (fPow2Div2(delta_fix));
    }
    else
    {
        INT res_e;

        temp = fPow2Div2(x);        // q25 * q25 - (DFRACT_BITS-1) - 1 = q18
        temp = temp + P281;                 // q18 + q18 = q18
        result = fDivNorm(x, temp, &res_e);
        result = scaleValue(result, (Q_ATANOUT-Q_ATANINP+18-DFRACT_BITS+1) + res_e );
        result = ONEP571 - result;          // q30 + q30 = q30
    }
    if (sign) {
      result = -result;
    }

    return(result);
}


#ifndef CORDIC_SINCOS

#include "FDK_tools_rom.h"

FIXP_DBL fixp_cos(FIXP_DBL x, int scale)
{
    FIXP_DBL residual, error, sine, cosine;
    
    residual = fixp_sin_cos_residual_inline(x, scale, &sine, &cosine);
    error = fMult(sine, residual);

    return cosine - error;
}

FIXP_DBL fixp_sin(FIXP_DBL x, int scale)
{
    FIXP_DBL residual, error, sine, cosine;
    
    residual = fixp_sin_cos_residual_inline(x, scale, &sine, &cosine);
    error = fMult(cosine, residual);

    return sine + error;
}

void fixp_cos_sin (FIXP_DBL x, int scale, FIXP_DBL *cos, FIXP_DBL *sin)
{
    FIXP_DBL residual, error0, error1, sine, cosine;
    
    residual = fixp_sin_cos_residual_inline(x, scale, &sine, &cosine);
    error0 = fMult(sine, residual);
    error1 = fMult(cosine, residual);
    *cos  = cosine - error0;
    *sin  = sine + error1;
}

#else /* #ifndef CORDIC_SINCOS */

/*
  // Matlab
  function v = cordic(beta,n)
  % This function computes v = [cos(beta), sin(beta)] (beta in radians)
  % using n iterations. Increasing n will increase the precision.

  if beta < -pi/2 | beta > pi/2
      if beta < 0
          v = cordic(beta + pi, n);
      else
          v = cordic(beta - pi, n);
      end
      v = -v; % flip the sign for second or third quadrant
      return
  end

  % Initialization of tables of constants used by CORDIC
  % need a table of arctangents of negative powers of two, in radians:
  % angles = atan(2.^-(0:27));
  angles =  [  ...
      0.78539816339745   0.46364760900081   0.24497866312686   0.12435499454676 ...
      0.06241880999596   0.03123983343027   0.01562372862048   0.00781234106010 ...
      0.00390623013197   0.00195312251648   0.00097656218956   0.00048828121119 ...
      0.00024414062015   0.00012207031189   0.00006103515617   0.00003051757812 ...
      0.00001525878906   0.00000762939453   0.00000381469727   0.00000190734863 ...
      0.00000095367432   0.00000047683716   0.00000023841858   0.00000011920929 ...
      0.00000005960464   0.00000002980232   0.00000001490116   0.00000000745058 ];
  % and a table of products of reciprocal lengths of vectors [1, 2^-j]:
  Kvalues = [ ...
      0.70710678118655   0.63245553203368   0.61357199107790   0.60883391251775 ...
      0.60764825625617   0.60735177014130   0.60727764409353   0.60725911229889 ...
      0.60725447933256   0.60725332108988   0.60725303152913   0.60725295913894 ...
      0.60725294104140   0.60725293651701   0.60725293538591   0.60725293510314 ...
      0.60725293503245   0.60725293501477   0.60725293501035   0.60725293500925 ...
      0.60725293500897   0.60725293500890   0.60725293500889   0.60725293500888 ];
  Kn = Kvalues(min(n, length(Kvalues)));

  % Initialize loop variables:
  v = [1;0]; % start with 2-vector cosine and sine of zero
  poweroftwo = 1;
  angle = angles(1);

  % Iterations
  for j = 0:n-1;
      if beta < 0
          sigma = -1;
      else
          sigma = 1;
      end
      factor = sigma * poweroftwo;
      R = [1, -factor; factor, 1];
      v = R * v; % 2-by-2 matrix multiply
      beta = beta - sigma * angle; % update the remaining angle
      poweroftwo = poweroftwo / 2;
      % update the angle from table, or eventually by just dividing by two
      if j+2 > length(angles)
          angle = angle / 2;
      else
          angle = angles(j+2);
      end
  end

  % Adjust length of output vector to be [cos(beta), sin(beta)]:
  v = v * Kn;
  return

  // C++
  #define C1
  #define C2
  #define TYPE double

  TYPE fixp_cordic(TYPE in, 
                   int n, 
                   int type)
  {
    int i;
    TYPE c,s,a;
    TYPE x,y,v;
    TYPE b;

    c = 0.5;
    s = 0.0;
    b = 1.0;

    for (i=0; i<n; i++) { 
      a = angleValues[i];
      x =  b * s;
      y =  b * c;

      if (in < 0.0) {
        c = c + x; 
        s = s - y; 
        in = in + a;
      }                        
      else {
        c = c - x; 
        s = s + y; 
        in = in - a; 
      }           
      b = b / 2;
    }

    if (type == 0)
      v = c * kValues[n-1]*2;        
    else
      v = s * kValues[n-1]*2;        

    return (v);
  }

  TYPE fixp_cos(TYPE in, 
                int n,
                int scale)
  {
    TYPE v;
    INT sign = 0;

    while ( (in < -M_PI/2) || (in > M_PI/2) ) {
      if (in < 0)
        in = in + M_PI;
      else
        in = in - M_PI;

      if (sign == 0)
        sign = 1;
      else
        sign = 0;
    }

    v = fixp_cordic(in,n,0);

    if (sign)
      v = -v;

    return (v);
  }

  TYPE fixp_sin(TYPE in, 
                int n,
                int scale)
  {
    TYPE v;
    INT sign = 0;

    while ( (in < -M_PI/2) || (in > M_PI/2) ) {
      if (in < 0)
        in = in + M_PI;
      else
        in = in - M_PI;

      if (sign == 0)
        sign = 1;
      else
        sign = 0;
    }

    v = fixp_cordic(in,n,1);

    if (sign)
      v = -v;

    return (v);
  }
*/


#define SF_C1   1
#define C1(x)   FL2FXCONST_DBL(x/(1<<SF_C1))
#define C2(x)   FL2FXCONST_DBL(x)
#define M_PI_4  FL2FXCONST_DBL(M_PI/4)
#define ITER    (DFRACT_BITS-1)

/*
  for (i=0; i<DFRACT_BITS; i++) {
    angleValues[i] = FDKatan(FDKpow(2.0,-i));
  }
*/
const FIXP_DBL angleValues[DFRACT_BITS] = {
  C1(7.853981633974483e-001), C1(4.636476090008061e-001), C1(2.449786631268641e-001), C1(1.243549945467614e-001), 
  C1(6.241880999595735e-002), C1(3.123983343026828e-002), C1(1.562372862047683e-002), C1(7.812341060101111e-003), 
  C1(3.906230131966972e-003), C1(1.953122516478819e-003), C1(9.765621895593195e-004), C1(4.882812111948983e-004), 
  C1(2.441406201493618e-004), C1(1.220703118936702e-004), C1(6.103515617420877e-005), C1(3.051757811552610e-005), 
  C1(1.525878906131576e-005), C1(7.629394531101970e-006), C1(3.814697265606496e-006), C1(1.907348632810187e-006), 
  C1(9.536743164059608e-007), C1(4.768371582030888e-007), C1(2.384185791015580e-007), C1(1.192092895507807e-007), 
  C1(5.960464477539055e-008), C1(2.980232238769530e-008), C1(1.490116119384766e-008), C1(7.450580596923828e-009), 
  C1(3.725290298461914e-009), C1(1.862645149230957e-009), C1(9.313225746154785e-010), C1(4.656612873077393e-010)
};

/*
  for (i=0; i<DFRACT_BITS; i++) {
    kValues[i] = 1.0;
    for (j=0; j<=i; j++) {
      val = FDKcos(FDKatan(FDKpow(2.0,-j)));
      kValues[i] *= val;
    }
  }
*/
const FIXP_DBL kValues[DFRACT_BITS] = {
  C2(7.071067811865476e-001), C2(6.324555320336759e-001), C2(6.135719910778964e-001), C2(6.088339125177524e-001), 
  C2(6.076482562561683e-001), C2(6.073517701412960e-001), C2(6.072776440935261e-001), C2(6.072591122988928e-001), 
  C2(6.072544793325625e-001), C2(6.072533210898753e-001), C2(6.072530315291345e-001), C2(6.072529591389450e-001), 
  C2(6.072529410413973e-001), C2(6.072529365170104e-001), C2(6.072529353859136e-001), C2(6.072529351031395e-001), 
  C2(6.072529350324459e-001), C2(6.072529350147725e-001), C2(6.072529350103542e-001), C2(6.072529350092496e-001), 
  C2(6.072529350089735e-001), C2(6.072529350089044e-001), C2(6.072529350088872e-001), C2(6.072529350088829e-001), 
  C2(6.072529350088818e-001), C2(6.072529350088816e-001), C2(6.072529350088814e-001), C2(6.072529350088814e-001), 
  C2(6.072529350088814e-001), C2(6.072529350088814e-001), C2(6.072529350088814e-001), C2(6.072529350088814e-001)
};

inline
static FIXP_DBL fixp_cordic(FIXP_DBL in, 
                            INT type)
{
  int i;
  FIXP_DBL c,s,a;
  FIXP_DBL x,y;
  int n = ITER;

  in = fMult(in,M_PI_4);
  c  = kValues[n-1] >> 1;
  s  = FL2FXCONST_DBL(0.0f);

  for (i=0; i<n; i++) { 
    a = angleValues[i];
    x = s >> i;
    y = c >> i;

    if (in < FL2FXCONST_DBL(0.0)) {
      c = c + x; 
      s = s - y; 
      in = in + a;
    }                        
    else {
      c = c - x; 
      s = s + y; 
      in = in - a; 
    }           
  }

  /* 1 headroom bit */
  if (type == 0)
    return (c);  
  else
    return (s);  
}

/*
   in:    argument
   scale: scalefactor of argument has to be in the range of 0, ... ,DFRACT_BITS-1
   type: calculate cosine 0
         calculate sine   1 
*/
static FIXP_DBL fixp_sincos(FIXP_DBL in, 
                            INT scale,
                            INT type)
{
  FIXP_DBL v;
  INT sign = 0;
  FIXP_DBL pi,pi_2;

  if (scale < 2) {
    in = in >> (2-scale);
    scale = 2;
  }

  pi   = M_PI_4 >> (scale-2);
  pi_2 = M_PI_4 >> (scale-1);
   
  /* move signal into the range of -pi/2 to +pi/2 */
  while ( (in < (-pi_2)) || (in > (pi_2)) ) {
    if (in < FL2FXCONST_DBL(0.0))
      in = in + pi;
    else
      in = in - pi;

    sign = ~sign;
  }

  /* scale signal with 2/pi => unscaled signal is in the range of [-1.0,...,+1.0[ */
  in = fMult(in,FL2FXCONST_DBL(2/M_PI)) << scale;

  v = fixp_cordic(in,type);

  if (sign)
    v = -v;

  /* compensate 1 headroom bit */
  if ( v <= FL2FXCONST_DBL(-0.5f) )
    v = (FIXP_DBL) (MINVAL_DBL+1); 
  else 
    v = SATURATE_LEFT_SHIFT(v,1,DFRACT_BITS);

  return (v);
}

/*
   in:    argument
   scale: scalefactor of argument has to be in the range of 0, ... ,DFRACT_BITS-1
*/
FIXP_DBL fixp_cos(FIXP_DBL in, 
                  INT scale)
                   
{
  FDK_ASSERT ((scale >= 0) && (scale < DFRACT_BITS));

  if (in == FL2FXCONST_DBL(0.0f))
    return /*FL2FXCONST_DBL(1.0)*/ (FIXP_DBL)MAXVAL_DBL;

  return (fixp_sincos(in,scale,0));
}

/*
   in:    argument
   scale: scalefactor of argument has to be in the range of 0, ... ,DFRACT_BITS-1
*/
FIXP_DBL fixp_sin(FIXP_DBL in, 
                  INT scale)
                   
{
  FDK_ASSERT ((scale >= 0) && (scale < DFRACT_BITS));

  if (in == FL2FXCONST_DBL(0.0f))
    return FL2FXCONST_DBL(0.0f);

  return (fixp_sincos(in,scale,1));
}

#endif /* #ifndef CORDIC_SINCOS */




