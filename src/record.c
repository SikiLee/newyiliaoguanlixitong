#include "record.h"
#include "patient.h"
#include "doctor.h"
#include "department.h"
#include "data.h"

void makeNextRecordId(char *out, int outSize)
{
    MedicalRecord *p = recordHead;
    int max = 0;
    int num;
    while (p != NULL) {
        num = getIdNumber(p->id, "REC");
        if (num > max) max = num;
        p = p->next;
    }
    makeId(out, outSize, "REC", max + 1);
}

MedicalRecord *findRecordById(const char *id)
{
    MedicalRecord *p = recordHead;
    while (p != NULL) {
        if (strcmp(p->id, id) == 0) return p;
        p = p->next;
    }
    return NULL;
}

int addMedicalRecord(const char *id, const char *patientId, const char *doctorId,
    const char *deptId, const char *type, const char *date, double fee,
    const char *relatedId, const char *note, int regStatus)
{
    MedicalRecord *node;
    MedicalRecord *p;
    int rs = regStatus;
    if (isBlank(id) || findPatientById(patientId) == NULL ||
        findDepartmentById(deptId) == NULL || isBlank(type) || hasBadChar(type) ||
        !checkDate(date) || !checkMoney(fee) || hasBadChar(note)) {
        return ERR_INPUT;
    }
    if (strcmp(type, "挂号") != 0) {
        rs = 1;
    } else if (rs != 0 && rs != 1) {
        return ERR_INPUT;
    }
    if (!isBlank(doctorId) && findDoctorById(doctorId) == NULL) {
        return ERR_INPUT;
    }
    if (findRecordById(id) != NULL) return ERR_REPEAT;
    node = (MedicalRecord *)malloc(sizeof(MedicalRecord));
    if (node == NULL) return ERR_FILE;
    safeCopy(node->id, ID_LEN, id);
    safeCopy(node->patientId, ID_LEN, patientId);
    safeCopy(node->doctorId, ID_LEN, doctorId);
    safeCopy(node->deptId, ID_LEN, deptId);
    safeCopy(node->type, STATUS_LEN, type);
    safeCopy(node->date, DATE_LEN, date);
    node->fee = fee;
    safeCopy(node->relatedId, ID_LEN, relatedId);
    safeCopy(node->note, TEXT_LEN, note);
    node->regStatus = rs;
    node->next = NULL;
    if (recordHead == NULL) {
        recordHead = node;
    } else {
        p = recordHead;
        while (p->next != NULL) p = p->next;
        p->next = node;
    }
    return OK;
}

static void appendRecordLine(MedicalRecord *r, char *out, int outSize)
{
    Patient *patient = findPatientById(r->patientId);
    Doctor *doctor = findDoctorById(r->doctorId);
    Department *dept = findDepartmentById(r->deptId);
    char line[LINE_LEN];
    snprintf(line, sizeof(line), "%s | %s | %s | %s | %s | %s | %.2f | %s\n",
        r->id, patient ? patient->name : "未知患者", doctor ? doctor->name : "-",
        dept ? dept->name : "未知科室", r->type, r->date, r->fee, r->note);
    appendText(out, outSize, line);
}

void listRecords(char *out, int outSize)
{
    MedicalRecord *p = recordHead;
    clearText(out, outSize);
    appendLine(out, outSize, "记录编号 | 患者 | 医生 | 科室 | 类型 | 日期 | 费用 | 说明");
    while (p != NULL) {
        appendRecordLine(p, out, outSize);
        p = p->next;
    }
}

void listRecordsByPatient(const char *patientId, char *out, int outSize)
{
    MedicalRecord *p = recordHead;
    int count = 0;
    clearText(out, outSize);
    appendLine(out, outSize, "记录编号 | 患者 | 医生 | 科室 | 类型 | 日期 | 费用 | 说明");
    while (p != NULL) {
        if (strcmp(p->patientId, patientId) == 0) {
            appendRecordLine(p, out, outSize);
            count++;
        }
        p = p->next;
    }
    if (count == 0) appendLine(out, outSize, "该患者没有医疗记录。");
}

void listRecordsByType(const char *type, char *out, int outSize)
{
    MedicalRecord *p = recordHead;
    int count = 0;
    clearText(out, outSize);
    appendLine(out, outSize, "记录编号 | 患者 | 医生 | 科室 | 类型 | 日期 | 费用 | 说明");
    while (p != NULL) {
        if (strcmp(p->type, type) == 0) {
            appendRecordLine(p, out, outSize);
            count++;
        }
        p = p->next;
    }
    if (count == 0) appendLine(out, outSize, "没有这种类型的记录。");
}

int countRegisterRecordsByDoctor(const char *doctorId)
{
    MedicalRecord *p = recordHead;
    int n = 0;
    if (doctorId == NULL || isBlank(doctorId)) return 0;
    while (p != NULL) {
        if (strcmp(p->type, "挂号") == 0 && strcmp(p->doctorId, doctorId) == 0) {
            n++;
        }
        p = p->next;
    }
    return n;
}

int hasPatientDeptRegisterOnDate(const char *patientId, const char *deptId, const char *date)
{
    MedicalRecord *p = recordHead;
    if (patientId == NULL || deptId == NULL || date == NULL) return 0;
    while (p != NULL) {
        if (strcmp(p->patientId, patientId) == 0 && strcmp(p->deptId, deptId) == 0 &&
            strcmp(p->type, "挂号") == 0 && strcmp(p->date, date) == 0) {
            return 1;
        }
        p = p->next;
    }
    return 0;
}

static int dateInRangeForPatient(const char *d, const char *begin, const char *end)
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

int patientHasVisitOnDate(const char *patientId, const char *date)
{
    MedicalRecord *p = recordHead;
    if (patientId == NULL || date == NULL || !checkDate(date)) return 0;
    while (p != NULL) {
        if (strcmp(p->patientId, patientId) == 0 && strcmp(p->type, "看诊") == 0 &&
            strcmp(p->date, date) == 0) {
            return 1;
        }
        p = p->next;
    }
    return 0;
}

MedicalRecord *findLatestRegisterByPatient(const char *patientId)
{
    MedicalRecord *p = recordHead;
    MedicalRecord *latest = NULL;
    if (patientId == NULL) return NULL;
    while (p != NULL) {
        if (strcmp(p->patientId, patientId) == 0 && strcmp(p->type, "挂号") == 0 &&
            p->regStatus == 1) {
            if (latest == NULL || strcmp(p->date, latest->date) >= 0) {
                latest = p;
            }
        }
        p = p->next;
    }
    return latest;
}

MedicalRecord *findLatestRegisterByPatientDoctor(const char *patientId, const char *doctorId)
{
    MedicalRecord *p = recordHead;
    MedicalRecord *latest = NULL;
    int pNum;
    int latestNum;
    if (patientId == NULL || doctorId == NULL || isBlank(doctorId)) return NULL;
    while (p != NULL) {
        if (strcmp(p->patientId, patientId) == 0 && strcmp(p->doctorId, doctorId) == 0 &&
            strcmp(p->type, "挂号") == 0 && p->regStatus == 1) {
            if (latest == NULL) {
                latest = p;
            } else {
                pNum = getIdNumber(p->id, "REC");
                latestNum = getIdNumber(latest->id, "REC");
                if (compareDate(p->date, latest->date) > 0 ||
                    (strcmp(p->date, latest->date) == 0 && pNum >= latestNum)) {
                    latest = p;
                }
            }
        }
        p = p->next;
    }
    return latest;
}

MedicalRecord *findLatestVisitByPatient(const char *patientId)
{
    MedicalRecord *p = recordHead;
    MedicalRecord *latest = NULL;
    int pNum;
    int latestNum;
    if (patientId == NULL) return NULL;
    while (p != NULL) {
        if (strcmp(p->patientId, patientId) == 0 && strcmp(p->type, "看诊") == 0) {
            if (latest == NULL) {
                latest = p;
            } else {
                pNum = getIdNumber(p->id, "REC");
                latestNum = getIdNumber(latest->id, "REC");
                if (compareDate(p->date, latest->date) > 0 ||
                    (strcmp(p->date, latest->date) == 0 && pNum >= latestNum)) {
                    latest = p;
                }
            }
        }
        p = p->next;
    }
    return latest;
}

MedicalRecord *findLatestVisitByPatientDoctor(const char *patientId, const char *doctorId)
{
    MedicalRecord *p = recordHead;
    MedicalRecord *latest = NULL;
    int pNum;
    int latestNum;
    if (patientId == NULL || doctorId == NULL || isBlank(doctorId)) return NULL;
    while (p != NULL) {
        if (strcmp(p->patientId, patientId) == 0 && strcmp(p->doctorId, doctorId) == 0 &&
            strcmp(p->type, "看诊") == 0) {
            if (latest == NULL) {
                latest = p;
            } else {
                pNum = getIdNumber(p->id, "REC");
                latestNum = getIdNumber(latest->id, "REC");
                if (compareDate(p->date, latest->date) > 0 ||
                    (strcmp(p->date, latest->date) == 0 && pNum >= latestNum)) {
                    latest = p;
                }
            }
        }
        p = p->next;
    }
    return latest;
}

static int isVisitRecordType(const char *type)
{
    if (type == NULL) return 0;
    return strcmp(type, "看诊") == 0 || strcmp(type, "检查") == 0 ||
        strcmp(type, "入院") == 0 || strcmp(type, "出院") == 0;
}

static void appendPatientViewRecordLine(MedicalRecord *r, char *out, int outSize)
{
    Doctor *doctor = findDoctorById(r->doctorId);
    Department *dept = findDepartmentById(r->deptId);
    char line[LINE_LEN];
    const char *docName = doctor ? doctor->name : "-";
    const char *deptName = dept ? dept->name : "未知科室";
    if (strcmp(r->type, "挂号") == 0) {
        char stBuf[48];
        const char *rsText = (r->regStatus == 0) ? "预约" : "已挂号";
        const char *st = patientHasVisitOnDate(r->patientId, r->date) ? "已就诊" : "待就诊";
        snprintf(stBuf, sizeof(stBuf), "%s·%s", rsText, st);
        snprintf(line, sizeof(line), "%s | %s | %s | %s | %.2f | %s | %s\n",
            r->date, deptName, docName, r->type, r->fee, r->note, stBuf);
    } else {
        snprintf(line, sizeof(line), "%s | %s | %s | %s | %.2f | %s | -\n",
            r->date, deptName, docName, r->type, r->fee, r->note);
    }
    appendText(out, outSize, line);
}

void listPatientMedicalPatientView(const char *patientId, const char *mode,
    const char *visitSubtype, const char *dateBegin, const char *dateEnd,
    char *out, int outSize)
{
    MedicalRecord *p = recordHead;
    int count = 0;
    int include;
    if (patientId == NULL || mode == NULL) {
        clearText(out, outSize);
        appendLine(out, outSize, "参数错误。");
        return;
    }
    clearText(out, outSize);
    appendLine(out, outSize, "日期 | 科室 | 医生 | 类型 | 费用 | 说明 | 状态");
    while (p != NULL) {
        if (strcmp(p->patientId, patientId) != 0) {
            p = p->next;
            continue;
        }
        if (!dateInRangeForPatient(p->date, dateBegin, dateEnd)) {
            p = p->next;
            continue;
        }
        include = 0;
        if (strcmp(mode, "register") == 0) {
            include = (strcmp(p->type, "挂号") == 0);
        } else if (strcmp(mode, "visit") == 0) {
            include = isVisitRecordType(p->type);
            if (include && visitSubtype != NULL && !isBlank(visitSubtype) &&
                strcmp(p->type, visitSubtype) != 0) {
                include = 0;
            }
        }
        if (include) {
            appendPatientViewRecordLine(p, out, outSize);
            count++;
        }
        p = p->next;
    }
    if (count == 0) {
        if (strcmp(mode, "register") == 0) {
            appendLine(out, outSize, "暂无挂号记录。");
        } else {
            appendLine(out, outSize, "暂无就诊类记录（看诊/检查/入院/出院）。");
        }
    }
}
