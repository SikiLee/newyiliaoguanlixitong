#include "prescription.h"
#include "patient.h"
#include "doctor.h"
#include "department.h"
#include "medicine.h"
#include "data.h"

void makeNextPrescriptionId(char *out, int outSize)
{
    Prescription *p = prescriptionHead;
    int max = 0;
    int num;
    while (p != NULL) {
        num = getIdNumber(p->id, "RX");
        if (num > max) max = num;
        p = p->next;
    }
    makeId(out, outSize, "RX", max + 1);
}

Prescription *findPrescriptionById(const char *id)
{
    Prescription *p = prescriptionHead;
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) return p;
        p = p->next;
    }
    return NULL;
}

int addPrescription(const char *id, const char *patientId, const char *doctorId,
    const char *deptId, const char *date, const char *medicalRecordId, const char *advice)
{
    Prescription *node;
    Prescription *p;
    if (isBlank(id) || findPatientById(patientId) == NULL || findDoctorById(doctorId) == NULL ||
        findDepartmentById(deptId) == NULL || !checkDate(date)) {
        return ERR_INPUT;
    }
    if (findPrescriptionById(id) != NULL) return ERR_REPEAT;
    node = (Prescription *)malloc(sizeof(Prescription));
    if (node == NULL) return ERR_FILE;
    safeCopy(node->id, ID_LEN, id);
    safeCopy(node->patientId, ID_LEN, patientId);
    safeCopy(node->doctorId, ID_LEN, doctorId);
    safeCopy(node->deptId, ID_LEN, deptId);
    safeCopy(node->date, DATE_LEN, date);
    safeCopy(node->medicalRecordId, ID_LEN, medicalRecordId ? medicalRecordId : "");
    safeCopy(node->advice, TEXT_LEN, advice ? advice : "");
    node->itemCount = 0;
    node->totalFee = 0;
    safeCopy(node->status, STATUS_LEN, "未缴费");
    node->next = NULL;
    if (prescriptionHead == NULL) {
        prescriptionHead = node;
    } else {
        p = prescriptionHead;
        while (p->next != NULL) p = p->next;
        p->next = node;
    }
    return OK;
}

int setPrescriptionMedicalRecordId(const char *id, const char *medicalRecordId)
{
    Prescription *rx = findPrescriptionById(id);
    if (rx == NULL) return ERR_NOT_FOUND;
    if (medicalRecordId == NULL || isBlank(medicalRecordId)) return ERR_INPUT;
    safeCopy(rx->medicalRecordId, ID_LEN, medicalRecordId);
    return OK;
}

int addPrescriptionItem(const char *prescriptionId, const char *medicineId, int count)
{
    Prescription *rx = findPrescriptionById(prescriptionId);
    Medicine *medicine = findMedicineById(medicineId);
    PrescriptionItem *node;
    PrescriptionItem *p;
    if (rx == NULL || medicine == NULL || !checkCount(count)) return ERR_INPUT;
    node = (PrescriptionItem *)malloc(sizeof(PrescriptionItem));
    if (node == NULL) return ERR_FILE;
    safeCopy(node->prescriptionId, ID_LEN, prescriptionId);
    safeCopy(node->medicineId, ID_LEN, medicineId);
    node->count = count;
    node->price = medicine->price;
    node->next = NULL;
    if (prescriptionItemHead == NULL) {
        prescriptionItemHead = node;
    } else {
        p = prescriptionItemHead;
        while (p->next != NULL) p = p->next;
        p->next = node;
    }
    rx->itemCount++;
    rx->totalFee += medicine->price * count;
    return OK;
}

int setPrescriptionPaid(const char *id)
{
    Prescription *rx = findPrescriptionById(id);
    if (rx == NULL) return ERR_NOT_FOUND;
    if (rx->itemCount <= 0) return ERR_INPUT;
    safeCopy(rx->status, STATUS_LEN, "已缴费");
    return OK;
}

int sendPrescription(const char *id)
{
    Prescription *rx = findPrescriptionById(id);
    PrescriptionItem *item;
    Medicine *medicine;
    int stockCode;
    if (rx == NULL) return ERR_NOT_FOUND;
    if (strcmp(rx->status, "已缴费") != 0) return ERR_INPUT;

    item = prescriptionItemHead;
    while (item != NULL) {
        if (strcmp(item->prescriptionId, id) == 0) {
            medicine = findMedicineById(item->medicineId);
            if (medicine == NULL || medicine->stock < item->count) {
                return ERR_NO_SPACE;
            }
        }
        item = item->next;
    }

    item = prescriptionItemHead;
    while (item != NULL) {
        if (strcmp(item->prescriptionId, id) == 0) {
            stockCode = reduceMedicineStock(item->medicineId, item->count);
            if (stockCode != OK) {
                return stockCode;
            }
        }
        item = item->next;
    }
    safeCopy(rx->status, STATUS_LEN, "已发药");
    return OK;
}

static int utf8TextWidth(const char *s)
{
    int width = 0;
    unsigned char c;
    if (s == NULL) return 0;
    while (*s != '\0') {
        c = (unsigned char)*s;
        if (c < 128) {
            width++;
            s++;
        } else {
            width += 2;
            s++;
            while (((unsigned char)*s & 0xC0) == 0x80) s++;
        }
    }
    return width;
}

static void appendTableSpaces(char *out, int outSize, int count)
{
    int i;
    for (i = 0; i < count; i++) {
        appendText(out, outSize, " ");
    }
}

static void appendTableCell(char *out, int outSize, const char *text, int width, int rightAlign)
{
    int pad = width - utf8TextWidth(text);
    if (pad < 0) pad = 0;
    if (rightAlign) appendTableSpaces(out, outSize, pad);
    appendText(out, outSize, text ? text : "");
    if (!rightAlign) appendTableSpaces(out, outSize, pad);
}

void listPrescriptionItems(const char *prescriptionId, char *out, int outSize)
{
    PrescriptionItem *item = prescriptionItemHead;
    Medicine *medicine;
    char line[LINE_LEN];
    char countText[32];
    char priceText[32];
    char subTotalText[32];
    clearText(out, outSize);
    clearText(line, sizeof(line));
    appendTableCell(line, sizeof(line), "处方编号", 10, 0);
    appendText(line, sizeof(line), " | ");
    appendTableCell(line, sizeof(line), "药品", 18, 0);
    appendText(line, sizeof(line), " | ");
    appendTableCell(line, sizeof(line), "数量", 4, 1);
    appendText(line, sizeof(line), " | ");
    appendTableCell(line, sizeof(line), "单价", 8, 1);
    appendText(line, sizeof(line), " | ");
    appendTableCell(line, sizeof(line), "小计", 8, 1);
    appendLine(out, outSize, line);
    while (item != NULL) {
        if (strcmp(item->prescriptionId, prescriptionId) == 0) {
            medicine = findMedicineById(item->medicineId);
            snprintf(countText, sizeof(countText), "%d", item->count);
            snprintf(priceText, sizeof(priceText), "%.2f", item->price);
            snprintf(subTotalText, sizeof(subTotalText), "%.2f", item->price * item->count);
            clearText(line, sizeof(line));
            appendTableCell(line, sizeof(line), item->prescriptionId, 10, 0);
            appendText(line, sizeof(line), " | ");
            appendTableCell(line, sizeof(line), medicine ? medicine->name : "未知药品", 18, 0);
            appendText(line, sizeof(line), " | ");
            appendTableCell(line, sizeof(line), countText, 4, 1);
            appendText(line, sizeof(line), " | ");
            appendTableCell(line, sizeof(line), priceText, 8, 1);
            appendText(line, sizeof(line), " | ");
            appendTableCell(line, sizeof(line), subTotalText, 8, 1);
            appendLine(out, outSize, line);
        }
        item = item->next;
    }
}

void listPrescriptionItemsForPatient(const char *prescriptionId, char *out, int outSize)
{
    PrescriptionItem *item = prescriptionItemHead;
    Medicine *medicine;
    char line[LINE_LEN];
    char countText[32];
    char priceText[32];
    char subTotalText[32];
    int n = 0;
    clearText(out, outSize);
    clearText(line, sizeof(line));
    appendTableCell(line, sizeof(line), "药品名称", 20, 0);
    appendText(line, sizeof(line), " | ");
    appendTableCell(line, sizeof(line), "数量", 4, 1);
    appendText(line, sizeof(line), " | ");
    appendTableCell(line, sizeof(line), "单价", 8, 1);
    appendText(line, sizeof(line), " | ");
    appendTableCell(line, sizeof(line), "小计", 8, 1);
    appendLine(out, outSize, line);
    while (item != NULL) {
        if (strcmp(item->prescriptionId, prescriptionId) == 0) {
            medicine = findMedicineById(item->medicineId);
            snprintf(countText, sizeof(countText), "%d", item->count);
            snprintf(priceText, sizeof(priceText), "%.2f", item->price);
            snprintf(subTotalText, sizeof(subTotalText), "%.2f", item->price * item->count);
            clearText(line, sizeof(line));
            appendTableCell(line, sizeof(line), medicine ? medicine->name : "未知药品", 20, 0);
            appendText(line, sizeof(line), " | ");
            appendTableCell(line, sizeof(line), countText, 4, 1);
            appendText(line, sizeof(line), " | ");
            appendTableCell(line, sizeof(line), priceText, 8, 1);
            appendText(line, sizeof(line), " | ");
            appendTableCell(line, sizeof(line), subTotalText, 8, 1);
            appendLine(out, outSize, line);
            n++;
        }
        item = item->next;
    }
    if (n == 0) {
        appendLine(out, outSize, "（该处方暂无药品明细）");
    }
}

int collectPrescriptionsByPatient(const char *patientId, Prescription *list[], int max)
{
    Prescription *p = prescriptionHead;
    int n = 0;
    if (patientId == NULL || list == NULL || max <= 0) return 0;
    while (p != NULL && n < max) {
        if (strcmp(p->patientId, patientId) == 0) {
            list[n++] = p;
        }
        p = p->next;
    }
    return n;
}

void listPrescriptions(char *out, int outSize)
{
    Prescription *p = prescriptionHead;
    Patient *patient;
    Doctor *doctor;
    char line[LINE_LEN];
    clearText(out, outSize);
    appendLine(out, outSize, "处方编号 | 患者 | 医生 | 日期 | 药品数 | 总金额 | 状态");
    while (p != NULL) {
        patient = findPatientById(p->patientId);
        doctor = findDoctorById(p->doctorId);
        snprintf(line, sizeof(line), "%s | %s | %s | %s | %d | %.2f | %s\n",
            p->id, patient ? patient->name : "未知患者", doctor ? doctor->name : "未知医生",
            p->date, p->itemCount, p->totalFee, p->status);
        appendText(out, outSize, line);
        p = p->next;
    }
}

int deletePrescriptionById(const char *id)
{
    Prescription *p = prescriptionHead;
    Prescription *pre = NULL;
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) {
            if (pre == NULL) prescriptionHead = p->next;
            else pre->next = p->next;
            free(p);
            return OK;
        }
        pre = p;
        p = p->next;
    }
    return ERR_NOT_FOUND;
}

int deletePrescriptionItemsByRxId(const char *prescriptionId)
{
    PrescriptionItem *p = prescriptionItemHead;
    PrescriptionItem *pre = NULL;
    int count = 0;
    while (p != NULL) {
        if (strcmp(p->prescriptionId, prescriptionId) == 0) {
            if (pre == NULL) prescriptionItemHead = p->next;
            else pre->next = p->next;
            free(p);
            count++;
            p = pre == NULL ? prescriptionItemHead : pre->next;
        } else {
            pre = p;
            p = p->next;
        }
    }
    return count;
}
