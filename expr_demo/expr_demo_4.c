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
    
    sprintf(buff,"%d",v);
    
    insertTo(found,nameLen,buff);
}

int calcBuiltin(char* name,int value){
    char justName[100] = {0};
    sprintf(justName,name);
    char* firstBrack = strstr(name,"(");
    char* lookUpName = name;
    if(firstBrack){
        int justNameLen = firstBrack - name;
        justName[justNameLen] = 0; 
        lookUpName = justName;
    }
    
    if(strcmp(lookUpName,"abs")==0){
        return abs(value);
    }else if(strcmp(lookUpName,"six")==0){
        return 666666;
    }
    else{
        printf("Function \"%s\" not found!\n", lookUpName);
        return 0;
        
    }
}


void calcSubFnc(char* str, int funcpos){
    char* fName = str + funcpos;
    char* fNameEnd = strstr(fName,"(");
    brack_len(fNameEnd,strlen(fNameEnd));
    int innerexpr = expression(fNameEnd+1); //genyo vagyok mer ugy veszem h csak a bezaroig parsol
    
    int resultOfFnc = calcBuiltin(fName,innerexpr);
    
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
    
    */
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
        int alma = -8;
        int ret = calcBuiltin("abs",alma);
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
 {
        char exprArr[100] = {0};
        sprintf(exprArr,"10+abs(5-six(333))");
        
        
        subst_all_variables_starting_from_the_back(exprArr, strlen(exprArr));
        
        subst_all_functions_starting_from_the_back(exprArr, strlen(exprArr));
        
        
        
        int ret = expression(exprArr);
        printf("%d\n",ret);
    }    
    {
        char exprArr[100] = {0};
        sprintf(exprArr,"10+y*abs(5-x-six(333))-1");
        //sprintf(exprArr,"1+abs(-1)+six(5*y)+y-1");
        
        
        
        printf("helloka\n");
        printf("\n%s\n", exprArr);
        //subst(exprArr,"y",10);
        
        
        
        subst_all_variables_starting_from_the_back(exprArr, strlen(exprArr));
        
        printf("\n%s\n", exprArr);
        
        subst_all_functions_starting_from_the_back(exprArr, strlen(exprArr));
        
        
        
        int ret = expression(exprArr);
        printf("%d\n",ret);
    }      

    
    //printf("%i",result2);
    return 10;
}
