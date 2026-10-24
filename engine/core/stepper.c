#include "stepper.h"

#include <math.h>

int stepper_plan(double frame_seconds, double max_step, double max_frame,
                 double* step) {
  double frame = frame_seconds;
  if (!(frame > 0.0)) frame = 0.0;
  if (frame > max_frame) frame = max_frame;
  int count = (int)ceil(frame / max_step);
  if (count < 1) count = 1;
  *step = frame / count;
  return count;
}
