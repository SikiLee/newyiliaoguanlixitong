#ifndef DOCTOR_H
#define DOCTOR_H

#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

void makeNextDoctorId(char *out, int outSize);
Doctor *findDoctorById(const char *id);
int collectDoctorsByName(const char *name, Doctor *list[], int maxCount);
int addDoctor(const char *id, const char *name, const char *deptId, const char *title,
    const char *workTime, int maxCount);
int updateDoctor(const char *id, const char *title, const char *workTime, int maxCount);
int deleteDoctor(const char *id);
void listDoctors(char *out, int outSize);

#ifdef __cplusplus
}
#endif

#endif
