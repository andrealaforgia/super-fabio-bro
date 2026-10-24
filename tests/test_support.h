#ifndef TESTS_TEST_SUPPORT_H_
#define TESTS_TEST_SUPPORT_H_
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define RUN(test)            \
  do {                       \
    printf("  %s\n", #test); \
    test();                  \
  } while (0)

#define CHECK(condition)                                                 \
  do {                                                                   \
    if (!(condition)) {                                                  \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,  \
              #condition);                                               \
      exit(1);                                                           \
    }                                                                    \
  } while (0)

#define CHECK_NEAR(a, b, tolerance)                                      \
  do {                                                                   \
    double a_ = (a), b_ = (b);                                           \
    if (fabs(a_ - b_) > (tolerance)) {                                   \
      fprintf(stderr, "%s:%d: %s = %g, expected %g (+/- %g)\n", __FILE__, \
              __LINE__, #a, a_, b_, (double)(tolerance));                \
      exit(1);                                                           \
    }                                                                    \
  } while (0)

#endif  // TESTS_TEST_SUPPORT_H_
