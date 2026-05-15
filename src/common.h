#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ID_LEN 20
#define NAME_LEN 64
#define PHONE_LEN 20
#define DATE_LEN 16
#define TIME_LEN 32
#define STATUS_LEN 24
#define TEXT_LEN 256
#define LINE_LEN 512
#define RESULT_LEN 30000

#define OK 0
#define ERR_INPUT 1
#define ERR_REPEAT 2
#define ERR_NOT_FOUND 3
#define ERR_NO_SPACE 4
#define ERR_LINKED 5
#define ERR_FILE 6

void safeCopy(char *to, int size, const char *from);
void clearText(char *out, int outSize);
void appendText(char *out, int outSize, const char *text);
void appendLine(char *out, int outSize, const char *text);
int isBlank(const char *s);
int hasBadChar(const char *s);

int checkName(const char *s);
int checkGender(const char *s);
int checkAge(int age);
int checkPhone(const char *s);
int checkCardId(const char *s);
int checkDate(const char *s);
int checkMoney(double money);
int checkCount(int count);
int checkMemo(const char *s);
int parseIntStrict(const char *s, int *value);
int parseMoneyStrict(const char *s, double *value);
int compareDate(const char *a, const char *b);
int daysBetween(const char *beginDate, const char *endDate);

int getIdNumber(const char *id, const char *prefix);
void makeId(char *out, int outSize, const char *prefix, int number);
const char *codeText(int code);

#ifdef __cplusplus
}
#endif

#endif
