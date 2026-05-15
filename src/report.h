#ifndef REPORT_H
#define REPORT_H

#ifdef __cplusplus
extern "C" {
#endif

void reportIncome(char *out, int outSize);
void reportBedUse(char *out, int outSize);
void reportMedicineStock(char *out, int outSize);
void reportDoctorWork(char *out, int outSize);
void reportDepartmentWork(char *out, int outSize);

#ifdef __cplusplus
}
#endif

#endif
