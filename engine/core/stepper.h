/**
 * @file stepper.h
 * @brief Splits a frame's elapsed time into equal simulation steps no longer
 * than a maximum, and caps a stalled frame, so physics stays stable at any
 * display rate.
 */
#ifndef ENGINE_CORE_STEPPER_H_
#define ENGINE_CORE_STEPPER_H_

/* Returns the number of steps (at least 1) and writes their length to
 * `step`. The frame is clamped to `max_frame` seconds first. */
int stepper_plan(double frame_seconds, double max_step, double max_frame,
                 double* step);

#endif  // ENGINE_CORE_STEPPER_H_
