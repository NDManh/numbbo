
#include <stdbool.h>
#include <assert.h>

#include "coco.h"
#include "coco_problem.c"

typedef struct {
    double *offset;
    double *shifted_x;
    coco_free_function_t old_free_problem;
} shift_variables_state_t;

static void _sv_evaluate_function(coco_problem_t *self, double *x, double *y) {
    size_t i;
    coco_transformed_problem_t *problem;
    shift_variables_state_t *state;
    assert(self != NULL);
    problem = (coco_transformed_problem_t *)self;

    assert(problem->inner_problem != NULL);
    assert(problem->state != NULL);
    state = (shift_variables_state_t *)problem->state;

    for (i = 0; i < self->number_of_variables; ++i) {
        state->shifted_x[i] = x[i] - state->offset[i];
    }
    coco_evaluate_function(problem->inner_problem, state->shifted_x, y);
}

static void _sv_free_problem(coco_problem_t *self) {
    coco_transformed_problem_t *problem;
    shift_variables_state_t *state;
    assert(self != NULL);
    problem = (coco_transformed_problem_t *)self;

    assert(problem->inner_problem != NULL);
    assert(problem->state != NULL);
    state = (shift_variables_state_t *)problem->state;
    
    coco_free_memory(state->shifted_x);
    coco_free_memory(state->offset);
    state->old_free_problem(self);
}

/* Shift all variables of ${inner_problem} by ${amount}.
 */
coco_problem_t *shift_variables(coco_problem_t *inner_problem,
                                  const double *offset,
                                  const bool shift_bounds) {
    size_t number_of_variables;
    coco_transformed_problem_t *obj;
    coco_problem_t *problem;
    shift_variables_state_t *state;
    assert(inner_problem != NULL);
    assert(offset != NULL);
    if (shift_bounds)
        coco_error("shift_bounds not implemented.");

    number_of_variables = inner_problem->number_of_variables;
    obj = coco_allocate_transformed_problem(inner_problem);
    problem = (coco_problem_t *)obj;

    state = (shift_variables_state_t *)coco_allocate_memory(sizeof(*state));
    state->offset = coco_duplicate_vector(offset, number_of_variables);
    state->shifted_x = coco_allocate_vector(number_of_variables);
    state->old_free_problem = problem->free_problem;

    obj->state = state;

    problem->evaluate_function = _sv_evaluate_function;
    problem->free_problem = _sv_free_problem;
    return problem;
}
