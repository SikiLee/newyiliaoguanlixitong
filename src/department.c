#include "department.h"
#include "data.h"

void makeNextDepartmentId(char *out, int outSize)
{
    Department *p = departmentHead;
    int max = 0;
    int num;
while (p != NULL) {
        num = getIdNumber(p->id, "DEP");
        if (num > max) max = num;
        p = p->next;
    }
    makeId(out, outSize, "DEP", max + 1);
}

Department *findDepartmentById(const char *id)
{
    Department *p = departmentHead;
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) return p;
        p = p->next;
    }
    return NULL;
}

Department *findDepartmentByName(const char *name)
{
    Department *p = departmentHead;
    while (p != NULL) {
        if (strcmp(p->name, name) == 0) return p;
        p = p->next;
    }
    return NULL;
}

int addDepartment(const char *id, const char *name, const char *intro, const char *wardType)
{
    Department *node;
    Department *p;
    if (isBlank(id) || !checkName(name) || hasBadChar(intro) || isBlank(wardType) || hasBadChar(wardType)) {
        return ERR_INPUT;
    }
    if (findDepartmentById(id) != NULL || findDepartmentByName(name) != NULL) {
        return ERR_REPEAT;
    }
    node = (Department *)malloc(sizeof(Department));
    if (node == NULL) return ERR_FILE;
    safeCopy(node->id, ID_LEN, id);
    safeCopy(node->name, NAME_LEN, name);
    safeCopy(node->intro, TEXT_LEN, intro);
    safeCopy(node->wardType, NAME_LEN, wardType);
    node->next = NULL;
    if (departmentHead == NULL) {
        departmentHead = node;
    } else {
        p = departmentHead;
        while (p->next != NULL) p = p->next;
        p->next = node;
    }
    return OK;
}

int updateDepartment(const char *id, const char *intro, const char *wardType)
{
    Department *p = findDepartmentById(id);
    if (p == NULL) return ERR_NOT_FOUND;
    if (hasBadChar(intro) || isBlank(wardType) || hasBadChar(wardType)) return ERR_INPUT;
    safeCopy(p->intro, TEXT_LEN, intro);
    safeCopy(p->wardType, NAME_LEN, wardType);
    return OK;
}

static int departmentHasLink(const char *id)
{
    Doctor *d = doctorHead;
    Medicine *m = medicineHead;
    Ward *w = wardHead;
    MedicalRecord *r = recordHead;
    Prescription *rx = prescriptionHead;
    while (d != NULL) {
        if (strcmp(d->deptId, id) == 0) return 1;
        d = d->next;
    }
    while (m != NULL) {
        if (strcmp(m->deptId, id) == 0) return 1;
        m = m->next;
    }
    while (w != NULL) {
        if (strcmp(w->deptId, id) == 0) return 1;
        w = w->next;
    }
    while (r != NULL) {
        if (strcmp(r->deptId, id) == 0) return 1;
        r = r->next;
    }
    while (rx != NULL) {
        if (strcmp(rx->deptId, id) == 0) return 1;
        rx = rx->next;
    }
    return 0;
}

int deleteDepartment(const char *id)
{
    Department *p = departmentHead;
    Department *pre = NULL;
    if (departmentHasLink(id)) {
        return ERR_LINKED;
    }
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) {
            if (pre == NULL) departmentHead = p->next;
            else pre->next = p->next;
            free(p);
            return OK;
        }
        pre = p;
        p = p->next;
    }
    return ERR_NOT_FOUND;
}

void listDepartments(char *out, int outSize)
{
    Department *p = departmentHead;
    char line[LINE_LEN];
    clearText(out, outSize);
    appendLine(out, outSize, "科室编号 | 科室名 | 关联病房类型 | 简介");
    while (p != NULL) {
        snprintf(line, sizeof(line), "%s | %s | %s | %s\n", p->id, p->name, p->wardType, p->intro);
        appendText(out, outSize, line);
        p = p->next;
    }
}
