#ifndef TEST_H
#define TEST_H

#ifdef __cplusplus
extern "C" {
#endif

void runInputTests(char *out, int outSize);
void runFlowTests(char *out, int outSize);
void runFileTests(char *out, int outSize);
void runReportTests(char *out, int outSize);
void runAllTests(char *out, int outSize);

#ifdef __cplusplus
}
#endif

#endif
