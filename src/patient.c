#include "patient.h"
#include "data.h"

void makeNextPatientId(char *out, int outSize)
{
    Patient *p;
    int max = 0;
    int num;
    p = patientHead;
    while (p != NULL) {
        num = getIdNumber(p->id, "PAT");
        if (num > max) {
            max = num;
        }
        p = p->next;
    }
    makeId(out, outSize, "PAT", max + 1);
}

Patient *findPatientById(const char *id)
{
    Patient *p = patientHead;
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

Patient *findPatientByCardId(const char *cardId)
{
    Patient *p = patientHead;
    while (p != NULL) {
        if (strcmp(p->cardId, cardId) == 0) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

int addPatient(const char *id, const char *name, const char *gender, int age,
    const char *phone, const char *cardId, const char *status)
{
    Patient *node;
    Patient *p;
    if (isBlank(id) || !checkName(name) || !checkGender(gender) ||
        !checkAge(age) || !checkPhone(phone) || !checkCardId(cardId) ||
        isBlank(status) || hasBadChar(status)) {
        return ERR_INPUT;
    }
    if (findPatientById(id) != NULL || findPatientByCardId(cardId) != NULL) {
        return ERR_REPEAT;
    }
    node = (Patient *)malloc(sizeof(Patient));
    if (node == NULL) {
        return ERR_FILE;
    }
    safeCopy(node->id, ID_LEN, id);
    safeCopy(node->name, NAME_LEN, name);
    safeCopy(node->gender, STATUS_LEN, gender);
    node->age = age;
    safeCopy(node->phone, PHONE_LEN, phone);
    safeCopy(node->cardId, NAME_LEN, cardId);
    safeCopy(node->status, STATUS_LEN, status);
    node->next = NULL;

    // 答辩注意：这里用尾插法，数据列表顺序和录入顺序一样，比较好检查（难度：简单）
    if (patientHead == NULL) {
        patientHead = node;
    } else {
        p = patientHead;
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = node;
    }
    return OK;
}

int updatePatient(const char *id, const char *phone, const char *status)
{
    Patient *p = findPatientById(id);
    if (p == NULL) {
        return ERR_NOT_FOUND;
    }
    if (!checkPhone(phone) || isBlank(status) || hasBadChar(status)) {
        return ERR_INPUT;
    }
    safeCopy(p->phone, PHONE_LEN, phone);
    safeCopy(p->status, STATUS_LEN, status);
    return OK;
}

int updatePatientStatus(const char *id, const char *status)
{
    Patient *p = findPatientById(id);
    if (p == NULL) {
        return ERR_NOT_FOUND;
    }
    if (isBlank(status) || hasBadChar(status)) {
        return ERR_INPUT;
    }
    safeCopy(p->status, STATUS_LEN, status);
    return OK;
}

static int patientHasLink(const char *id)
{
    MedicalRecord *r = recordHead;
    Prescription *rx = prescriptionHead;
    Payment *pay = paymentHead;
    Bed *bed = bedHead;
    while (r != NULL) {
        if (strcmp(r->patientId, id) == 0) {
            return 1;
        }
        r = r->next;
    }
    while (rx != NULL) {
        if (strcmp(rx->patientId, id) == 0) {
            return 1;
        }
        rx = rx->next;
    }
    while (pay != NULL) {
        if (strcmp(pay->patientId, id) == 0) {
            return 1;
        }
        pay = pay->next;
    }
    while (bed != NULL) {
        if (strcmp(bed->patientId, id) == 0) {
            return 1;
        }
        bed = bed->next;
    }
    return 0;
}

int deletePatient(const char *id)
{
    Patient *p = patientHead;
    Patient *pre = NULL;
    if (patientHasLink(id)) {
        // 答辩注意：患者有历史记录时不能删除，不然记录会找不到患者（难度：中等）
        return ERR_LINKED;
    }
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) {
            if (pre == NULL) {
                patientHead = p->next;
            } else {
                pre->next = p->next;
            }
            free(p);
            return OK;
        }
        pre = p;
        p = p->next;
    }
    return ERR_NOT_FOUND;
}

int collectPatientsByName(const char *name, Patient *list[], int maxCount)
{
    Patient *p = patientHead;
    int count = 0;
    while (p != NULL && count < maxCount) {
        if (strcmp(p->name, name) == 0) {
            list[count] = p;
            count++;
        }
        p = p->next;
    }
    return count;
}

static void appendPatientLine(Patient *p, char *out, int outSize)
{
    char line[LINE_LEN];
    snprintf(line, sizeof(line), "%s | %s | %s | %d | %s | %s | %s\n",
        p->id, p->name, p->gender, p->age, p->phone, p->cardId, p->status);
    appendText(out, outSize, line);
}

void listPatients(char *out, int outSize)
{
    Patient *p = patientHead;
    clearText(out, outSize);
    appendLine(out, outSize, "患者编号 | 姓名 | 性别 | 年龄 | 电话 | 身份证号 | 状态");
    while (p != NULL) {
        appendPatientLine(p, out, outSize);
        p = p->next;
    }
}

void queryPatientByName(const char *name, char *out, int outSize)
{
    Patient *p = patientHead;
    int count = 0;
    clearText(out, outSize);
    appendLine(out, outSize, "查询结果：");
    while (p != NULL) {
        if (strcmp(p->name, name) == 0) {
            appendPatientLine(p, out, outSize);
            count++;
        }
        p = p->next;
    }
    if (count == 0) {
        appendLine(out, outSize, "没有找到同名患者。");
    } else if (count > 1) {
        appendLine(out, outSize, "提示：有重名患者，办理业务时请看身份证号和电话再选择序号。");
    }
}
