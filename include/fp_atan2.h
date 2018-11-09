#ifndef __FP_ATAN2_H
#define __FP_ATAN2_H

#include <stdint.h>
#include <stdio.h>


/// \addtogroup atan
/// @{


/* Fast atan2:
 * To calculate 4 quadrant arctangent one is computing:
 *  theta = atan(y/x), because of (amp) normalization the values on MCU/DSPs can 
 *                     result outside of defined ratious [-1, ), so we need to 
 *                     normalize it ourselves. Trick is:
 *  
 * (simple phase calc with 1st degree polynomial)
 * Quad 1, normalization: r = (x-y) / (y+x), phase: angle1 = pi/4 - pi/4*r
 * Quad 2, normalization: r = (x+y) / (y-x), phase: angle2 = 3*pi/4 - pi/4*r
 * Quad 3, check if not in Quad 1 but in Quad3 and invert angle1
 * Quad 4, check if not in Quad 4 but in Quad4 and invert angle2
 * 
 * To get better precision higher degree polynomial is used (3rd in our case).
 * Because it is the odd function, even degree coefficients disapper (are not used), 
 * so the equation is:
 *  angle1 = a*r^3 + b*r + c
 *  angle2 = a*r^3 + b*r + 3*c
 * where goot values for approximation arE:
 *  a =  0.1963
 *  b = -0.9816
 *  c =  pi/4
 *
 * Error on all range should be less than 0.01 [rad].
 *
 * Return value is in fp_s11_20 format!
 */

#define fp_fract 9
#define coeff_1 101
#define coeff_2 503
#define coeff_3 402

//#define fp_fract 13
//#define coeff_1  1608
//#define coeff_2  8041
//#define coeff_3  6034

//#define fp_fract 12
//#define coeff_1  804
//#define coeff_2  4021
//#define coeff_3  3217

//static const int32_t coeff_1 = 101;  /* coeff_1 = 0.1963; */
//static const int32_t coeff_2 = 503;  /* coeff_2 = 0.9816; */
//static const int32_t coeff_3 = 402;  /* coeff_3 = pi/4; */

/*static*/ inline int32_t __fp_mul(int32_t a, int32_t b)
{
    int32_t r = (int32_t)((int32_t)((int32_t)a * b)>>fp_fract);
    return r;
}

/*static*/ inline int32_t __fp_div(int32_t a, int32_t b)
{
    int32_t r = (int32_t)((((int32_t)a<<fp_fract))/b);

    return r;
}

/*static*/ inline int32_t fp_atan2(int16_t y, int16_t x)
{
    int32_t fp_y = ((int32_t)y<<fp_fract);
    int32_t fp_x = ((int32_t)x<<fp_fract);

    int32_t mask = (fp_y >> 31);

    int32_t fp_abs_y = (mask + fp_y) ^ mask;

    mask = (fp_x >> 31);

    int32_t fp_abs_x = (mask + fp_x) ^ mask;

    int32_t r, angle;

    if((fp_abs_y < 100) && (fp_abs_x < 100))
        return 0;


    if (x>=0) { /* quad 1 or 2 */
        /* r = (x - abs_y) / (x + abs_y)  */
        r = __fp_div((fp_x - fp_abs_y), (fp_x + fp_abs_y));

        /* angle = coeff_1 * r^3 - coeff_2 * r + coeff_3 */
        angle =
            __fp_mul(coeff_1, __fp_mul(r, __fp_mul(r, r))) -
            __fp_mul(coeff_2, r) + coeff_3;
    } else {
        /* r = (x + abs_y) / (abs_y - x); */
        r = __fp_div((fp_x + fp_abs_y), (fp_abs_y - fp_x));
        /*        angle = coeff_1 * r*r*r - coeff_2 * r + 3*coeff_3; */
        angle =
            __fp_mul(coeff_1, __fp_mul(r, __fp_mul(r, r))) -
            __fp_mul(coeff_2, r) + 3 * coeff_3;
    }

    if (y < 0)
        return(-angle);     // negate if in quad 3 or 4
    else
        return(angle);
}


/// @}

#endif // __FP_ATAN2_H
