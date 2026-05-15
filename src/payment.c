#include "payment.h"
#include "patient.h"
#include "data.h"

void makeNextPaymentId(char *out, int outSize)
{
    Payment *p = paymentHead;
    int max = 0;
    int num;
    while (p != NULL) {
        num = getIdNumber(p->id, "PAY");
        if (num > max) max = num;
        p = p->next;
    }
    makeId(out, outSize, "PAY", max + 1);
}

Payment *findPaymentById(const char *id)
{
    Payment *p = paymentHead;
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) return p;
        p = p->next;
    }
    return NULL;
}

Payment *findPaymentBySource(const char *sourceType, const char *sourceId)
{
    Payment *p = paymentHead;
    while (p != NULL) {
        if (strcmp(p->sourceType, sourceType) == 0 && strcmp(p->sourceId, sourceId) == 0) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

int addPayment(const char *id, const char *patientId, const char *sourceType,
    const char *sourceId, double amount, const char *status, const char *date, const char *note)
{
    Payment *node;
    Payment *p;
    if (isBlank(id) || findPatientById(patientId) == NULL || isBlank(sourceType) ||
        isBlank(sourceId) || !checkMoney(amount) || !checkDate(date) ||
        (strcmp(status, "未缴费") != 0 && strcmp(status, "已缴费") != 0) ||
        hasBadChar(note)) {
        return ERR_INPUT;
    }
    if (findPaymentById(id) != NULL || findPaymentBySource(sourceType, sourceId) != NULL) {
        return ERR_REPEAT;
    }
    node = (Payment *)malloc(sizeof(Payment));
    if (node == NULL) return ERR_FILE;
    safeCopy(node->id, ID_LEN, id);
    safeCopy(node->patientId, ID_LEN, patientId);
    safeCopy(node->sourceType, STATUS_LEN, sourceType);
    safeCopy(node->sourceId, ID_LEN, sourceId);
    node->amount = amount;
    safeCopy(node->status, STATUS_LEN, status);
    safeCopy(node->date, DATE_LEN, date);
    safeCopy(node->note, TEXT_LEN, note);
    node->next = NULL;
    if (paymentHead == NULL) {
        paymentHead = node;
    } else {
        p = paymentHead;
        while (p->next != NULL) p = p->next;
        p->next = node;
    }
    return OK;
}

int deletePaymentById(const char *id)
{
    Payment **pp = &paymentHead;
    if (id == NULL || isBlank(id)) return ERR_INPUT;
    while (*pp != NULL) {
        if (strcmp((*pp)->id, id) == 0) {
            Payment *t = *pp;
            *pp = t->next;
            free(t);
            return OK;
        }
        pp = &(*pp)->next;
    }
    return ERR_NOT_FOUND;
}

int payPayment(const char *id, const char *date)
{
    Payment *p = findPaymentById(id);
    if (p == NULL) return ERR_NOT_FOUND;
    if (!checkDate(date)) return ERR_INPUT;
    if (strcmp(p->status, "已缴费") == 0) return ERR_REPEAT;
    // 答辩注意：同一笔账单不能重复缴费，避免状态错乱（难度：简单）
    safeCopy(p->status, STATUS_LEN, "已缴费");
    safeCopy(p->date, DATE_LEN, date);
    return OK;
}

int isSourcePaid(const char *sourceType, const char *sourceId)
{
    Payment *p = findPaymentBySource(sourceType, sourceId);
    if (p == NULL) return 0;
    return strcmp(p->status, "已缴费") == 0;
}

static void appendPaymentLine(Payment *p, char *out, int outSize)
{
    Patient *patient = findPatientById(p->patientId);
    char line[LINE_LEN];
    snprintf(line, sizeof(line), "%s | %s | %s | %s | %.2f | %s | %s | %s\n",
        p->id, patient ? patient->name : "未知患者", p->sourceType, p->sourceId,
        p->amount, p->status, p->date, p->note);
    appendText(out, outSize, line);
}

void listPayments(char *out, int outSize)
{
    Payment *p = paymentHead;
    clearText(out, outSize);
    appendLine(out, outSize, "缴费编号 | 患者 | 类型 | 来源编号 | 金额 | 状态 | 日期 | 备注");
    while (p != NULL) {
        appendPaymentLine(p, out, outSize);
        p = p->next;
    }
}

void listPaymentsByPatient(const char *patientId, char *out, int outSize)
{
    Payment *p = paymentHead;
    int count = 0;
    clearText(out, outSize);
    appendLine(out, outSize, "缴费编号 | 患者 | 类型 | 来源编号 | 金额 | 状态 | 日期 | 备注");
    while (p != NULL) {
        if (strcmp(p->patientId, patientId) == 0) {
            appendPaymentLine(p, out, outSize);
            count++;
        }
        p = p->next;
    }
    if (count == 0) appendLine(out, outSize, "该患者没有缴费记录。");
}

static int paymentDateInRange(const char *d, const char *begin, const char *end)
{
    if (d == NULL || !checkDate(d)) return 0;
    if (begin != NULL && !isBlank(begin)) {
        if (!checkDate(begin) || compareDate(d, begin) < 0) return 0;
    }
    if (end != NULL && !isBlank(end)) {
        if (!checkDate(end) || compareDate(d, end) > 0) return 0;
    }
    return 1;
}

void paymentSummaryByPatient(const char *patientId, char *out, int outSize)
{
    Payment *p = paymentHead;
    double reg = 0, exam = 0, rx = 0, inhosp = 0, other = 0, unpaid = 0, paidTotal = 0;
    char line[LINE_LEN];
    if (patientId == NULL) {
        clearText(out, outSize);
        return;
    }
    clearText(out, outSize);
    appendLine(out, outSize, "费用汇总（按缴费类型，仅统计该患者）");
    p = paymentHead;
    while (p != NULL) {
        if (strcmp(p->patientId, patientId) != 0) {
            p = p->next;
            continue;
        }
        if (strcmp(p->status, "已缴费") == 0) {
            paidTotal += p->amount;
            if (strcmp(p->sourceType, "挂号") == 0) {
                reg += p->amount;
            } else if (strcmp(p->sourceType, "检查") == 0) {
                exam += p->amount;
            } else if (strcmp(p->sourceType, "处方") == 0) {
                rx += p->amount;
            } else if (strcmp(p->sourceType, "住院") == 0) {
                inhosp += p->amount;
            } else {
                other += p->amount;
            }
        } else {
            unpaid += p->amount;
        }
        p = p->next;
    }
    snprintf(line, sizeof(line), "挂号费合计：%.2f\n", reg);
    appendText(out, outSize, line);
    snprintf(line, sizeof(line), "检查费合计：%.2f\n", exam);
    appendText(out, outSize, line);
    snprintf(line, sizeof(line), "药费（处方）合计：%.2f\n", rx);
    appendText(out, outSize, line);
    snprintf(line, sizeof(line), "住院费合计：%.2f\n", inhosp);
    appendText(out, outSize, line);
    snprintf(line, sizeof(line), "其他缴费合计：%.2f\n", other);
    appendText(out, outSize, line);
    snprintf(line, sizeof(line), "已缴费总计：%.2f\n", paidTotal);
    appendText(out, outSize, line);
    snprintf(line, sizeof(line), "未缴费总计：%.2f\n", unpaid);
    appendText(out, outSize, line);
}

void paymentSummaryByPatientInRange(const char *patientId,
    const char *dateBegin, const char *dateEnd, char *out, int outSize)
{
    Payment *p = paymentHead;
    double reg = 0, exam = 0, rx = 0, inhosp = 0, other = 0, unpaid = 0, paidTotal = 0;
    char line[LINE_LEN];
    if (patientId == NULL) {
        clearText(out, outSize);
        return;
    }
    clearText(out, outSize);
    appendLine(out, outSize, "费用汇总（按所选日期范围内的缴费）");
    while (p != NULL) {
        if (strcmp(p->patientId, patientId) != 0) {
            p = p->next;
            continue;
        }
        if (!paymentDateInRange(p->date, dateBegin, dateEnd)) {
            p = p->next;
            continue;
        }
        if (strcmp(p->status, "已缴费") == 0) {
            paidTotal += p->amount;
            if (strcmp(p->sourceType, "挂号") == 0) {
                reg += p->amount;
            } else if (strcmp(p->sourceType, "检查") == 0) {
                exam += p->amount;
            } else if (strcmp(p->sourceType, "处方") == 0) {
                rx += p->amount;
            } else if (strcmp(p->sourceType, "住院") == 0) {
                inhosp += p->amount;
            } else {
                other += p->amount;
            }
        } else {
            unpaid += p->amount;
        }
        p = p->next;
    }
    snprintf(line, sizeof(line), "挂号费合计：%.2f\n", reg);
    appendText(out, outSize, line);
    snprintf(line, sizeof(line), "检查费合计：%.2f\n", exam);
    appendText(out, outSize, line);
    snprintf(line, sizeof(line), "药费（处方）合计：%.2f\n", rx);
    appendText(out, outSize, line);
    snprintf(line, sizeof(line), "住院费合计：%.2f\n", inhosp);
    appendText(out, outSize, line);
    snprintf(line, sizeof(line), "其他缴费合计：%.2f\n", other);
    appendText(out, outSize, line);
    snprintf(line, sizeof(line), "已缴费总计：%.2f\n", paidTotal);
    appendText(out, outSize, line);
    snprintf(line, sizeof(line), "未缴费总计：%.2f\n", unpaid);
    appendText(out, outSize, line);
}

void listPaymentsByPatientPatientView(const char *patientId,
    const char *dateBegin, const char *dateEnd, char *out, int outSize)
{
    Payment *p = paymentHead;
    char line[LINE_LEN];
    int count = 0;
    if (patientId == NULL) {
        clearText(out, outSize);
        return;
    }
    clearText(out, outSize);
    appendLine(out, outSize, "费用类型 | 金额 | 状态 | 日期 | 备注");
    while (p != NULL) {
        if (strcmp(p->patientId, patientId) != 0) {
            p = p->next;
            continue;
        }
        if (!paymentDateInRange(p->date, dateBegin, dateEnd)) {
            p = p->next;
            continue;
        }
        snprintf(line, sizeof(line), "%s | %.2f | %s | %s | %s\n",
            p->sourceType, p->amount, p->status, p->date, p->note);
        appendText(out, outSize, line);
        count++;
        p = p->next;
    }
    if (count == 0) {
        appendLine(out, outSize, "所选条件下没有缴费记录。");
    }
}

int collectUnpaidPaymentsByPatient(const char *patientId, Payment *list[], int max)
{
    Payment *p = paymentHead;
    int n = 0;
    if (patientId == NULL || list == NULL || max <= 0) return 0;
    while (p != NULL && n < max) {
        if (strcmp(p->patientId, patientId) == 0 && strcmp(p->status, "未缴费") == 0) {
            list[n++] = p;
        }
        p = p->next;
    }
    return n;
}
