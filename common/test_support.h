#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#define ASSERT(T) { if (!(T)) assert_fail(#T, __LINE__); }

	// report a failure of given condition at line.
	extern void assert_fail(const char* cond, unsigned line);

	extern unsigned get_number_of_failures(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_SUPPORT_H
