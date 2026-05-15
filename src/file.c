#include "file.h"
#include "common.h"
#include "data.h"
#include "patient.h"
#include "department.h"
#include "doctor.h"
#include "medicine.h"
#include "ward.h"
#include "record.h"
#include "prescription.h"
#include "payment.h"

static void makePath(char *out, int outSize, const char *dir, const char *name)
{
    snprintf(out, outSize, "%s\\%s", dir, name);
}

static void removeEnter(char *s)
{
    int len = (int)strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}

static int splitLine(char *line, char *fields[], int needCount)
{
    int count = 0;
    char *start = line;
    char *p = line;
    while (1) {
        if (*p == '|' || *p == '\0') {
            if (count >= needCount) {
                return -1;
            }
            fields[count++] = start;
            if (*p == '\0') {
                break;
            }
            *p = '\0';
            start = p + 1;
        }
        p++;
    }
    if (count != needCount) {
        return -1;
    }
    return count;
}

int checkDataFile(const char *path, int fieldCount, char *msg, int msgSize)
{
    FILE *fp;
    char line[LINE_LEN];
    char *fields[20];
    int row = 0;
    fp = fopen(path, "r");
    clearText(msg, msgSize);
    if (fp == NULL) {
        snprintf(msg, msgSize, "文件打不开：%s", path);
        return ERR_FILE;
    }
    while (fgets(line, sizeof(line), fp) != NULL) {
        row++;
        removeEnter(line);
        if (isBlank(line)) continue;
        if (splitLine(line, fields, fieldCount) < 0) {
            snprintf(msg, msgSize, "第%d行字段数量错误，整文件拒绝导入。", row);
            fclose(fp);
            return ERR_FILE;
        }
    }
    fclose(fp);
    snprintf(msg, msgSize, "文件字段数量检查通过。");
    return OK;
}

static int loadDepartments(const char *path, char *msg, int msgSize)
{
    FILE *fp = fopen(path, "r");
    char line[LINE_LEN];
    char *f[4];
    int row = 0;
    int code;
    if (fp == NULL) return OK;
    while (fgets(line, sizeof(line), fp) != NULL) {
        row++;
        removeEnter(line);
        if (isBlank(line)) continue;
        if (splitLine(line, f, 4) != 4) {
            snprintf(msg, msgSize, "department.txt 第%d行字段数量错误。", row);
            fclose(fp);
            return ERR_FILE;
        }
        code = addDepartment(f[0], f[1], f[2], f[3]);
        if (code != OK) {
            snprintf(msg, msgSize, "department.txt 第%d行错误：%s。", row, codeText(code));
            fclose(fp);
            return ERR_FILE;
        }
    }
    fclose(fp);
    return OK;
}

static int loadPatients(const char *path, char *msg, int msgSize)
{
    FILE *fp = fopen(path, "r");
    char line[LINE_LEN];
    char *f[7];
    int row = 0;
    int age;
    int code;
    if (fp == NULL) return OK;
    while (fgets(line, sizeof(line), fp) != NULL) {
        row++;
        removeEnter(line);
        if (isBlank(line)) continue;
        if (splitLine(line, f, 7) != 7 || !parseIntStrict(f[3], &age)) {
            snprintf(msg, msgSize, "patient.txt 第%d行格式错误。", row);
            fclose(fp);
            return ERR_FILE;
        }
        code = addPatient(f[0], f[1], f[2], age, f[4], f[5], f[6]);
        if (code != OK) {
            snprintf(msg, msgSize, "patient.txt 第%d行错误：%s。", row, codeText(code));
            fclose(fp);
            return ERR_FILE;
        }
    }
    fclose(fp);
    return OK;
}

static int loadDoctors(const char *path, char *msg, int msgSize)
{
    FILE *fp = fopen(path, "r");
    char line[LINE_LEN];
    char *f[6];
    int row = 0;
    int maxCount;
    int code;
    if (fp == NULL) return OK;
    while (fgets(line, sizeof(line), fp) != NULL) {
        row++;
        removeEnter(line);
        if (isBlank(line)) continue;
        if (splitLine(line, f, 6) != 6 || !parseIntStrict(f[5], &maxCount)) {
            snprintf(msg, msgSize, "doctor.txt 第%d行格式错误。", row);
            fclose(fp);
            return ERR_FILE;
        }
        code = addDoctor(f[0], f[1], f[2], f[3], f[4], maxCount);
        if (code != OK) {
            snprintf(msg, msgSize, "doctor.txt 第%d行错误：%s。", row, codeText(code));
            fclose(fp);
            return ERR_FILE;
        }
    }
    fclose(fp);
    return OK;
}

static int loadMedicines(const char *path, char *msg, int msgSize)
{
    FILE *fp = fopen(path, "r");
    char line[LINE_LEN];
    char *f[9];
    int row = 0;
    int stock, lowLimit;
    double price;
    int code;
    if (fp == NULL) return OK;
    while (fgets(line, sizeof(line), fp) != NULL) {
        row++;
        removeEnter(line);
        if (isBlank(line)) continue;
        if (splitLine(line, f, 9) != 9 || !parseMoneyStrict(f[6], &price) ||
            !parseIntStrict(f[7], &stock) || !parseIntStrict(f[8], &lowLimit)) {
            snprintf(msg, msgSize, "medicine.txt 第%d行格式错误。", row);
            fclose(fp);
            return ERR_FILE;
        }
        code = addMedicine(f[0], f[1], f[2], f[3], f[4], f[5], price, stock, lowLimit);
        if (code != OK) {
            snprintf(msg, msgSize, "medicine.txt 第%d行错误：%s。", row, codeText(code));
            fclose(fp);
            return ERR_FILE;
        }
    }
    fclose(fp);
    return OK;
}

static int loadWards(const char *path, char *msg, int msgSize)
{
    FILE *fp = fopen(path, "r");
    char line[LINE_LEN];
    char *f[6];
    int row = 0;
    int totalBeds, freeBeds;
    double dailyFee;
    int code;
    if (fp == NULL) return OK;
    while (fgets(line, sizeof(line), fp) != NULL) {
        row++;
        removeEnter(line);
        if (isBlank(line)) continue;
        if (splitLine(line, f, 6) != 6 || !parseMoneyStrict(f[3], &dailyFee) ||
            !parseIntStrict(f[4], &totalBeds) || !parseIntStrict(f[5], &freeBeds)) {
            snprintf(msg, msgSize, "ward.txt 第%d行格式错误。", row);
            fclose(fp);
            return ERR_FILE;
        }
        code = addWard(f[0], f[1], f[2], dailyFee, totalBeds, freeBeds);
        if (code != OK) {
            snprintf(msg, msgSize, "ward.txt 第%d行错误：%s。", row, codeText(code));
            fclose(fp);
            return ERR_FILE;
        }
    }
    fclose(fp);
    return OK;
}

static int loadBeds(const char *path, char *msg, int msgSize)
{
    FILE *fp = fopen(path, "r");
    char line[LINE_LEN];
    char *f[5];
    int row = 0;
    int code;
    if (fp == NULL) return OK;
    while (fgets(line, sizeof(line), fp) != NULL) {
        row++;
        removeEnter(line);
        if (isBlank(line)) continue;
        if (splitLine(line, f, 5) != 5) {
            snprintf(msg, msgSize, "bed.txt 第%d行字段数量错误。", row);
            fclose(fp);
            return ERR_FILE;
        }
        code = addBed(f[0], f[1], f[2], f[3], f[4]);
        if (code != OK) {
            snprintf(msg, msgSize, "bed.txt 第%d行错误：%s。", row, codeText(code));
            fclose(fp);
            return ERR_FILE;
        }
    }
    fclose(fp);
    return OK;
}

static int loadRecords(const char *path, char *msg, int msgSize)
{
    FILE *fp = fopen(path, "r");
    char line[LINE_LEN];
    char lineCopy[LINE_LEN];
    char *f[11];
    int row = 0;
    double fee;
    int code;
    int regStatus;
    int n;
    if (fp == NULL) return OK;
    while (fgets(line, sizeof(line), fp) != NULL) {
        row++;
        removeEnter(line);
        if (isBlank(line)) continue;
        safeCopy(lineCopy, sizeof(lineCopy), line);
        n = splitLine(lineCopy, f, 10);
        if (n == 10 && parseMoneyStrict(f[6], &fee) && parseIntStrict(f[9], &regStatus) &&
            (regStatus == 0 || regStatus == 1)) {
            code = addMedicalRecord(f[0], f[1], f[2], f[3], f[4], f[5], fee, f[7], f[8], regStatus);
        } else {
            safeCopy(lineCopy, sizeof(lineCopy), line);
            if (splitLine(lineCopy, f, 9) != 9 || !parseMoneyStrict(f[6], &fee)) {
                snprintf(msg, msgSize, "record.txt 第%d行格式错误。", row);
                fclose(fp);
                return ERR_FILE;
            }
            code = addMedicalRecord(f[0], f[1], f[2], f[3], f[4], f[5], fee, f[7], f[8], 1);
        }
        if (code != OK) {
            snprintf(msg, msgSize, "record.txt 第%d行错误：%s。", row, codeText(code));
            fclose(fp);
            return ERR_FILE;
        }
    }
    fclose(fp);
    return OK;
}

static int countPipes(const char *line)
{
    int n = 0;
    while (*line) {
        if (*line == '|') n++;
        line++;
    }
    return n;
}

static int loadPrescriptions(const char *path, char *msg, int msgSize)
{
    FILE *fp = fopen(path, "r");
    char line[LINE_LEN];
    char *f[10];
    int row = 0;
    int code;
    int fieldCount;
    int isNewFormat;
    Prescription *rx;
    if (fp == NULL) return OK;
    while (fgets(line, sizeof(line), fp) != NULL) {
        row++;
        removeEnter(line);
        if (isBlank(line)) continue;
        isNewFormat = (countPipes(line) == 9);
        fieldCount = isNewFormat ? 10 : 8;
        if (splitLine(line, f, fieldCount) != fieldCount) {
            snprintf(msg, msgSize, "prescription.txt 第%d行字段数量错误。", row);
            fclose(fp);
            return ERR_FILE;
        }
        code = addPrescription(f[0], f[1], f[2], f[3], f[4],
            isNewFormat ? f[5] : "", isNewFormat ? f[6] : "");
        if (code != OK) {
            snprintf(msg, msgSize, "prescription.txt 第%d行错误：%s。", row, codeText(code));
            fclose(fp);
            return ERR_FILE;
        }
        rx = findPrescriptionById(f[0]);
        if (rx != NULL) {
            if (isNewFormat) {
                if (!parseIntStrict(f[7], &rx->itemCount)) {
                    snprintf(msg, msgSize, "prescription.txt 第%d行药品数错误。", row);
                    fclose(fp);
                    return ERR_FILE;
                }
                if (!parseMoneyStrict(f[8], &rx->totalFee)) {
                    snprintf(msg, msgSize, "prescription.txt 第%d行总金额错误。", row);
                    fclose(fp);
                    return ERR_FILE;
                }
                safeCopy(rx->status, STATUS_LEN, f[9]);
            } else {
                if (!parseIntStrict(f[5], &rx->itemCount)) {
                    snprintf(msg, msgSize, "prescription.txt 第%d行药品数错误。", row);
                    fclose(fp);
                    return ERR_FILE;
                }
                if (!parseMoneyStrict(f[6], &rx->totalFee)) {
                    snprintf(msg, msgSize, "prescription.txt 第%d行总金额错误。", row);
                    fclose(fp);
                    return ERR_FILE;
                }
                safeCopy(rx->status, STATUS_LEN, f[7]);
            }
        }
    }
    fclose(fp);
    return OK;
}

static int loadPrescriptionItems(const char *path, char *msg, int msgSize)
{
    FILE *fp = fopen(path, "r");
    char line[LINE_LEN];
    char *f[4];
    int row = 0;
    int count;
    int code;
    if (fp == NULL) return OK;
    while (fgets(line, sizeof(line), fp) != NULL) {
        row++;
        removeEnter(line);
        if (isBlank(line)) continue;
        if (splitLine(line, f, 4) != 4 || !parseIntStrict(f[2], &count)) {
            snprintf(msg, msgSize, "prescription_item.txt 第%d行格式错误。", row);
            fclose(fp);
            return ERR_FILE;
        }
        code = addPrescriptionItem(f[0], f[1], count);
        if (code != OK) {
            snprintf(msg, msgSize, "prescription_item.txt 第%d行错误：%s。", row, codeText(code));
            fclose(fp);
            return ERR_FILE;
        }
    }
    fclose(fp);
    return OK;
}

static int loadPayments(const char *path, char *msg, int msgSize)
{
    FILE *fp = fopen(path, "r");
    char line[LINE_LEN];
    char *f[8];
    int row = 0;
    double amount;
    int code;
    if (fp == NULL) return OK;
    while (fgets(line, sizeof(line), fp) != NULL) {
        row++;
        removeEnter(line);
        if (isBlank(line)) continue;
        if (splitLine(line, f, 8) != 8 || !parseMoneyStrict(f[4], &amount)) {
            snprintf(msg, msgSize, "payment.txt 第%d行格式错误。", row);
            fclose(fp);
            return ERR_FILE;
        }
        code = addPayment(f[0], f[1], f[2], f[3], amount, f[5], f[6], f[7]);
        if (code != OK) {
            snprintf(msg, msgSize, "payment.txt 第%d行错误：%s。", row, codeText(code));
            fclose(fp);
            return ERR_FILE;
        }
    }
    fclose(fp);
    return OK;
}

int loadAllData(const char *dir, char *msg, int msgSize)
{
    char path[TEXT_LEN];
    clearText(msg, msgSize);
    clearAllData();
    // 答辩注意：读文件按依赖顺序读，先读科室和患者，后读记录和缴费（难度：中等）
    makePath(path, sizeof(path), dir, "department.txt");
    if (loadDepartments(path, msg, msgSize) != OK) { clearAllData(); return ERR_FILE; }
    makePath(path, sizeof(path), dir, "patient.txt");
    if (loadPatients(path, msg, msgSize) != OK) { clearAllData(); return ERR_FILE; }
    makePath(path, sizeof(path), dir, "doctor.txt");
    if (loadDoctors(path, msg, msgSize) != OK) { clearAllData(); return ERR_FILE; }
    makePath(path, sizeof(path), dir, "medicine.txt");
    if (loadMedicines(path, msg, msgSize) != OK) { clearAllData(); return ERR_FILE; }
    makePath(path, sizeof(path), dir, "ward.txt");
    if (loadWards(path, msg, msgSize) != OK) { clearAllData(); return ERR_FILE; }
    makePath(path, sizeof(path), dir, "bed.txt");
    if (loadBeds(path, msg, msgSize) != OK) { clearAllData(); return ERR_FILE; }
    makePath(path, sizeof(path), dir, "record.txt");
    if (loadRecords(path, msg, msgSize) != OK) { clearAllData(); return ERR_FILE; }
    makePath(path, sizeof(path), dir, "prescription.txt");
    if (loadPrescriptions(path, msg, msgSize) != OK) { clearAllData(); return ERR_FILE; }
    makePath(path, sizeof(path), dir, "prescription_item.txt");
    if (loadPrescriptionItems(path, msg, msgSize) != OK) { clearAllData(); return ERR_FILE; }
    makePath(path, sizeof(path), dir, "payment.txt");
    if (loadPayments(path, msg, msgSize) != OK) { clearAllData(); return ERR_FILE; }
    snprintf(msg, msgSize, "全部数据读取完成。");
    return OK;
}

static void saveDepartments(FILE *fp)
{
    Department *p = departmentHead;
    while (p != NULL) {
        fprintf(fp, "%s|%s|%s|%s\n", p->id, p->name, p->intro, p->wardType);
        p = p->next;
    }
}

static void savePatients(FILE *fp)
{
    Patient *p = patientHead;
    while (p != NULL) {
        fprintf(fp, "%s|%s|%s|%d|%s|%s|%s\n",
            p->id, p->name, p->gender, p->age, p->phone, p->cardId, p->status);
        p = p->next;
    }
}

static void saveDoctors(FILE *fp)
{
    Doctor *p = doctorHead;
    while (p != NULL) {
        fprintf(fp, "%s|%s|%s|%s|%s|%d\n",
            p->id, p->name, p->deptId, p->title, p->workTime, p->maxCount);
        p = p->next;
    }
}

static void saveMedicines(FILE *fp)
{
    Medicine *p = medicineHead;
    while (p != NULL) {
        fprintf(fp, "%s|%s|%s|%s|%s|%s|%.2f|%d|%d\n",
            p->id, p->name, p->commonName, p->tradeName, p->alias,
            p->deptId, p->price, p->stock, p->lowLimit);
        p = p->next;
    }
}

static void saveWards(FILE *fp)
{
    Ward *p = wardHead;
    while (p != NULL) {
        fprintf(fp, "%s|%s|%s|%.2f|%d|%d\n",
            p->id, p->type, p->deptId, p->dailyFee, p->totalBeds, p->freeBeds);
        p = p->next;
    }
}

static void saveBeds(FILE *fp)
{
    Bed *p = bedHead;
    while (p != NULL) {
        fprintf(fp, "%s|%s|%s|%s|%s\n", p->id, p->wardId, p->status, p->patientId, p->inDate);
        p = p->next;
    }
}

static void saveRecords(FILE *fp)
{
    MedicalRecord *p = recordHead;
    while (p != NULL) {
        fprintf(fp, "%s|%s|%s|%s|%s|%s|%.2f|%s|%s|%d\n",
            p->id, p->patientId, p->doctorId, p->deptId, p->type, p->date,
            p->fee, p->relatedId, p->note, p->regStatus);
        p = p->next;
    }
}

static void savePrescriptions(FILE *fp)
{
    Prescription *p = prescriptionHead;
    while (p != NULL) {
        fprintf(fp, "%s|%s|%s|%s|%s|%s|%s|%d|%.2f|%s\n",
            p->id, p->patientId, p->doctorId, p->deptId, p->date,
            p->medicalRecordId, p->advice,
            p->itemCount, p->totalFee, p->status);
        p = p->next;
    }
}

static void savePrescriptionItems(FILE *fp)
{
    PrescriptionItem *p = prescriptionItemHead;
    while (p != NULL) {
        fprintf(fp, "%s|%s|%d|%.2f\n", p->prescriptionId, p->medicineId, p->count, p->price);
        p = p->next;
    }
}

static void savePayments(FILE *fp)
{
    Payment *p = paymentHead;
    while (p != NULL) {
        fprintf(fp, "%s|%s|%s|%s|%.2f|%s|%s|%s\n",
            p->id, p->patientId, p->sourceType, p->sourceId,
            p->amount, p->status, p->date, p->note);
        p = p->next;
    }
}

static int saveOne(const char *dir, const char *name, void (*saveFunc)(FILE *))
{
    char path[TEXT_LEN];
    FILE *fp;
    makePath(path, sizeof(path), dir, name);
    fp = fopen(path, "w");
    if (fp == NULL) return ERR_FILE;
    saveFunc(fp);
    fclose(fp);
    return OK;
}

int saveAllData(const char *dir, char *msg, int msgSize)
{
    if (saveOne(dir, "department.txt", saveDepartments) != OK ||
        saveOne(dir, "patient.txt", savePatients) != OK ||
        saveOne(dir, "doctor.txt", saveDoctors) != OK ||
        saveOne(dir, "medicine.txt", saveMedicines) != OK ||
        saveOne(dir, "ward.txt", saveWards) != OK ||
        saveOne(dir, "bed.txt", saveBeds) != OK ||
        saveOne(dir, "record.txt", saveRecords) != OK ||
        saveOne(dir, "prescription.txt", savePrescriptions) != OK ||
        saveOne(dir, "prescription_item.txt", savePrescriptionItems) != OK ||
        saveOne(dir, "payment.txt", savePayments) != OK) {
        snprintf(msg, msgSize, "保存失败，请检查 data 文件夹是否存在。");
        return ERR_FILE;
    }
    snprintf(msg, msgSize, "全部数据保存完成。");
    return OK;
}
