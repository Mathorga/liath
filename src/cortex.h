/*
*****************************************************************
cortex.h

Copyright (C) 2021 Luka Micheletti
*****************************************************************
*/

#ifndef __CORTEX__
#define __CORTEX__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

// Translate an id wrapping it to the provided size (pacman effect).
// WARNING: Only works with signed types and does not show errors otherwise.
// [i] is the given index.
// [n] is the size over which to wrap.
#define WRAP(i, n) ((i) >= 0 ? ((i) % (n)) : ((n) + ((i) % (n))))

// Computes the diameter of a square neighborhood given its radius.
#define NH_DIAM_2D(r) (2 * (r) + 1)

// Computes the number of neighbors in a square neighborhood given its diameter.
#define NH_COUNT_2D(d) ((d) * (d) - 1)

// Translates bidimensional indexes to a monodimensional one.
// |i| is the row index.
// |j| is the column index.
// |m| is the number of columns (length of the rows).
#define IDX2D(i, j, m) (((m) * (j)) + (i))

// Translates tridimensional indexes to a monodimensional one.
// |i| is the index in the first dimension.
// |j| is the index in the second dimension.
// |k| is the index in the third dimension.
// |m| is the size of the first dimension.
// |n| is the size of the second dimension.
#define IDX3D(i, j, k, m, n) (((m) * (n) * (k)) + ((m) * (j)) + (i))

#define EVOL_STEP_NEVER 0x0000FFFFU

#define PULSE_WINDOW_LARGE 0x3FU
#define PULSE_WINDOW_MID 0x1FU
#define PULSE_WINDOW_SMALL 0x0AU

#define SAMPLE_WINDOW_LARGE 0x40U
#define SAMPLE_WINDOW_MID 0x20U
#define SAMPLE_WINDOW_SMALL 0x10U

#define MAX_SYN_STRENGTH 0x07U
#define MAX_CHANCE 0xFFFFU

// Completely arbitrary values used to define a sort of acceptable cortex right away.
#define DEFAULT_THRESHOLD 0x88U
#define DEFAULT_STARTING_VALUE 0x00U
#define DEFAULT_RECOVERY_VALUE -0x2A
#define DEFAULT_MAX_TOUCH 0.25F
#define DEFAULT_EXC_VALUE 0x20U
#define DEFAULT_DECAY_RATE 0x01U
#define DEFAULT_PULSE_WINDOW PULSE_WINDOW_LARGE
#define DEFAULT_EVOL_STEP 0x0000000AU
#define DEFAULT_INHEXC_RANGE 0x64U
#define DEFAULT_INHEXC_RATIO 0x06U
#define DEFAULT_SAMPLE_WINDOW SAMPLE_WINDOW_SMALL
#define DEFAULT_MAX_TOT_STRENGTH 0x20U
#define DEFAULT_SYNGEN_CHANCE 0x02A0U
#define DEFAULT_SYNSTR_CHANCE 0x00A0U

typedef uint8_t byte;

typedef int16_t neuron_value_t;

// A mask made of 8 bytes can hold up to 48 neighbors (i.e. radius = 3).
// Using 16 bytes the radius can be up to 5 (120 neighbors).
typedef uint64_t nh_mask_t;
typedef int8_t nh_radius_t;
typedef uint8_t syn_count_t;
typedef uint8_t syn_strength_t;
typedef uint16_t ticks_count_t;
typedef uint32_t evol_step_t;
typedef uint64_t pulse_mask_t;
typedef int8_t spikes_count_t;
typedef uint32_t chance_t;
typedef uint32_t rand_state_t;

typedef int32_t cortex_size_t;

typedef enum bool_t {
    FALSE = 0,
    TRUE = 1
} bool_t;

typedef enum pulse_mapping_t {
    // Values are forced to 32 bit integers by using big enough values: 100000 is 17 bits long, so 32 bits are automatically allocated.
    // Linear.
    PULSE_MAPPING_LINEAR = 0x10000,
    // Floored proportional.
    PULSE_MAPPING_FPROP = 0x10001,
    // Rounded proportional.
    PULSE_MAPPING_RPROP = 0x10002,
    // Double floored proportional.
    PULSE_MAPPING_DFPROP = 0x10003,
} pulse_mapping_t;

typedef struct input2d_t {
    cortex_size_t x0;
    cortex_size_t y0;
    cortex_size_t x1;
    cortex_size_t y1;
    neuron_value_t exc_value;
    pulse_mapping_t pulse_mapping;
    ticks_count_t* values;
} input2d_t;

// TODO output2d.

/// Neuron.
typedef struct neuron_t {
    // Neighborhood connections pattern (SYNapses ACtivation state):
    // 1|1|0
    // 0|x|1 => 1100x1100
    // 1|0|0
    nh_mask_t synac_mask;
    // Neighborhood excitatory states pattern (SYNapses EXcitatory state), defines whether the synapses from the neighbors are excitatory (1) or inhibitory (0).
    // Only values corresponding to active synapses are used.
    nh_mask_t synex_mask;
    // Neighborhood synapses strength pattern (SYNapses STRength). Defines a 3 bit value defined as [cba].
    nh_mask_t synstr_mask_a;
    nh_mask_t synstr_mask_b;
    nh_mask_t synstr_mask_c;


    // Activation history pattern:
    //           |<--pulse_window-->|
    // xxxxxxxxxx01001010001010001001--------> t
    //                              ^
    // Used to know the pulse frequency in a given moment (e.g. for syngen).
    pulse_mask_t pulse_mask;
    // Amount of activations in the cortex' pulse window.
    spikes_count_t pulse;


    // Current internal value.
    neuron_value_t value;
    // Maximum number of synapses to the neuron. Cannot be greater than the cortex' max_syn_count.
    syn_count_t max_syn_count;
    // Amount of connected neighbors.
    syn_count_t syn_count;
    // Total amount of syn strength from input neurons.
    syn_strength_t tot_syn_strength;
    // Proportion between excitatory and inhibitory generated synapses. Can vary between 0 and cortex.inhexc_range.
    // inhexc_ratio = 0 -> all synapses are excitatory.
    // inhexc_ratio = cortex.inhexc_range -> all synapses are inhibitory.
    chance_t inhexc_ratio;
} neuron_t;

/// 2D cortex of neurons.
typedef struct cortex2d_t {
    // Width of the cortex.
    cortex_size_t width;
    // Height of the cortex.
    cortex_size_t height;
    // Ticks performed since cortex creation.
    ticks_count_t ticks_count;
    // Random state. The random state has to be consistent inside a cortex, so it qualifies as a property.
    // The random state is used to generate consistent random numbers across the lifespan of a cortex, therefore should NEVER be manually changed.
    rand_state_t rand_state;
    // Evolutions performed since cortex creation.
    ticks_count_t evols_count;
    // Amount of ticks between each evolution.
    ticks_count_t evol_step;
    // Length of the window used to count pulses in the cortex' neurons.
    // TODO Switch "beat" and "pulse".
    spikes_count_t pulse_window;


    // Radius of each neuron's neighborhood.
    nh_radius_t nh_radius;
    neuron_value_t fire_threshold;
    neuron_value_t recovery_value;
    neuron_value_t exc_value;
    neuron_value_t decay_value;


    // Chance (out of 0xFFFFU) of synapse generation or deletion (structural plasticity).
    chance_t syngen_chance;
    // Chance (out of 0xFFFFU) of synapse strengthening or weakening (functional plasticity).
    chance_t synstr_chance;


    // Max strength available for a single neuron, meaning the strength of all the synapses coming to each neuron cannot be more than this.
    syn_strength_t max_tot_strength;
    // Maximum number of synapses between a neuron and its neighbors.
    syn_count_t max_syn_count;
    // Maximum range for inhexc chance: single neurons' inhexc ratio will vary between 0 and inhexc_range. 0 means all excitatory, inhexc_range means all inhibitory.
    chance_t inhexc_range;


    // Length of the window used to sample inputs.
    ticks_count_t sample_window;
    pulse_mapping_t pulse_mapping;

    neuron_t* neurons;
} cortex2d_t;

// TODO cortex3d_t


// ########################################## Initialization functions ##########################################

/// Initializes the given input with the given values.
error_code_t i2d_init(input2d_t** input, cortex_size_t x0, cortex_size_t y0, cortex_size_t x1, cortex_size_t y1, neuron_value_t exc_value, pulse_mapping_t pulse_mapping);

/// Initializes the given cortex with default values.
error_code_t c2d_init(cortex2d_t** cortex, cortex_size_t width, cortex_size_t height, nh_radius_t nh_radius);

/// Destroys the given input2d and frees memory.
error_code_t i2d_destroy(input2d_t* input);

/// Destroys the given cortex2d and frees memory.
error_code_t c2d_destroy(cortex2d_t* cortex);

/// Returns a cortex with the same properties as the given one.
error_code_t c2d_copy(cortex2d_t* to, cortex2d_t* from);


// ########################################## Setter functions ##################################################

/// Sets the neighborhood radius for all neurons in the cortex.
error_code_t c2d_set_nhradius(cortex2d_t* cortex, nh_radius_t radius);

/// Sets the neighborhood mask for all neurons in the cortex.
void c2d_set_nhmask(cortex2d_t* cortex, nh_mask_t mask);

/// Sets the evolution step for the cortex.
void c2d_set_evol_step(cortex2d_t* cortex, evol_step_t evol_step);

/// Sets the pulse window width for the cortex.
void c2d_set_pulse_window(cortex2d_t* cortex, spikes_count_t window);

/// Sets the sample window for the cortex.
void c2d_set_sample_window(cortex2d_t* cortex, ticks_count_t sample_window);

/// Sets the fire threshold for all neurons in the cortex.
void c2d_set_fire_threshold(cortex2d_t* cortex, neuron_value_t threshold);

/// Sets the maximum number of (input) synapses for the neurons of the cortex.
/// @param cortex The cortex to edit.
/// @param syn_count The max number of allowable synapses.
void c2d_set_max_syn_count(cortex2d_t* cortex, syn_count_t syn_count);

/// Sets the maximum allowable touch for each neuron in the network.
/// A neuron touch is defined as its synapses count divided by its total neighbors count.
/// @param touch The touch to assign the cortex. Only values between 0 and 1 are allowed.
void c2d_set_max_touch(cortex2d_t* cortex, float touch);

/// Sets the preferred input mapping for the given cortex.
void c2d_set_pulse_mapping(cortex2d_t* cortex, pulse_mapping_t pulse_mapping);

/// Sets the range for excitatory to inhibitory ratios in single neurons.
void c2d_set_inhexc_range(cortex2d_t* cortex, chance_t inhexc_range);

/// Sets the proportion between excitatory and inhibitory generated synapses.
void c2d_set_inhexc_ratio(cortex2d_t* cortex, chance_t inhexc_ratio);

/// Sets whether the tick pass should wrap around the edges (pacman effect).
void c2d_set_wrapped(cortex2d_t* cortex, bool_t wrapped);

/// Disables self connections whithin the specified bounds.
void c2d_syn_disable(cortex2d_t* cortex, cortex_size_t x0, cortex_size_t y0, cortex_size_t x1, cortex_size_t y1);

#ifdef __cplusplus
}
#endif

#endif
