#include <stdio.h>

#include "test_support.h"

static unsigned fails = 0;

unsigned get_number_of_failures(void)
{
	return fails;
}

void assert_fail(const char* cond, unsigned line)
{
	fails++;
	printf("** Assert(%s) failed at line %u\n", cond, line);
}

