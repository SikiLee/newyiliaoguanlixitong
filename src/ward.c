#include "ward.h"
#include "department.h"
#include "data.h"

void makeNextWardId(char *out, int outSize)
{
    Ward *p = wardHead;
    int max = 0;
    int num;
    while (p != NULL) {
        num = getIdNumber(p->id, "WAR");
        if (num > max) max = num;
        p = p->next;
    }
    makeId(out, outSize, "WAR", max + 1);
}

void makeNextBedId(char *out, int outSize)
{
    Bed *p = bedHead;
    int max = 0;
    int num;
    while (p != NULL) {
        num = getIdNumber(p->id, "BED");
        if (num > max) max = num;
        p = p->next;
    }
    makeId(out, outSize, "BED", max + 1);
}

Ward *findWardById(const char *id)
{
    Ward *p = wardHead;
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) return p;
        p = p->next;
    }
    return NULL;
}

Bed *findBedById(const char *id)
{
    Bed *p = bedHead;
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) return p;
        p = p->next;
    }
    return NULL;
}

int addWard(const char *id, const char *type, const char *deptId, double dailyFee,
    int totalBeds, int freeBeds)
{
    Ward *node;
    Ward *p;
    if (isBlank(id) || isBlank(type) || hasBadChar(type) || findDepartmentById(deptId) == NULL ||
        !checkMoney(dailyFee) || totalBeds < 1 || freeBeds < 0 || freeBeds > totalBeds) {
        return ERR_INPUT;
    }
    if (findWardById(id) != NULL) return ERR_REPEAT;
    node = (Ward *)malloc(sizeof(Ward));
    if (node == NULL) return ERR_FILE;
    safeCopy(node->id, ID_LEN, id);
    safeCopy(node->type, NAME_LEN, type);
    safeCopy(node->deptId, ID_LEN, deptId);
    node->dailyFee = dailyFee;
    node->totalBeds = totalBeds;
    node->freeBeds = freeBeds;
    node->next = NULL;
    if (wardHead == NULL) {
        wardHead = node;
    } else {
        p = wardHead;
        while (p->next != NULL) p = p->next;
        p->next = node;
    }
    return OK;
}

int updateWard(const char *id, double dailyFee)
{
    Ward *p = findWardById(id);
    if (p == NULL) return ERR_NOT_FOUND;
    if (!checkMoney(dailyFee)) return ERR_INPUT;
    p->dailyFee = dailyFee;
    return OK;
}

static int wardHasUsedBed(const char *id)
{
    Bed *b = bedHead;
    while (b != NULL) {
        if (strcmp(b->wardId, id) == 0) return 1;
        b = b->next;
    }
    return 0;
}

int deleteWard(const char *id)
{
    Ward *p = wardHead;
    Ward *pre = NULL;
    if (wardHasUsedBed(id)) return ERR_LINKED;
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) {
            if (pre == NULL) wardHead = p->next;
            else pre->next = p->next;
            free(p);
            return OK;
        }
        pre = p;
        p = p->next;
    }
    return ERR_NOT_FOUND;
}

int addBed(const char *id, const char *wardId, const char *status, const char *patientId,
    const char *inDate)
{
    Bed *node;
    Bed *p;
    Ward *ward = findWardById(wardId);
    if (isBlank(id) || ward == NULL || isBlank(status) || hasBadChar(status)) {
        return ERR_INPUT;
    }
    if (strcmp(status, "空闲") != 0 && strcmp(status, "占用") != 0) {
        return ERR_INPUT;
    }
    if (strcmp(status, "占用") == 0 && (isBlank(patientId) || !checkDate(inDate))) {
        return ERR_INPUT;
    }
    if (findBedById(id) != NULL) return ERR_REPEAT;
    node = (Bed *)malloc(sizeof(Bed));
    if (node == NULL) return ERR_FILE;
    safeCopy(node->id, ID_LEN, id);
    safeCopy(node->wardId, ID_LEN, wardId);
    safeCopy(node->status, STATUS_LEN, status);
    safeCopy(node->patientId, ID_LEN, patientId);
    safeCopy(node->inDate, DATE_LEN, inDate);
    node->next = NULL;
    if (bedHead == NULL) {
        bedHead = node;
    } else {
        p = bedHead;
        while (p->next != NULL) p = p->next;
        p->next = node;
    }
    return OK;
}

int deleteBed(const char *id)
{
    Bed *p = bedHead;
    Bed *pre = NULL;
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) {
            if (strcmp(p->status, "占用") == 0) return ERR_LINKED;
            if (pre == NULL) bedHead = p->next;
            else pre->next = p->next;
            free(p);
            return OK;
        }
        pre = p;
        p = p->next;
    }
    return ERR_NOT_FOUND;
}

Bed *findFreeBed(const char *deptId, const char *wardType)
{
    Bed *b = bedHead;
    Ward *w;
    while (b != NULL) {
        w = findWardById(b->wardId);
        if (w != NULL && strcmp(w->deptId, deptId) == 0 &&
            strcmp(w->type, wardType) == 0 && strcmp(b->status, "空闲") == 0) {
            return b;
        }
        b = b->next;
    }
    return NULL;
}

int occupyBed(const char *bedId, const char *patientId, const char *inDate)
{
    Bed *b = findBedById(bedId);
    Ward *w;
    if (b == NULL) return ERR_NOT_FOUND;
    if (strcmp(b->status, "空闲") != 0 || !checkDate(inDate)) return ERR_INPUT;
    w = findWardById(b->wardId);
    if (w == NULL || w->freeBeds <= 0) return ERR_NO_SPACE;
    // 答辩注意：住院时床位和病房空床数要一起改，两个数据要保持一致（难度：中等）
    safeCopy(b->status, STATUS_LEN, "占用");
    safeCopy(b->patientId, ID_LEN, patientId);
    safeCopy(b->inDate, DATE_LEN, inDate);
    w->freeBeds--;
    return OK;
}

int releasePatientBed(const char *patientId)
{
    Bed *b = findBedByPatient(patientId);
    Ward *w;
    if (b == NULL) return ERR_NOT_FOUND;
    w = findWardById(b->wardId);
    safeCopy(b->status, STATUS_LEN, "空闲");
    b->patientId[0] = '\0';
    b->inDate[0] = '\0';
    if (w != NULL && w->freeBeds < w->totalBeds) {
        w->freeBeds++;
    }
    return OK;
}

Bed *findBedByPatient(const char *patientId)
{
    Bed *b = bedHead;
    while (b != NULL) {
        if (strcmp(b->patientId, patientId) == 0 && strcmp(b->status, "占用") == 0) {
            return b;
        }
        b = b->next;
    }
    return NULL;
}

void listWards(char *out, int outSize)
{
    Ward *p = wardHead;
    Department *dept;
    char line[LINE_LEN];
    clearText(out, outSize);
    appendLine(out, outSize, "病房编号 | 类型 | 科室 | 每日费用 | 总床位 | 空床");
    while (p != NULL) {
        dept = findDepartmentById(p->deptId);
        snprintf(line, sizeof(line), "%s | %s | %s | %.2f | %d | %d\n",
            p->id, p->type, dept ? dept->name : "未知科室", p->dailyFee, p->totalBeds, p->freeBeds);
        appendText(out, outSize, line);
        p = p->next;
    }
}

void listBeds(char *out, int outSize)
{
    Bed *p = bedHead;
    char line[LINE_LEN];
    clearText(out, outSize);
    appendLine(out, outSize, "床位编号 | 病房编号 | 状态 | 患者编号 | 入院日期");
    while (p != NULL) {
        snprintf(line, sizeof(line), "%s | %s | %s | %s | %s\n",
            p->id, p->wardId, p->status, p->patientId, p->inDate);
        appendText(out, outSize, line);
        p = p->next;
    }
}
