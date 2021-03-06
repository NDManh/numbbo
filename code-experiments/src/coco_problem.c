#include <float.h>
#include "coco.h"

#include "coco_utilities.c"

/***********************************
 * Global definitions in this file
 *
 * TODO: are these really needed? 
 * Only if they would need to be used from
 * outside. Benchmarks that are included in
 * coco_suite_benchmark.c can include coco_problem.c
 * directly due to the amalgamate magic.
 * 
 ***********************************/

coco_problem_t *coco_problem_allocate(const size_t number_of_variables,
                                      const size_t number_of_objectives,
                                      const size_t number_of_constraints);
coco_problem_t *coco_problem_duplicate(coco_problem_t *other);
typedef void (*coco_transformed_free_data_t)(void *data);

/* typedef coco_transformed_data_t; */
coco_problem_t *coco_transformed_allocate(coco_problem_t *inner_problem,
                                          void *userdata,
                                          coco_transformed_free_data_t free_data);
void *coco_transformed_get_data(coco_problem_t *self);
coco_problem_t *coco_transformed_get_inner_problem(coco_problem_t *self);

/* typedef coco_stacked_problem_data_t; */
typedef void (*coco_stacked_problem_free_data_t)(void *data);
coco_problem_t *coco_stacked_problem_allocate(coco_problem_t *problem1_to_be_stacked,
                                              coco_problem_t *problem2_to_be_stacked);

/***********************************/

void coco_problem_free(coco_problem_t *self) {
  assert(self != NULL);
  if (self->free_problem != NULL) {
    self->free_problem(self);
  } else {
    /* Best guess at freeing all relevant structures */
    if (self->smallest_values_of_interest != NULL)
      coco_free_memory(self->smallest_values_of_interest);
    if (self->largest_values_of_interest != NULL)
      coco_free_memory(self->largest_values_of_interest);
    if (self->best_parameter != NULL)
      coco_free_memory(self->best_parameter);
    if (self->best_value != NULL)
      coco_free_memory(self->best_value);
    if (self->nadir_value != NULL)
      coco_free_memory(self->nadir_value);
    if (self->problem_name != NULL)
      coco_free_memory(self->problem_name);
    if (self->problem_id != NULL)
      coco_free_memory(self->problem_id);
    if (self->problem_type != NULL)
      coco_free_memory(self->problem_type);
    if (self->data != NULL)
      coco_free_memory(self->data);
    self->smallest_values_of_interest = NULL;
    self->largest_values_of_interest = NULL;
    self->best_parameter = NULL;
    self->best_value = NULL;
    self->nadir_value = NULL;
    self->data = NULL;
    coco_free_memory(self);
  }
}

/**
 * coco_problem_allocate(number_of_variables):
 *
 * Allocate and pre-populate a new coco_problem_t for a problem with
 * ${number_of_variables}.
 */
coco_problem_t *coco_problem_allocate(const size_t number_of_variables,
                                      const size_t number_of_objectives,
                                      const size_t number_of_constraints) {
  coco_problem_t *problem;
  problem = (coco_problem_t *) coco_allocate_memory(sizeof(*problem));
  /* Initialize fields to sane/safe defaults */
  problem->initial_solution = NULL;
  problem->evaluate_function = NULL;
  problem->evaluate_constraint = NULL;
  problem->recommend_solutions = NULL;
  problem->free_problem = NULL;
  problem->number_of_variables = number_of_variables;
  problem->number_of_objectives = number_of_objectives;
  problem->number_of_constraints = number_of_constraints;
  problem->smallest_values_of_interest = coco_allocate_vector(number_of_variables);
  problem->largest_values_of_interest = coco_allocate_vector(number_of_variables);
  problem->best_parameter = coco_allocate_vector(number_of_variables);
  problem->best_value = coco_allocate_vector(number_of_objectives);
  if (number_of_objectives > 1)
    problem->nadir_value = coco_allocate_vector(number_of_objectives);
  else
    problem->nadir_value = NULL;
  problem->problem_name = NULL;
  problem->problem_id = NULL;
  problem->problem_type = NULL;
  problem->evaluations = 0;
  problem->final_target_delta[0] = 1e-8; /* in case to be modified by the benchmark */
  problem->best_observed_fvalue[0] = DBL_MAX;
  problem->best_observed_evaluation[0] = 0;
  problem->suite_dep_index = 0;
  problem->suite_dep_function = 0;
  problem->suite_dep_instance = 0;
  problem->data = NULL;
  return problem;
}

/**
 * Creates a duplicate of the 'other' for all fields except for data, which points to NULL.
 */
coco_problem_t *coco_problem_duplicate(coco_problem_t *other) {
  size_t i;
  coco_problem_t *problem;
  problem = coco_problem_allocate(other->number_of_variables, other->number_of_objectives,
      other->number_of_constraints);

  problem->initial_solution = other->initial_solution;
  problem->evaluate_function = other->evaluate_function;
  problem->evaluate_constraint = other->evaluate_constraint;
  problem->recommend_solutions = other->recommend_solutions;
  problem->free_problem = NULL;

  for (i = 0; i < problem->number_of_variables; ++i) {
    problem->smallest_values_of_interest[i] = other->smallest_values_of_interest[i];
    problem->largest_values_of_interest[i] = other->largest_values_of_interest[i];
    if (other->best_parameter)
      problem->best_parameter[i] = other->best_parameter[i];
  }

  if (other->best_value)
    for (i = 0; i < problem->number_of_objectives; ++i) {
      problem->best_value[i] = other->best_value[i];
    }

  if (other->nadir_value)
    for (i = 0; i < problem->number_of_objectives; ++i) {
      problem->nadir_value[i] = other->nadir_value[i];
    }

  problem->problem_name = coco_strdup(other->problem_name);
  problem->problem_id = coco_strdup(other->problem_id);
  problem->problem_type = coco_strdup(other->problem_type);

  problem->evaluations = other->evaluations;
  problem->final_target_delta[0] = other->final_target_delta[0];
  problem->best_observed_fvalue[0] = other->best_observed_fvalue[0];
  problem->best_observed_evaluation[0] = other->best_observed_evaluation[0];

  problem->suite_dep_index = other->suite_dep_index;
  problem->suite_dep_function = other->suite_dep_function;
  problem->suite_dep_instance = other->suite_dep_instance;

  problem->data = NULL;

  return problem;
}

/**
 * Allocate a problem using scalar values for smallest_value_of_interest, largest_value_of_interest
 * and best_parameter.
 */
coco_problem_t *coco_problem_allocate_from_scalars(const char *problem_name,
                                                   coco_evaluate_function_t evaluate_function,
                                                   coco_free_function_t free_function,
                                                   size_t number_of_variables,
                                                   double smallest_value_of_interest,
                                                   double largest_value_of_interest,
                                                   double best_parameter) {
  size_t i;
  coco_problem_t *problem = coco_problem_allocate(number_of_variables, 1, 0);

  problem->problem_name = coco_strdup(problem_name);
  problem->number_of_variables = number_of_variables;
  problem->number_of_objectives = 1;
  problem->number_of_constraints = 0;
  problem->evaluate_function = evaluate_function;
  problem->free_problem = free_function;

  for (i = 0; i < number_of_variables; ++i) {
    problem->smallest_values_of_interest[i] = smallest_value_of_interest;
    problem->largest_values_of_interest[i] = largest_value_of_interest;
    problem->best_parameter[i] = best_parameter;
  }
  return problem;
}

/**
 * Checks whether the given string is in the right format to be a problem_id (does not contain any
 * non-alphanumeric characters besides - and _.).
 */
static int coco_problem_id_is_fine(const char *id, ...) {
  va_list args;
  const int reject = 0;
  const int accept = 1;
  const char *cp;
  char *s;
  int result = accept;

  va_start(args, id);
  s = coco_vstrdupf(id, args);
  va_end(args);
  for (cp = s; *cp != '\0'; ++cp) {
    if (('A' <= *cp) && (*cp <= 'Z'))
      continue;
    if (('a' <= *cp) && (*cp <= 'z'))
      continue;
    if ((*cp == '_') || (*cp == '-'))
      continue;
    if (('0' <= *cp) && (*cp <= '9'))
      continue;
    result = reject;
  }
  coco_free_memory(s);
  return result;
}

/**
 * Formatted printing of a problem ID, mimicking sprintf(id, ...) while taking care of memory
 * (de-)allocations and verifying that the id is in the correct format.
 */
static void coco_problem_set_id(coco_problem_t *problem, const char *id, ...) {
  va_list args;

  va_start(args, id);
  if (problem->problem_id != NULL)
    coco_free_memory(problem->problem_id);
  problem->problem_id = coco_vstrdupf(id, args);
  va_end(args);
  if (!coco_problem_id_is_fine(problem->problem_id)) {
    coco_error("Problem id should only contain standard chars, not like '%s'", problem->problem_id);
  }
}

/**
 * Formatted printing of a problem name, mimicking sprintf(name, ...) while taking care of memory
 * (de-)allocation.
 */
static void coco_problem_set_name(coco_problem_t *problem, const char *name, ...) {
  va_list args;

  va_start(args, name);
  if (problem->problem_name != NULL)
    coco_free_memory(problem->problem_name);
  problem->problem_name = coco_vstrdupf(name, args);
  va_end(args);
}

/**
 * Formatted printing of a problem type, mimicking sprintf(id, ...) while taking care of memory
 * (de-)allocation.
 */
static void coco_problem_set_type(coco_problem_t *problem, const char *type, ...) {
  va_list args;

  va_start(args, type);
  if (problem->problem_type != NULL)
    coco_free_memory(problem->problem_type);
  problem->problem_type = coco_vstrdupf(type, args);
  va_end(args);
}

/**
 * Generic data member of a transformed (or "outer") coco_problem_t.
 */
typedef struct {
  coco_problem_t *inner_problem;
  void *data;
  coco_transformed_free_data_t free_data;
} coco_transformed_data_t;

static void transformed_evaluate_function(coco_problem_t *self, const double *x, double *y) {
  coco_transformed_data_t *data;
  assert(self != NULL);
  assert(self->data != NULL);
  data = self->data;
  assert(data->inner_problem != NULL);

  coco_evaluate_function(data->inner_problem, x, y);
}

static void transformed_evaluate_constraint(coco_problem_t *self, const double *x, double *y) {
  coco_transformed_data_t *data;
  assert(self != NULL);
  assert(self->data != NULL);
  data = self->data;
  assert(data->inner_problem != NULL);

  coco_evaluate_constraint(data->inner_problem, x, y);
}

static void transformed_recommend_solutions(coco_problem_t *self, const double *x, size_t number_of_solutions) {
  coco_transformed_data_t *data;
  assert(self != NULL);
  assert(self->data != NULL);
  data = self->data;
  assert(data->inner_problem != NULL);

  coco_recommend_solutions(data->inner_problem, x, number_of_solutions);
}

static void transformed_free_problem(coco_problem_t *self) {
  coco_transformed_data_t *data;

  assert(self != NULL);
  assert(self->data != NULL);
  data = self->data;
  assert(data->inner_problem != NULL);

  if (data->inner_problem != NULL) {
    coco_problem_free(data->inner_problem);
    data->inner_problem = NULL;
  }
  if (data->data != NULL) {
    if (data->free_data != NULL) {
      data->free_data(data->data);
      data->free_data = NULL;
    }
    coco_free_memory(data->data);
    data->data = NULL;
  }
  /* Let the generic free problem code deal with the rest of the
   * fields. For this we clear the free_problem function pointer and
   * recall the generic function.
   */
  self->free_problem = NULL;
  coco_problem_free(self);
}

/**
 * coco_transformed_allocate(inner_problem):
 *
 * Allocate a transformed problem that wraps ${inner_problem}. By
 * default all methods will dispatch to the ${inner_problem} method.
 *
 */
coco_problem_t *coco_transformed_allocate(coco_problem_t *inner_problem,
                                          void *userdata,
                                          coco_transformed_free_data_t free_data) {
  coco_transformed_data_t *data;
  coco_problem_t *self;

  data = coco_allocate_memory(sizeof(*data));
  data->inner_problem = inner_problem;
  data->data = userdata;
  data->free_data = free_data;

  self = coco_problem_duplicate(inner_problem);
  self->evaluate_function = transformed_evaluate_function;
  self->evaluate_constraint = transformed_evaluate_constraint;
  self->recommend_solutions = transformed_recommend_solutions;
  self->free_problem = transformed_free_problem;
  self->data = data;
  return self;
}

void *coco_transformed_get_data(coco_problem_t *self) {
  assert(self != NULL);
  assert(self->data != NULL);
  assert(((coco_transformed_data_t *) self->data)->data != NULL);

  return ((coco_transformed_data_t *) self->data)->data;
}

coco_problem_t *coco_transformed_get_inner_problem(coco_problem_t *self) {
  assert(self != NULL);
  assert(self->data != NULL);
  assert(((coco_transformed_data_t *) self->data)->inner_problem != NULL);

  return ((coco_transformed_data_t *) self->data)->inner_problem;
}

/** type provided COCO problem data for a stacked COCO problem
 */
typedef struct {
  coco_problem_t *problem1;
  coco_problem_t *problem2;
} coco_stacked_problem_data_t;

static void coco_stacked_problem_evaluate(coco_problem_t *self, const double *x, double *y) {
  coco_stacked_problem_data_t* data = (coco_stacked_problem_data_t *) self->data;

  assert(
      coco_problem_get_number_of_objectives(self)
          == coco_problem_get_number_of_objectives(data->problem1)
              + coco_problem_get_number_of_objectives(data->problem2));

  coco_evaluate_function(data->problem1, x, &y[0]);
  coco_evaluate_function(data->problem2, x, &y[coco_problem_get_number_of_objectives(data->problem1)]);
}

static void coco_stacked_problem_evaluate_constraint(coco_problem_t *self, const double *x, double *y) {
  coco_stacked_problem_data_t* data = (coco_stacked_problem_data_t*) self->data;

  assert(
      coco_problem_get_number_of_constraints(self)
          == coco_problem_get_number_of_constraints(data->problem1)
              + coco_problem_get_number_of_constraints(data->problem2));

  if (coco_problem_get_number_of_constraints(data->problem1) > 0)
    coco_evaluate_constraint(data->problem1, x, y);
  if (coco_problem_get_number_of_constraints(data->problem2) > 0)
    coco_evaluate_constraint(data->problem2, x, &y[coco_problem_get_number_of_constraints(data->problem1)]);
}

static void coco_stacked_problem_free(coco_problem_t *self) {
  coco_stacked_problem_data_t *data;

  assert(self != NULL);
  assert(self->data != NULL);
  data = self->data;

  if (data->problem1 != NULL) {
    coco_problem_free(data->problem1);
    data->problem1 = NULL;
  }
  if (data->problem2 != NULL) {
    coco_problem_free(data->problem2);
    data->problem2 = NULL;
  }
  /* Let the generic free problem code deal with the rest of the
   * fields. For this we clear the free_problem function pointer and
   * recall the generic function.
   */
  self->free_problem = NULL;
  coco_problem_free(self);
}

/**
 * Return a problem that stacks the output of two problems, namely
 * of coco_evaluate_function and coco_evaluate_constraint. The accepted
 * input remains the same and must be identical for the stacked
 * problems. 
 * 
 * This is particularly useful to generate multiobjective problems,
 * e.g. a biobjective problem from two single objective problems.
 *
 * Details: regions of interest must either agree or at least one
 * of them must be NULL. Best parameter becomes somewhat meaningless. 
 */
coco_problem_t *coco_stacked_problem_allocate(coco_problem_t *problem1,
                                              coco_problem_t *problem2) {

  const size_t number_of_variables = coco_problem_get_dimension(problem1);
  const size_t number_of_objectives = coco_problem_get_number_of_objectives(problem1)
      + coco_problem_get_number_of_objectives(problem2);
  const size_t number_of_constraints = coco_problem_get_number_of_constraints(problem1)
      + coco_problem_get_number_of_constraints(problem2);
  size_t i;
  char *s;
  const double *smallest, *largest;
  coco_stacked_problem_data_t *data;
  coco_problem_t *problem; /* the new coco problem */

  assert(coco_problem_get_dimension(problem1) == coco_problem_get_dimension(problem2));

  problem = coco_problem_allocate(number_of_variables, number_of_objectives, number_of_constraints);

  s = coco_strconcat(coco_problem_get_id(problem1), "__");
  problem->problem_id = coco_strconcat(s, coco_problem_get_id(problem2));
  coco_free_memory(s);
  s = coco_strconcat(coco_problem_get_name(problem1), " + ");
  problem->problem_name = coco_strconcat(s, coco_problem_get_name(problem2));
  coco_free_memory(s);

  problem->evaluate_function = coco_stacked_problem_evaluate;
  if (number_of_constraints > 0)
    problem->evaluate_constraint = coco_stacked_problem_evaluate_constraint;

  /* set/copy "boundaries" and best_parameter */
  smallest = problem1->smallest_values_of_interest;
  if (smallest == NULL)
    smallest = problem2->smallest_values_of_interest;

  largest = problem1->largest_values_of_interest;
  if (largest == NULL)
    largest = problem2->largest_values_of_interest;

  for (i = 0; i < number_of_variables; ++i) {
    if (problem2->smallest_values_of_interest != NULL)
      assert(smallest[i] == problem2->smallest_values_of_interest[i]);
    if (problem2->largest_values_of_interest != NULL)
      assert(largest[i] == problem2->largest_values_of_interest[i]);

    if (smallest != NULL)
      problem->smallest_values_of_interest[i] = smallest[i];
    if (largest != NULL)
      problem->largest_values_of_interest[i] = largest[i];

    if (problem->best_parameter) /* logger_bbob doesn't work then anymore */
      coco_free_memory(problem->best_parameter);
    problem->best_parameter = NULL;
  }

  /* Compute the ideal and nadir values */
  assert(problem->best_value);
  assert(problem->nadir_value);
  problem->best_value[0] = problem1->best_value[0];
  problem->best_value[1] = problem2->best_value[0];
  coco_evaluate_function(problem1, problem2->best_parameter, &problem->nadir_value[0]);
  coco_evaluate_function(problem2, problem1->best_parameter, &problem->nadir_value[1]);

  /* setup data holder */
  data = coco_allocate_memory(sizeof(*data));
  data->problem1 = problem1;
  data->problem2 = problem2;

  problem->data = data;
  problem->free_problem = coco_stacked_problem_free; /* free self->data and coco_problem_free(self) */

  return problem;
}

