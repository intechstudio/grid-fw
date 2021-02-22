#ifndef GRID_EXPR_H_INCLUDED
#define GRID_EXPR_H_INCLUDED

#include "sam.h"
#include "grid_module.h"

#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 


#define GRID_EXPR_INPUT_STRING_MAXLENGTH 100
#define GRID_EXPR_OUTPUT_STRING_MAXLENGTH 100

struct grid_expr_model
{

    struct grid_ui_event* current_event;

    


    uint8_t input_string[GRID_EXPR_OUTPUT_STRING_MAXLENGTH];
    uint8_t input_string_length;

    uint8_t output_string[GRID_EXPR_OUTPUT_STRING_MAXLENGTH+1]; 
    uint8_t output_string_length;
	

};



grid_expr_clear_input(struct grid_expr_model* expr);
grid_expr_clear_output(struct grid_expr_model* expr);


volatile struct grid_expr_model grid_expr_state;



void grid_expr_init(struct grid_expr_model* expr);



grid_expr_set_current_event(struct grid_expr_model* expr, struct grid_ui_event* eve);
grid_expr_evaluate(struct grid_expr_model* expr, char* input_str, uint8_t input_length);


#endif

