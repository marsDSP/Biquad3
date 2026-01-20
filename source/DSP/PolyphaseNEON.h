#pragma once

#ifndef BIQUAD3_POLYPHASENEON_H
#define BIQUAD3_POLYPHASENEON_H

#include <arm_neon.h>
#include <vector>
#include <algorithm>

/*
 * The difference here is AVX2 (Intel) is 256-bit (8 floats) while NEON (M1) is 128-bit (4 floats).
 * This may seem like a downgrade, but the Apple Silicon architecture is actually pretty massive; it has execution
 * units that can run 4 NEON instructions in parallel, so it's often faster than Intel chips despite being narrower!
 */

class PolyphaseStageNEON {

};

#endif
