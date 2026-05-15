#include "doctor.h"
#include "department.h"
#include "data.h"

void makeNextDoctorId(char *out, int outSize)
{
    Doctor *p = doctorHead;
    int max = 0;
    int num;
    while (p != NULL) {
        num = getIdNumber(p->id, "DOC");
        if (num > max) max = num;
        p = p->next;
    }
    makeId(out, outSize, "DOC", max + 1);
}

Doctor *findDoctorById(const char *id)
{
    Doctor *p = doctorHead;
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) return p;
        p = p->next;
    }
    return NULL;
}

int collectDoctorsByName(const char *name, Doctor *list[], int maxCount)
{
    Doctor *p = doctorHead;
    int count = 0;
    while (p != NULL && count < maxCount) {
        if (strcmp(p->name, name) == 0) {
            list[count++] = p;
        }
        p = p->next;
    }
    return count;
}

int addDoctor(const char *id, const char *name, const char *deptId, const char *title,
    const char *workTime, int maxCount)
{
    Doctor *node;
    Doctor *p;
    if (isBlank(id) || !checkName(name) || findDepartmentById(deptId) == NULL ||
        isBlank(title) || hasBadChar(title) || isBlank(workTime) || hasBadChar(workTime) ||
        maxCount < 1 || maxCount > 200) {
        return ERR_INPUT;
    }
    if (findDoctorById(id) != NULL) {
        return ERR_REPEAT;
    }
    node = (Doctor *)malloc(sizeof(Doctor));
    if (node == NULL) return ERR_FILE;
    safeCopy(node->id, ID_LEN, id);
    safeCopy(node->name, NAME_LEN, name);
    safeCopy(node->deptId, ID_LEN, deptId);
    safeCopy(node->title, NAME_LEN, title);
    safeCopy(node->workTime, NAME_LEN, workTime);
    node->maxCount = maxCount;
    node->next = NULL;
    if (doctorHead == NULL) {
        doctorHead = node;
    } else {
        p = doctorHead;
        while (p->next != NULL) p = p->next;
        p->next = node;
    }
    return OK;
}

int updateDoctor(const char *id, const char *title, const char *workTime, int maxCount)
{
    Doctor *p = findDoctorById(id);
    if (p == NULL) return ERR_NOT_FOUND;
    if (isBlank(title) || hasBadChar(title) || isBlank(workTime) || hasBadChar(workTime) ||
        maxCount < 0 || maxCount > 200) {
        return ERR_INPUT;
    }
    safeCopy(p->title, NAME_LEN, title);
    safeCopy(p->workTime, NAME_LEN, workTime);
    p->maxCount = maxCount;
    return OK;
}

static int doctorHasLink(const char *id)
{
    MedicalRecord *r = recordHead;
    Prescription *rx = prescriptionHead;
    while (r != NULL) {
        if (strcmp(r->doctorId, id) == 0) return 1;
        r = r->next;
    }
    while (rx != NULL) {
        if (strcmp(rx->doctorId, id) == 0) return 1;
        rx = rx->next;
    }
    return 0;
}

int deleteDoctor(const char *id)
{
    Doctor *p = doctorHead;
    Doctor *pre = NULL;
    if (doctorHasLink(id)) return ERR_LINKED;
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) {
            if (pre == NULL) doctorHead = p->next;
            else pre->next = p->next;
            free(p);
            return OK;
        }
        pre = p;
        p = p->next;
    }
    return ERR_NOT_FOUND;
}

void listDoctors(char *out, int outSize)
{
    Doctor *p = doctorHead;
    Department *dept;
    char line[LINE_LEN];
    clearText(out, outSize);
    appendLine(out, outSize, "医生编号 | 姓名 | 科室 | 职称 | 出诊时间 | 最大号源");
    while (p != NULL) {
        dept = findDepartmentById(p->deptId);
        snprintf(line, sizeof(line), "%s | %s | %s | %s | %s | %d\n",
            p->id, p->name, dept ? dept->name : "未知科室", p->title, p->workTime, p->maxCount);
        appendText(out, outSize, line);
        p = p->next;
    }
}
