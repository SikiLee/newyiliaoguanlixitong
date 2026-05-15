#include "common.h"

void safeCopy(char *to, int size, const char *from)
{
    if (to == NULL || size <= 0) {
        return;
    }
    if (from == NULL) {
        to[0] = '\0';
        return;
    }
    strncpy(to, from, size - 1);
    to[size - 1] = '\0';
}

void clearText(char *out, int outSize)
{
    if (out != NULL && outSize > 0) {
        out[0] = '\0';
    }
}

void appendText(char *out, int outSize, const char *text)
{
    int left;
    if (out == NULL || text == NULL || outSize <= 0) {
        return;
    }
    left = outSize - (int)strlen(out) - 1;
    if (left <= 0) {
        return;
    }
    strncat(out, text, left);
}

void appendLine(char *out, int outSize, const char *text)
{
    appendText(out, outSize, text);
    appendText(out, outSize, "\n");
}

int isBlank(const char *s)
{
    if (s == NULL) {
        return 1;
    }
    while (*s != '\0') {
        if (!isspace((unsigned char)*s)) {
            return 0;
        }
        s++;
    }
    return 1;
}

int hasBadChar(const char *s)
{
    if (s == NULL) {
        return 1;
    }
    while (*s != '\0') {
        if (*s == '|' || *s == '\n' || *s == '\r') {
            return 1;
        }
        s++;
    }
    return 0;
}

int checkName(const char *s)
{
    int i;
    int len = 0;
    if (isBlank(s) || hasBadChar(s)) {
        return 0;
    }
    for (i = 0; s[i] != '\0'; i++) {
        if (((unsigned char)s[i] & 0xC0) != 0x80) {
            len++;
        }
    }
    return len >= 2 && len <= 20;
}

int checkGender(const char *s)
{
    return strcmp(s, "男") == 0 || strcmp(s, "女") == 0;
}

int checkAge(int age)
{
    return age >= 0 && age <= 120;
}

int checkPhone(const char *s)
{
    int i;
    if (s == NULL || strlen(s) != 11) {
        return 0;
    }
    for (i = 0; i < 11; i++) {
        if (!isdigit((unsigned char)s[i])) {
            return 0;
        }
    }
    return 1;
}

int checkCardId(const char *s)
{
    int i;
    if (s == NULL || strlen(s) != 18) {
        return 0;
    }
    for (i = 0; i < 17; i++) {
        if (!isdigit((unsigned char)s[i])) {
            return 0;
        }
    }
    if (!isdigit((unsigned char)s[17]) && s[17] != 'X' && s[17] != 'x') {
        return 0;
    }
    return 1;
}

static int isLeap(int year)
{
    if (year % 400 == 0) {
        return 1;
    }
    if (year % 100 == 0) {
        return 0;
    }
    return year % 4 == 0;
}

static int monthDays(int year, int month)
{
    int days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month == 2 && isLeap(year)) {
        return 29;
    }
    if (month < 1 || month > 12) {
        return 0;
    }
    return days[month - 1];
}

static int splitDate(const char *s, int *year, int *month, int *day)
{
    int y, m, d;
    char c1, c2;
    if (s == NULL || strlen(s) != 10) {
        return 0;
    }
    if (sscanf(s, "%4d%c%2d%c%2d", &y, &c1, &m, &c2, &d) != 5) {
        return 0;
    }
    if (c1 != '-' || c2 != '-') {
        return 0;
    }
    *year = y;
    *month = m;
    *day = d;
    return 1;
}

int checkDate(const char *s)
{
    int y, m, d;
    if (!splitDate(s, &y, &m, &d)) {
        return 0;
    }
    if (y < 1900 || y > 2100 || m < 1 || m > 12) {
        return 0;
    }
    return d >= 1 && d <= monthDays(y, m);
}

int checkMoney(double money)
{
    return money >= 0 && money <= 999999.99;
}

int checkCount(int count)
{
    return count >= 1 && count <= 9999;
}

int checkMemo(const char *s)
{
    int i;
    int len = 0;
    int hasCjk = 0;
    if (isBlank(s) || hasBadChar(s)) {
        return 0;
    }
    for (i = 0; s[i] != '\0'; i++) {
        if (((unsigned char)s[i] & 0x80) != 0) {
            hasCjk = 1;
        }
        if (((unsigned char)s[i] & 0xC0) != 0x80) {
            len++;
        }
    }
    return hasCjk && len >= 1 && len <= TEXT_LEN - 1;
}

int parseIntStrict(const char *s, int *value)
{
    int i;
    int v = 0;
    if (isBlank(s)) {
        return 0;
    }
    for (i = 0; s[i] != '\0'; i++) {
        if (!isdigit((unsigned char)s[i])) {
            return 0;
        }
        v = v * 10 + s[i] - '0';
    }
    *value = v;
    return 1;
}

int parseMoneyStrict(const char *s, double *value)
{
    int i;
    int dotCount = 0;
    int dotPos = -1;
    double v;
    char tail[8];
    if (isBlank(s)) {
        return 0;
    }
    for (i = 0; s[i] != '\0'; i++) {
        if (s[i] == '.') {
            dotCount++;
            dotPos = i;
        } else if (!isdigit((unsigned char)s[i])) {
            return 0;
        }
    }
    if (dotCount > 1) {
        return 0;
    }
    if (dotPos >= 0) {
        safeCopy(tail, sizeof(tail), s + dotPos + 1);
        if ((int)strlen(tail) > 2) {
            return 0;
        }
    }
    v = atof(s);
    if (!checkMoney(v)) {
        return 0;
    }
    *value = v;
    return 1;
}

int compareDate(const char *a, const char *b)
{
    return strcmp(a, b);
}

static int daysFromStart(int year, int month, int day)
{
    int y, m;
    int sum = 0;
    for (y = 1900; y < year; y++) {
        sum += isLeap(y) ? 366 : 365;
    }
    for (m = 1; m < month; m++) {
        sum += monthDays(year, m);
    }
    return sum + day;
}

int daysBetween(const char *beginDate, const char *endDate)
{
    int y1, m1, d1, y2, m2, d2;
    int dayCount;
    if (!checkDate(beginDate) || !checkDate(endDate)) {
        return -1;
    }
    splitDate(beginDate, &y1, &m1, &d1);
    splitDate(endDate, &y2, &m2, &d2);
    dayCount = daysFromStart(y2, m2, d2) - daysFromStart(y1, m1, d1);
    if (dayCount < 0) {
        return -1;
    }
    if (dayCount == 0) {
        return 1;
    }
    return dayCount;
}

int getIdNumber(const char *id, const char *prefix)
{
    int len;
    if (id == NULL || prefix == NULL) {
        return 0;
    }
    len = (int)strlen(prefix);
    if (strncmp(id, prefix, len) != 0) {
        return 0;
    }
    return atoi(id + len);
}

void makeId(char *out, int outSize, const char *prefix, int number)
{
    snprintf(out, outSize, "%s%04d", prefix, number);
}

const char *codeText(int code)
{
    if (code == OK) return "成功";
    if (code == ERR_INPUT) return "输入不符合限制";
    if (code == ERR_REPEAT) return "数据重复";
    if (code == ERR_NOT_FOUND) return "没有找到数据";
    if (code == ERR_NO_SPACE) return "没有可用数量或床位";
    if (code == ERR_LINKED) return "已有业务关联，不能删除";
    if (code == ERR_FILE) return "文件读写错误";
    return "未知错误";
}
