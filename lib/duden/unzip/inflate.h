#pragma once

#ifdef __cplusplus
extern "C"
#endif
int duden_inflate(const void* input,
                  unsigned inputSize,
                  void* output,
                  unsigned* outputSize);
