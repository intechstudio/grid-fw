/******************************************************************************

                            Online C Compiler.
                Code, Compile, Run and Debug C program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/


#include "grid_expr.h"

void grid_expr_init(struct grid_expr_model* expr){

    expr->current_event = NULL;

    grid_expr_clear_input(expr);
    grid_expr_clear_output(expr);

}


grid_expr_clear_input(struct grid_expr_model* expr){

    expr->input_string_length = 0;

    for (uint32_t i=0; i<GRID_EXPR_INPUT_STRING_MAXLENGTH; i++){

        expr->input_string[i] = 0;

    }

}

grid_expr_clear_output(struct grid_expr_model* expr){


    expr->output_string_length = 0;

    for (uint32_t i=0; i<GRID_EXPR_OUTPUT_STRING_MAXLENGTH; i++){

        expr->output_string[i] = 0;

    }

}



grid_expr_set_current_event(struct grid_expr_model* expr, struct grid_ui_event* eve){

    expr->current_event = eve;
}


grid_expr_evaluate(struct grid_expr_model* expr, char* input_str, uint8_t input_length){

    uint8_t debug_level = 0;

    grid_expr_clear_input(expr);
    grid_expr_clear_output(expr);

    for (uint32_t i=0; i<input_length; i++){

        expr->input_string[i] = input_str[i];
        
    }

    expr->input_string_length = input_length;


    if (debug_level) printf("Input: %s\r\n", expr->input_string);
    if (debug_level) delay_ms(3);
    
    subst_all_variables_starting_from_the_back(expr->input_string, expr->input_string_length);    
    subst_all_functions_starting_from_the_back(expr->input_string, expr->input_string_length);

    int32_t result = expression(expr->input_string);

    expr->return_value = result;

    if (debug_level) printf("Result: %d\r\n", result);
    if (debug_level) delay_ms(3);
    
    if (debug_level) printf("Result String: \"%s\"\r\n", &expr->output_string[GRID_EXPR_OUTPUT_STRING_MAXLENGTH-expr->output_string_length]);
    if (debug_level) delay_ms(3);
    
}
  
  
int* e_param_list = 0;
int  e_param_list_length = 0;  


int* p_param_list = 0;
int  p_param_list_length = 0;  


//https://stackoverflow.com/questions/9329406/evaluating-arithmetic-expressions-from-string-in-c
char peek(char** e)
{
    return **e;
}

char peek2(char** e)
{
    return *((*e)+1);
}


char get(char** e)
{
    char ret = **e;
    ++*e;
    return ret;
}

int number(char** e)
{
    int result = get(e) - '0';
    while (peek(e) >= '0' && peek(e) <= '9') // HEX para
    {
        result = 10*result + get(e) - '0'; // HEX para
    }
    return result;
}

int expr_level_3(char** e) // factor
{
    if (peek(e) >= '0' && peek(e) <= '9') // HEX para
        return number(e);
    else if (peek(e) == '(')
    {
        get(e); // '('
        int result = expr_level_0(e);
        get(e); // ')'
        return result;
    }
    else if (peek(e) == '-')
    {
        get(e);
        return -expr_level_3(e);
    }
    printf("ERROR in expr_level_3()\n");
    return 0; // error
}

int expr_level_2(char ** e) // term
{
    int result = expr_level_3(e);
    while (peek(e) == '*' || peek(e) == '/' || peek(e) == '%'){
    
        char peeked = get(e);
        
        if (peeked == '*'){
            result *= expr_level_3(e);
        }
        else if (peeked == '%'){
            result %= expr_level_3(e);
        }
        else{
            result /= expr_level_3(e);
            
        }
    }
    return result;
}

int expr_level_1(char ** e) // equality
{
    int result = expr_level_2(e);
    while (peek(e) == '+' || peek(e) == '-')
        if (get(e) == '+')
            result += expr_level_2(e);
        else
            result -= expr_level_2(e);
    return result;
}

int expr_level_0(char ** e) // equality
{
    int result = expr_level_1(e);
    
    
    while (     (peek(e) == '>' && peek2(e) != '=') || 
                (peek(e) == '<' && peek2(e) != '=') || 
                (peek(e) == '=' && peek2(e) == '=') ||
                (peek(e) == '!' && peek2(e) == '=') ||
                (peek(e) == '>' && peek2(e) == '=') ||
                (peek(e) == '<' && peek2(e) == '=') 
            ){
        
        char peeked = get(e);
        char peeked2 = peek(e);

        if ((peeked == '>' && peeked2 != '=')){
            result = (result>expr_level_1(e));
        }
        else if (peeked == '<' && peeked2 != '='){
            result = (result<expr_level_1(e));
        }
        else if (peeked == '=' && peeked2 == '='){
            get(e); // burn the second character
            result = (result == expr_level_1(e));
        }
        else if (peeked == '!' && peeked2 == '='){
            get(e); // burn the second character
            result = (result != expr_level_1(e));
        }
        else if (peeked == '>' && peeked2 == '='){
            get(e); // burn the second character
            result = (result >= expr_level_1(e));
        }
        else if (peeked == '<' && peeked2 == '='){
            get(e); // burn the second character
            result = (result <= expr_level_1(e));
        }
    }
    return result;
}

int expression_inner(char ** e)
{
    int result = expr_level_0(e);

    return result;
}

int expression(char * str){


    return expression_inner(&str);
}

void insertTo(char* start,int length,char* that){
    
    char ending[100] = {0};
    
    //printf("insertTo: Hova: %s Milyen hosszú helyre: %d Mit: %s\n", start, length, that);
    
    sprintf(ending,"%s",start+length);
    sprintf(start,"%s",that);
    sprintf(start+strlen(that),"%s",ending);
}

int brack_len(char* funcDesc,int maxLen){ //pl.: almafa(6*(2+2))*45
    
        // START: SUKU
    
    int nyitCount = 0;
    int zarCount = 0;
    
    for(int i=0; i<maxLen; i++){
        
        if (funcDesc[i] == '('){
            
            nyitCount++;
        }
        else if (funcDesc[i] == ')'){
            zarCount++;
            
            if (zarCount == nyitCount){
                return i+1;
            }
        }
        
    }
}



void calcSubFnc(char* startposition){

    uint8_t debug_level = 0; 

    char* fName = startposition;
    char* fNameEnd = strstr(fName,"(");


    if (debug_level) printf("FNC name: ");
    if (debug_level) delay_ms(1);

    for(uint8_t i=0; i<fNameEnd-fName; i++){

        if (debug_level) printf("%c",fName[i]);
        if (debug_level) delay_ms(1);

    }

    if (debug_level) printf("\r\n");
    if (debug_level) delay_ms(1);
    
    int max_offset = brack_len(fNameEnd,strlen(fNameEnd)) -2;
    
    if (debug_level) printf("calcSubFnc Maxoffset: %d  ## \r\n", max_offset);
    if (debug_level) delay_ms(5);;
    
    int param_expr_results[10] = {0};

    int param_expr_results_count = 0;
    
    char* start = fNameEnd+1;
 

    
    char* comma = strstr(start, ",");
    int commaoffset = -1;
    
    
    
    for (int i=0; i<max_offset; i=i){
        
        
        int commaoffset = -1;
        
        for(int j=i; j<max_offset; j++){
            
            if (start[j] == ','){
                commaoffset = j;
                break;
            }
        }
        
 
        if (commaoffset==-1){
            
           // printf("No more commas! \r\n");
            
            char param_expr[20] = {0};
            
            for (int j=0; j<(max_offset-i); j++){
                param_expr[j] = start[i+j];
                
                
            }
            
            if (debug_level) printf("Parameter: \"%s\", ", param_expr);
            if (debug_level) delay_ms(2);
            
            param_expr_results[param_expr_results_count] = expression(param_expr);
            
            
            if (debug_level) printf("Result: \"%d\" \r\n", param_expr_results[param_expr_results_count]);
            if (debug_level) delay_ms(2);
            
            param_expr_results_count++;
            
            
            
            break;
        }
        else{
            //printf("Commaoffset : %d: %d!  ", i, commaoffset);
            
            char param_expr[20] = {0};
            
            for (int j=0; j<commaoffset-i; j++){
                param_expr[j] = start[i+j];
                
            }
            
            if (debug_level) printf("Parameter: \"%s\" , ", param_expr);
            if (debug_level) delay_ms(2);
       
            
            param_expr_results[param_expr_results_count] = expression(param_expr);
            
            
            if (debug_level) printf("Result: \"%d\" \r\n", param_expr_results[param_expr_results_count]);
            if (debug_level) delay_ms(2);
            
            param_expr_results_count++;
            
            i=commaoffset+1;
            
        }
    }
    
    
    
    int resultOfFnc = 0;
    
    
    // START: CALC BUILTIN


    char justName[10] = {0};
    
    for (int i=0; i<9; i++){
        
        if (fName[i] == '('){
            //justName[i] == '(';
            break;
        }
        else{
            justName[i] = fName[i];
        }
        
    }

    
    if(strcmp(justName,"abs")==0){
        resultOfFnc = abs(param_expr_results[0]);
    }
    else if(strcmp(justName,"six")==0){
        resultOfFnc = 666666;
    }
    else if(strcmp(justName,"add")==0){
        resultOfFnc = param_expr_results[0] + param_expr_results[1];
    }
    else if(strcmp(justName,"print")==0 || strcmp(justName,"p")==0 ){

        //printf("print param count %d\r\n", param_expr_results_count);

        char fmt_str[] = "%02x";
        
        //printf("print_length: %d (%c)", param_expr_results[1], param_expr_results[1]+'0');

        if (param_expr_results_count>1){

            if (param_expr_results[param_expr_results_count-1]<=8){

                fmt_str[2] = param_expr_results[param_expr_results_count-1]+'0';
            }
            else{

                fmt_str[2] = 8+'0';

            }

        }

        
        uint8_t temp_array[20] = {0};
        uint8_t temp_array_length = 0;

        // print only the first param, maybe enable multiple params later!!
        sprintf(temp_array, fmt_str, param_expr_results[0]);

        if (param_expr_results_count>1){

            temp_array_length = param_expr_results[param_expr_results_count-1];
        }
        else{

            temp_array_length = 2; // default print length
        }

        struct grid_expr_model* expr = &grid_expr_state;

        for (uint8_t i=0; i<temp_array_length; i++){

            expr->output_string[GRID_EXPR_OUTPUT_STRING_MAXLENGTH-expr->output_string_length-temp_array_length+i] = temp_array[i];

        }

        expr->output_string_length += temp_array_length;


        resultOfFnc = param_expr_results[0];
    }
    else if(strcmp(justName,"if")==0){
        
        if (param_expr_results[0]){
            resultOfFnc = param_expr_results[1];
        }else{
            resultOfFnc = param_expr_results[2];
        }
        
    }
    else{
        printf("Function \"%s\" not found!\n", justName);
        resultOfFnc = 0;
        
    }    
    
    // END;
    
    
    
    //printf("resultOfFnc: %d\n", resultOfFnc);
    

    
    char buff[100] = {0};
    
    sprintf(buff,"(%d)",resultOfFnc); //HEX para, sign para
    
    // hova, milyen hosszan, mit
    insertTo(startposition,(fNameEnd-fName)+max_offset+2,buff);
    
        
    //printf(" @@ debug: %s @@\n", startposition);
}


uint8_t char_is_valid_name(uint8_t ch){


    if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_')){

                    return 1;
                }
    
    return 0;


}


void subst_all_variables_starting_from_the_back(char* expr_string, int len){


    uint8_t debug_level = 0;
    
    uint8_t function_name_found = 0;
    uint8_t variable_name_found = 0;
    uint8_t variable_name_valid = 0;
    

    int izgi = 0;
    int var_end_pos = -1;
    char var_name[10] = {0};
    
    if (debug_level) printf("Subst Vars\r\n");
    
    // i must be signed int
    for(int i = len; i>=0; i--){
        

        if (function_name_found){
             
            // HEX para
            if (char_is_valid_name(expr_string[i])){
                //továbbra is para van mert ez funtion
            }
            else {
                function_name_found=0;
            }
            
            
        }
        else if (variable_name_found == 0){
            // HEX para
            if  (char_is_valid_name(expr_string[i])){
                
                if (expr_string[i+1] == '('){
                    
                    variable_name_found = 0;
                    function_name_found = 1;                    
                    
                }
                else{
                    
                    if ((expr_string[i] >= '0' && expr_string[i] <= '9')){
                        
                        variable_name_found = 1;                   
                        var_end_pos = i;
                    }
                    else{
                        // nem csak szám van benne szóval fasz
                        variable_name_found = 1;
                        variable_name_valid = 1; 
                        var_end_pos = i; 
                        
                    }                  
 
                    
                }
                
            }
            
        }

        if (variable_name_found){

            if (char_is_valid_name(expr_string[i])){
                
                
                var_name[var_end_pos-i] = expr_string[i];
                
                
                if ((expr_string[i] >= '0' && expr_string[i] <= '9')){
                    //variable_name_valid = 0; //pl nem valid a 0variable12
                }
                else{
                    // nem csak szám van benne szóval fasza
                    variable_name_valid = 1;
                }

                
            }

            if (variable_name_valid){

                uint8_t test = 0;

                if (i==0){
                    test = 1;
                    
                }
                else{
                    if (!char_is_valid_name(expr_string[i-1])){
                        test = 1;
                    }
                }

                if (test){


                    int var_name_len = strlen(var_name);
                    
                    // need to reverse the variable name string
                    char var_name_good[10] = {0};
        
                    for (int j = 0; j<var_name_len; j++){
                        var_name_good[j] = var_name[var_name_len-1-j];
                        var_name_good[j+1] = 0;
                    }
                    
                    if (debug_level) printf("Variable \"%s\" found!\r\n", var_name_good);
                    
                    // TODO: Find variable registered in a list or something. Now var is always 1
                    int32_t variable_value = 1;
                    
                    // T TEST
                    if (var_name_len == 2 || var_name_len == 3){
                        
                        if (var_name_good[0] == 'T'){
                            
                            uint8_t is_template_var = 1;
                            uint8_t index = 0;

                            for (uint8_t j = 1; j<var_name_len; j++){

                                if (var_name_good[j] >= '0' && var_name_good[j] <= '9' ){

                                    // all good
                                    index = index*10;
                                    index += var_name_good[j] - '0'; 

                                }
                                else{

                                    is_template_var = 0;

                                }

                            }

                            if (is_template_var){

                                variable_value = grid_expr_state.current_event->parent->template_parameter_list[index];

                                if (debug_level) printf(" ## var dump:  %d ", variable_value);

                                for(uint8_t j=0; j<32; j++){
                                    if (debug_level) printf("%d",(variable_value>>(31-j))&1);

                                }

                                if (debug_level) printf(" \r\n", variable_value);
                            
                            }
                            
                        }
                        else if (var_name_good[0] == 'Z'){
                            
                            uint8_t is_template_var = 1;
                            uint8_t index = 0;

                            for (uint8_t j = 1; j<var_name_len; j++){

                                if (var_name_good[j] >= '0' && var_name_good[j] <= '9' ){

                                    // all good
                                    index = index*10;
                                    index += var_name_good[j] - '0'; 

                                }
                                else{

                                    is_template_var = 0;

                                }

                            }

                            if (is_template_var){

                                if (index == 0){
                                    variable_value = grid_sys_get_bank_num(&grid_sys_state);
                                }
                                else if (index == 1){
                                    variable_value = grid_sys_get_bank_red(&grid_sys_state);
                                }
                                else if (index == 2){
                                    variable_value = grid_sys_get_bank_gre(&grid_sys_state);
                                }
                                else if (index == 3){
                                    variable_value = grid_sys_get_bank_blu(&grid_sys_state);
                                }
                                else if (index == 4){
                                    variable_value = grid_sys_get_map_state(&grid_sys_state);
                                }
                                else if (index == 5){
                                    variable_value = grid_sys_get_bank_next(&grid_sys_state);
                                }
                            
                            }
                            
                        }
                    }
                    

                    char* found = &expr_string[i]; // i+1 helyen lesz mindenképpen!
                    
                    char buff[100] = {0};
                    
                    sprintf(buff,"%d",variable_value); // HEX para
                    
                    // hova, milyen hosszú helyre, mit
                    insertTo(found,var_name_len,buff);
                    variable_name_found = 0;
                    variable_name_valid = 0;
                    
                    if (debug_level) printf("%s\r\n", expr_string);

                    for (int j = 0; j<10; j++){
                        var_name[j] = 0;
                    }

                    
                    variable_name_found = 0;
                    variable_name_valid = 0;
                

                    for (int j = 0; j<10; j++){
                        var_name[j] = 0;
                    }

                }

            }   

            if (!char_is_valid_name(expr_string[i])){
                
                variable_name_found = 0;
                variable_name_valid = 0;
            

                for (int j = 0; j<10; j++){
                    var_name[j] = 0;
                }

                
            }         


       
        }else{

            variable_name_found = 0;
            variable_name_valid = 0;
        

            for (int j = 0; j<10; j++){
                var_name[j] = 0;
            }
        }
   
    if (debug_level ==2) printf("i%d %d %d %d\r\n",i, function_name_found, variable_name_found, variable_name_valid);
    if (debug_level ==2) delay_ms(5);
        
    }
    
}



void subst_all_functions_starting_from_the_back(char* expr_string, int len){
    

    uint8_t debug_level = 0;   


    if (debug_level) printf("Subst Fncs in %s\r\n", expr_string);
    if (debug_level) delay_ms(5);
    
    uint8_t function_name_found = 0;
    uint8_t function_name_valid = 0;

    // i must be signed int!!!!
    for(int i= len; i>=0; i--){
        
        if (expr_string[i] == '(' && function_name_valid == 0){

            function_name_found = 1;
            function_name_valid = 0;
                //printf("izgi=%d, i=%d\n", izgi, i);  

            
        }
        else if (function_name_found){
            
            if (char_is_valid_name(expr_string[i])){
                
                if ((expr_string[i] >= '0' && expr_string[i] <= '9')){
                    
                }
                else{
                    // nem csak szám van benne szóval fasz
                    function_name_valid = 1;
                    
                }
                
                
                if (i==0 && function_name_valid){ // start of expr string special case
                    calcSubFnc(&expr_string[i]);

                    function_name_valid = 0;
                    function_name_found = 0;
                    
                }
                //printf("izgi=%d, i=%d\n", izgi, i);  
                
            }
            else if (function_name_valid){
                
                calcSubFnc(&expr_string[i+1]);
  
                function_name_valid = 0;
                function_name_found = (expr_string[i] == '(');
                //printf("izgi=%d, i=%d\n", izgi, i);  
                
            }
            else{
                
                function_name_valid = 0;
                function_name_found = 0;
                //printf("izgi=%d, i=%d\n", izgi, i);  
                //printf("mégsem");
                
            }
            
        }

        if (debug_level) printf("i%d %d %d\r\n",i, function_name_found, function_name_valid);
        if (debug_level) delay_ms(1);
    }
    
}


