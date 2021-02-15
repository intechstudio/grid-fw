/******************************************************************************

                            Online C Compiler.
                Code, Compile, Run and Debug C program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
  
//https://www.geeksforgeeks.org/c-program-replace-word-text-another-given-word/
// Function to replace a string with another 
// string 
char* replaceWord(const char* s, const char* oldW, 
                  const char* newW) 
{ 
    char* result; 
    int i, cnt = 0; 
    int newWlen = strlen(newW); 
    int oldWlen = strlen(oldW); 
  
    // Counting the number of times old word 
    // occur in the string 
    for (i = 0; s[i] != '\0'; i++) { 
        if (strstr(&s[i], oldW) == &s[i]) { 
            cnt++; 
  
            // Jumping to index after the old word. 
            i += oldWlen - 1; 
        } 
    } 
  
    // Making new string of enough length 
    result = (char*)malloc(i + cnt * (newWlen - oldWlen) + 1); 
  
    i = 0; 
    while (*s) { 
        // compare the substring with the result 
        if (strstr(s, oldW) == s) { 
            strcpy(&result[i], newW); 
            i += newWlen; 
            s += oldWlen; 
        } 
        else
            result[i++] = *s++; 
    } 
  
    result[i] = '\0'; 
    return result; 
} 

//https://stackoverflow.com/questions/9329406/evaluating-arithmetic-expressions-from-string-in-c
char peek(char** e)
{
    return **e;
}

char get(char** e)
{
    char ret = **e;
    ++*e;
    return ret;
}

int expression_inner();

int number(char** e)
{
    int result = get(e) - '0';
    while (peek(e) >= '0' && peek(e) <= '9')
    {
        result = 10*result + get(e) - '0';
    }
    return result;
}

int factor(char** e)
{
    if (peek(e) >= '0' && peek(e) <= '9')
        return number(e);
    else if (peek(e) == '(')
    {
        get(e); // '('
        int result = expression_inner(e);
        get(e); // ')'
        return result;
    }
    else if (peek(e) == '-')
    {
        get(e);
        return -factor(e);
    }
    return 0; // error
}

int term(char ** e)
{
    int result = factor(e);
    while (peek(e) == '*' || peek(e) == '/')
        if (get(e) == '*')
            result *= factor(e);
        else
            result /= factor(e);
    return result;
}

int expression_inner(char ** e)
{
    int result = term(e);
    while (peek(e) == '+' || peek(e) == '-')
        if (get(e) == '+')
            result += term(e);
        else
            result -= term(e);
    return result;
}

int expression(char * fos){
    return expression_inner(&fos);
}

int alma(char** a){
    ++(*a);
}

void insertTo(char* found,int foundLen,char* that){
    char ending[100] = {0};
    
    sprintf(ending,"%s",found+foundLen);
    sprintf(found,"%s",that);
    sprintf(found+strlen(that),"%s",ending);
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

//fuggvennyel ne hivjad csak ha calcSubFnc
void subst(char* str,char* vName,int v){
    int nameLen = strlen(vName);
    
    char* found = strstr(str,vName);
    
    //printf("Hossz: %d\n", nameLen);
    
    // END: SUKU
    
    char buff[100] = {0};
    
    sprintf(buff,"%d",v);
    
    // hova, milyen hosszú helyre, mit
    insertTo(found,nameLen,buff);

}

void substFn(char* str,char* vName,int v){
    int nameLen = strlen(vName);
    
    char* found = strstr(str,vName);
    char* firstBrack = strstr(found,"(");
    if(firstBrack){
         //calc closing of found[nameLen] == '('
        // nameLen értéke legyen a pl abs(2*(4-1)*6) nal 14
        
        nameLen = brack_len(found,strlen(found));
    }
    //printf("Hossz: %d\n", nameLen);
    
    // END: SUKU
    
    char buff[100] = {0};
    
    sprintf(buff,"(%d)",v);
    
    insertTo(found,nameLen,buff);
    
        
    printf(" @@ debug: %s @@\n", str);
}




void calcSubFnc(char* str, int funcpos){
    char* fName = str + funcpos;
    char* fNameEnd = strstr(fName,"(");
    
    int max_offset = brack_len(fNameEnd,strlen(fNameEnd)) - (fNameEnd-fName) -1;
    

    
    int param_expr_results[10] = {0};

    int param_expr_results_count = 0;
    
    char* start = fNameEnd+1;
    
    char* comma = strstr(start, ",");
    int commaoffset = -1;
    
    
    //printf("Maxoffset: %d\n", max_offset);
    
    for (int i=0; i<max_offset; i=i){
        
        
        int commaoffset = -1;
        
        for(int j=i; j<max_offset; j++){
            
            if (start[j] == ','){
                commaoffset = j;
                break;
            }
        }
        
 
        if (commaoffset==-1){
            
            printf("No more commas! ");
            
            char param_expr[20] = {0};
            
            for (int j=0; j<(max_offset); j++){
                param_expr[j] = start[j];
                
                
            }
            
            printf("Parameter: \"%s\", ", param_expr);
        
            
            param_expr_results[param_expr_results_count] = expression(param_expr);
            
            
            printf("Result: \"%d\" \n", param_expr_results[param_expr_results_count]);
            param_expr_results_count++;
            
            
            
            break;
        }
        else{
            printf("Commaoffset : %d: %d!  ", i, commaoffset);
            
            char param_expr[20] = {0};
            
            for (int j=0; j<commaoffset; j++){
                param_expr[j] = start[i+j];
                
            }
            
            printf("Parameter: \"%s\" , ", param_expr);
       
            
            param_expr_results[param_expr_results_count] = expression(param_expr);
            
            
            printf("Result: \"%d\" \n", param_expr_results[param_expr_results_count]);
            param_expr_results_count++;
            
            i+=commaoffset+1;
            start+=commaoffset+1;
            
        }
    }
    
    
    
    int resultOfFnc = 0;
    
    
    // START: CALC BUILTIN


    char justName[10] = {0};
    
    for (int i=0; i<9; i++){
        
        if (fName[i] == '('){
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
        printf(" ## add: resultOfFnc: %d ##\n", resultOfFnc);
    }
    else{
        printf("Function \"%s\" not found!\n", justName);
        resultOfFnc = 0;
        
    }    
    
    // END;
    
    
    
    printf("resultOfFnc: %d\n", resultOfFnc);
    
    substFn(str,fName,resultOfFnc);
}

void subst_all_variables_starting_from_the_back(char* expr_string, int len){
    
    
    int izgi = 0;
    int var_end_pos = -1;
    char var_name[10] = {0};
    
    printf("Subst Vars\n");
    for(int i= len; i!=0; i--){
        
        
        if (izgi == -1){
            
            
            if ((expr_string[i] >= '0' && expr_string[i] <= '9') || 
                (expr_string[i] >= 'a' && expr_string[i] <= 'z') || 
                (expr_string[i] >= 'A' && expr_string[i] <= 'Z') || 
                (expr_string[i] == '_')){
                //továbbra is para van mert ez funtion
            }
            else {
                izgi=0;
                //printf("izgi=%d, i=%d\n", izgi, i);
            }
            
            
        }
        
        if (izgi == 0){
            
            if  ((expr_string[i] >= '0' && expr_string[i] <= '9') || 
                (expr_string[i] >= 'a' && expr_string[i] <= 'z') || 
                (expr_string[i] >= 'A' && expr_string[i] <= 'Z') || 
                (expr_string[i] == '_')){
                
                if (expr_string[i+1] == '('){
                    
                    izgi = -1;
                    
                    //printf("izgi=%d, i=%d\n", izgi, i);
                    
                    
                }
                else{
                    
                    if ((expr_string[i] >= '0' && expr_string[i] <= '9')){
                        
                        izgi = 1;
                        //printf("izgi=%d, i=%d\n", izgi, i);
                        var_end_pos = i;
                        var_name[var_end_pos-i] = expr_string[i];                     
                        
                    }
                    else{
                        // nem csak szám van benne szóval fasz
                        izgi = 2;
                        //printf("izgi=%d, i=%d\n", izgi, i);
                        var_end_pos = i;
                        var_name[var_end_pos-i] = expr_string[i];   
                        
                    }                  
 
                    
                }
                
            }
            
        }
        else if (izgi == 1 || izgi == 2){
            
            if ((expr_string[i] >= '0' && expr_string[i] <= '9') || 
            (expr_string[i] >= 'a' && expr_string[i] <= 'z') || 
            (expr_string[i] >= 'A' && expr_string[i] <= 'Z') || 
            (expr_string[i] == '_')){
                
                
                var_name[var_end_pos-i] = expr_string[i];
                
                if ((expr_string[i] >= '0' && expr_string[i] <= '9')){
                    
                    
                    
                }
                else{
                    // nem csak szám van benne szóval fasz
                    izgi = 2;
                
                //    printf("izgi=%d, i=%d\n", izgi, i);
                    
                }

                
            }
            else if (izgi==2){
                

                int var_name_len = strlen(var_name);
                
                char var_name_good[10] = {0};
    
                // printf("Variable \"%s\" found!\n", var_name);
                // printf("Length \"%d\" found!\n", var_name_len);
                 
                for (int j = 0; j<var_name_len; j++){
                    var_name_good[j] = var_name[var_name_len-1-j];
                    
                    //printf("%d: %c\n",j,var_name[var_name_len-1-j]);
                    var_name_good[j+1] = 0;
                }
                
                
                
                printf("Variable \"%s\" found!\n", var_name_good);
                //calcSubFnc(expr_string,i+1);
                
                subst(&expr_string[i], var_name_good,1);

                
                izgi = 0;
                //printf("izgi=%d, i=%d\n", izgi, i);
                
            }
            else{
                
                izgi = 0;
                for (int j = 0; j<10; j++){
                    var_name[j] = 0;
                }
                
            }
            
        }
        
        
        
    }
    
}


void subst_all_functions_starting_from_the_back(char* expr_string, int len){
    
    
    int izgi = 0;
    
    
    
    printf("Subst Fncs\n");
    for(int i= len; i!=0; i--){
        
        if (izgi == 0){
            
            if (expr_string[i] == '(' ){

                izgi = 1;
                //printf("izgi=%d, i=%d\n", izgi, i);  
                
            }
            
        }
        else if (izgi == 1 || izgi == 2){
            
            if ((expr_string[i] >= '0' && expr_string[i] <= '9') || 
            (expr_string[i] >= 'a' && expr_string[i] <= 'z') || 
            (expr_string[i] >= 'A' && expr_string[i] <= 'Z') || 
            (expr_string[i] == '_')){
                
                izgi = 2;
                
                //printf("izgi=%d, i=%d\n", izgi, i);
                
            }
            else if (izgi==2){
                
                calcSubFnc(expr_string,i+1);
                
                
                
                izgi = 0;
                //printf("izgi=%d, i=%d\n", izgi, i);
                
            }
            
        }
        
        
        
    }
    
}


int main()
{
    /*
    {
        char exprArr[100] = {0};
        sprintf(exprArr,"(3*3-(2*5))");
        printf("%i\n",expression(exprArr));
    }    
    {
        char exprArr[100] = {0};
        sprintf(exprArr,"1+10");
        printf("%i\n",expression(exprArr));
    }
    {
        char exprArr[100] = {0};
        sprintf(exprArr,"1+10*2)faszvagyok");
        printf("%i\n",expression(exprArr));
    }
    

    {
        char exprArr[100] = {0};
        sprintf(exprArr,"1+abs(-1)+six(5*y)+y-1");
        subst(exprArr,"y",9);
        printf("\n%s\n", exprArr);       
        
        subst_all_functions_starting_from_the_back(exprArr, strlen(exprArr));
                
        
        int ret = expression(exprArr);
        printf("%d\n",ret);
    }
    
    {
        char exprArr[100] = {0};
        sprintf(exprArr,"1+x+33");
        subst(exprArr,"x",1000);
        int ret = expression(exprArr);
        printf("%d\n",ret);
    }    
    
    {
        char exprArr[100] = {0};
        sprintf(exprArr,"1000+xanax");
        subst(exprArr,"xanax",1);
        int ret = expression(exprArr);
        printf("%d\n",ret);
    }        
    {
        char exprArr[100] = {0};
        sprintf(exprArr,"1+six(3)*2");
        
        calcSubFnc(exprArr,2);
        
        int ret = expression(exprArr);
        
        printf("%d\n",ret);
    }
    {
        char exprArr[100] = {0};
        sprintf(exprArr,"1+abs(3*3-(2*5))*2");
        
        calcSubFnc(exprArr,2);
        
        int ret = expression(exprArr);
        
        printf("%d\n",ret);
    }  
    
    {
        char exprArr[100] = {0};
        sprintf(exprArr,"1000+xanax2000");
        subst(exprArr,"xanax2000",1);
        int ret = expression(exprArr);
        printf("%d\n",ret);
    }       
    
    */
    
 {
        char exprArr[100] = {0};
        sprintf(exprArr,"10+abs(5-six(123))");
        printf("%s\n",exprArr);
        
        subst_all_variables_starting_from_the_back(exprArr, strlen(exprArr));
        
        subst_all_functions_starting_from_the_back(exprArr, strlen(exprArr));
        
        
        
        int ret = expression(exprArr);
        printf("%d\n\n",ret);
    }    
    {
        char exprArr[100] = {0};
        sprintf(exprArr,"10+y*abs(5-x-six(123))-1");
        printf("%s\n",exprArr);

        
        
        subst_all_variables_starting_from_the_back(exprArr, strlen(exprArr));
        subst_all_functions_starting_from_the_back(exprArr, strlen(exprArr));
        
        
        
        int ret = expression(exprArr);
        printf("%d\n\n",ret);
    }      
    
 {
        char exprArr[100] = {0};
        sprintf(exprArr,"10+abs(5-add(5,-10))");
        printf("%s\n",exprArr);
        
        subst_all_variables_starting_from_the_back(exprArr, strlen(exprArr));
        
        subst_all_functions_starting_from_the_back(exprArr, strlen(exprArr));
        
        
        
        int ret = expression(exprArr);
        printf("%d\n\n",ret);
    }  

    
    //printf("%i",result2);
    return 10;
}
