/*
 * File:        common.h
 * Purpose:     File to be included by all project files
 *
 * Notes:
 */

#ifndef _COMMON_H_
#define _COMMON_H_

/********************************************************************/

/*
 * Include the generic CPU header file
 */
#include "arm_cm4.h"

/*
 * Include the cpu specific header file
 */
#if (defined(CPU_MK40N512VMD100))
  #include <freescale/MK40N512VMD100.h>
#elif (defined(CPU_MK53N512VMD100))
  #include <freescale/MK53N512CMD100.h>
#elif (defined(CPU_MK60N512VMD100))
  #include <freescale/MK60N512VMD100.h>
#else
  #error "No valid CPU defined"
#endif

/********************************************************************/

#endif /* _COMMON_H_ */
