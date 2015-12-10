/*
 * Generic COCO runtime implementation.
 *
 * Other language interfaces might want to replace this so that memory
 * allocation and error handling go through the respective language
 * runtime.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "coco.h"
#include "coco_utilities.c"

void coco_error(const char *message, ...) {
  va_list args;

  fprintf(stderr, "FATAL ERROR: ");
  va_start(args, message);
  vfprintf(stderr, message, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(EXIT_FAILURE);
}

void coco_warning(const char *message, ...) {
  va_list args;

  if (coco_log_level >= COCO_WARNING) {
    fprintf(stderr, "WARNING: ");
    va_start(args, message);
    vfprintf(stderr, message, args);
    va_end(args);
    fprintf(stderr, "\n");
  }
}

void coco_info(const char *message, ...) {
  va_list args;

  if (coco_log_level >= COCO_INFO) {
    fprintf(stdout, "INFO: ");
    va_start(args, message);
    vfprintf(stdout, message, args);
    va_end(args);
    fprintf(stdout, "\n");
  }
}

void coco_debug(const char *message, ...) {
  va_list args;

  if (coco_log_level >= COCO_DEBUG) {
    fprintf(stdout, "DEBUG: ");
    va_start(args, message);
    vfprintf(stdout, message, args);
    va_end(args);
    fprintf(stdout, "\n");
  }
}

void *coco_allocate_memory(const size_t size) {
  void *data;
  if (size == 0) {
    coco_error("coco_allocate_memory() called with 0 size.");
    return NULL; /* never reached */
  }
  data = malloc(size);
  if (data == NULL)
    coco_error("coco_allocate_memory() failed.");
  return data;
}

void coco_free_memory(void *data) {
  free(data);
}