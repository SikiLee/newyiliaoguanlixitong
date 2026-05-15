#include "report.h"
#include "data.h"
#include "doctor.h"
#include "department.h"
#include "medicine.h"
#include "ward.h"

void reportIncome(char *out, int outSize)
{
    Payment *p = paymentHead;
    double registerFee = 0;
    double checkFee = 0;
    double medicineFee = 0;
    double hospitalFee = 0;
    double otherFee = 0;
    char line[LINE_LEN];
    clearText(out, outSize);
    while (p != NULL) {
        if (strcmp(p->status, "已缴费") == 0) {
            if (strcmp(p->sourceType, "挂号") == 0) registerFee += p->amount;
            else if (strcmp(p->sourceType, "检查") == 0) checkFee += p->amount;
            else if (strcmp(p->sourceType, "处方") == 0) medicineFee += p->amount;
            else if (strcmp(p->sourceType, "住院") == 0) hospitalFee += p->amount;
            else otherFee += p->amount;
        }
        p = p->next;
    }
    appendLine(out, outSize, "医院收入统计");
    snprintf(line, sizeof(line), "挂号费：%.2f\n检查费：%.2f\n药费：%.2f\n住院费：%.2f\n其他费用：%.2f\n总收入：%.2f\n",
        registerFee, checkFee, medicineFee, hospitalFee, otherFee,
        registerFee + checkFee + medicineFee + hospitalFee + otherFee);
    appendText(out, outSize, line);
}

void reportBedUse(char *out, int outSize)
{
    Ward *w = wardHead;
    char line[LINE_LEN];
    clearText(out, outSize);
    appendLine(out, outSize, "床位使用统计");
    appendLine(out, outSize, "病房编号 | 类型 | 总床位 | 空床 | 使用率");
    while (w != NULL) {
        double rate = 0;
        if (w->totalBeds > 0) {
            rate = (double)(w->totalBeds - w->freeBeds) * 100.0 / w->totalBeds;
        }
        snprintf(line, sizeof(line), "%s | %s | %d | %d | %.1f%%\n",
            w->id, w->type, w->totalBeds, w->freeBeds, rate);
        appendText(out, outSize, line);
        w = w->next;
    }
}

void reportMedicineStock(char *out, int outSize)
{
    Medicine *m = medicineHead;
    char line[LINE_LEN];
    clearText(out, outSize);
    appendLine(out, outSize, "药品库存统计");
    appendLine(out, outSize, "药品编号 | 药品名 | 库存 | 下限 | 状态");
    while (m != NULL) {
        snprintf(line, sizeof(line), "%s | %s | %d | %d | %s\n",
            m->id, m->name, m->stock, m->lowLimit, m->stock <= m->lowLimit ? "库存偏低" : "正常");
        appendText(out, outSize, line);
        m = m->next;
    }
}

void reportDoctorWork(char *out, int outSize)
{
    Doctor *d = doctorHead;
    MedicalRecord *r;
    char line[LINE_LEN];
    int count;
    clearText(out, outSize);
    appendLine(out, outSize, "医生工作量统计");
    appendLine(out, outSize, "医生编号 | 姓名 | 记录数");
    while (d != NULL) {
        count = 0;
        r = recordHead;
        while (r != NULL) {
            if (strcmp(r->doctorId, d->id) == 0) count++;
            r = r->next;
        }
        snprintf(line, sizeof(line), "%s | %s | %d\n", d->id, d->name, count);
        appendText(out, outSize, line);
        d = d->next;
    }
}

void reportDepartmentWork(char *out, int outSize)
{
    Department *d = departmentHead;
    MedicalRecord *r;
    char line[LINE_LEN];
    int count;
    clearText(out, outSize);
    appendLine(out, outSize, "科室工作量统计");
    appendLine(out, outSize, "科室编号 | 科室名 | 记录数");
    while (d != NULL) {
        count = 0;
        r = recordHead;
        while (r != NULL) {
            if (strcmp(r->deptId, d->id) == 0) count++;
            r = r->next;
        }
        snprintf(line, sizeof(line), "%s | %s | %d\n", d->id, d->name, count);
        appendText(out, outSize, line);
        d = d->next;
    }
}
