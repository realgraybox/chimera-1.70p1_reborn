/* pbm.h:
 *
 * PBM header file
 *
 * jim frost 10.15.89
 */

typedef struct {
  unsigned char width[2];
  unsigned char height[2];
} PBMCompact;

/* scale sample a of 0..b to 0..c; samples beyond b (bad files) give c,
 * and the product is done in long so it can't overflow (found by fuzzing) */
#define PM_SCALE(a, b, c) ((a) >= (b) ? (long)(c) : (long)(a) * (c) / (b))
