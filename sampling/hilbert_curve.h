/* Non-recursive hilbert curve traversal.
 * Code taken (nearly) verbatim from "Hacker's Delight" by Henry S. Warren
 */

#include "numtype.h"

void hil_inc_xy(uint *xp, uint *yp, int n) {

   int i;
   uint x, y, state, dx, dy, row, dochange;

   x = *xp;
   y = *yp;
   state = 0;                   // Initialize.
   dx = -((1 << n) - 1);        // Init. -(2**n - 1).
   dy = 0;

   for (i = n-1; i >= 0; i--) {         // Do n times.
      row = 4*state | (2*((x >> i) & 1) | ((y >> i) & 1));
      dochange = (0xBDDB >> row) & 1;
      if (dochange) {
         dx = ((0x16451659 >> 2*row) & 3) - 1;
         dy = ((0x51166516 >> 2*row) & 3) - 1;
      }
      state = (0x8FE65831 >> 2*row) & 3;
   }
   *xp = *xp + dx;
   *yp = *yp + dy;
}
