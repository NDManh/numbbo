#include <stdio.h>
#include <assert.h>

#include "coco.h"

#include "coco_utilities.c"
#include "coco_problem.c"
#include "coco_strdup.c"

typedef struct {
    char *path;
    FILE *logfile;
    double *target_values;
    size_t number_of_target_values;
    size_t next_target_value;
    long number_of_evaluations;
} log_hitting_time_t;

static void lht_evaluate_function(coco_problem_t *self, double *x, double *y) {
    coco_transformed_problem_t *obj = (coco_transformed_problem_t *)self;
    log_hitting_time_t *state = (log_hitting_time_t *)obj->state;
    assert(obj->state != NULL);
    
    coco_evaluate_function(obj->inner_problem, x, y);
    state->number_of_evaluations++;

    /* Open logfile if it is not alread open */
    if (state->logfile == NULL) {
        state->logfile = fopen(state->path, "w");
        if (state->logfile == NULL) {
            char *buf;
            const char *error_format = 
                "lht_evaluate_function() failed to open log file '%s'.";
            size_t buffer_size = 
                snprintf(NULL, 0, error_format, state->path);
            buf = (char *)coco_allocate_memory(buffer_size);
            snprintf(buf, buffer_size, error_format, state->path);
            coco_error(buf);
            coco_free_memory(buf); /* Never reached */
        }
        fputs("target_value function_value number_of_evaluations\n",
              state->logfile);                    
    }
    
    /* Add a line for each hitting level we have reached. */
    while (y[0] <= state->target_values[state->next_target_value] &&
           state->next_target_value < state->number_of_target_values) {
        fprintf(state->logfile, "%e %e %li\n",
                state->target_values[state->next_target_value],
                y[0],
                state->number_of_evaluations);
        state->next_target_value++;
    }
    /* Flush output so that impatient users can see progress. */
    fflush(state->logfile);
}

static void lht_free_problem(coco_problem_t *self) {
    coco_transformed_problem_t *obj; 
    coco_problem_t *problem;
    log_hitting_time_t *state;

    assert(self != NULL);   
    obj = (coco_transformed_problem_t *)self;    
    problem = (coco_problem_t *)obj;

    assert(obj->state != NULL);
    state = (log_hitting_time_t *)obj->state;

    coco_free_memory(state->path);
    if (state->logfile != NULL) {
        fclose(state->logfile);
        state->logfile = NULL;
    }
    coco_free_memory(obj->state);
    if (obj->inner_problem != NULL) {
        coco_free_problem(obj->inner_problem);
        obj->inner_problem = NULL;
    }
    if (problem->problem_id != NULL)
        coco_free_memory(problem->problem_id);
    if (problem->problem_name != NULL)
        coco_free_memory(problem->problem_name);
    coco_free_memory(obj);
}

coco_problem_t *log_hitting_times(coco_problem_t *inner_problem,
                                    const double *target_values,
                                    const size_t number_of_target_values,
                                    const char *path) {
    coco_transformed_problem_t *obj = 
        coco_allocate_transformed_problem(inner_problem);
    coco_problem_t *problem = (coco_problem_t *)obj;
    log_hitting_time_t *state = (log_hitting_time_t *)coco_allocate_memory(sizeof(*state));

    problem->evaluate_function = lht_evaluate_function;
    problem->free_problem = lht_free_problem;

    state->number_of_evaluations = 0;
    state->path = coco_strdup(path);
    state->logfile = NULL; /* Open lazily in lht_evaluate_function(). */
    state->target_values = coco_duplicate_vector(target_values, 
                                                   number_of_target_values);
    state->number_of_target_values = number_of_target_values;
    state->next_target_value = 0;

    obj->state = state;
    return problem;
}
