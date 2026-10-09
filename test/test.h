#ifndef TEST_H
#define TEST_H
#include <stdio.h>
#include <stdlib.h>

#define test_assert(expr, test_name, err_message) (printf(test_name ": "), (expr) ? printf("OK\n") : printf("FAIL\n>   " err_message "\n")) 
#define test_assert_exit(expr, test_name, err_message) (printf(test_name ": "), (expr) ? printf("OK\n") : (printf("FAIL\n>    " err_message "\n"), exit(EXIT_FAILURE))) 





#endif

