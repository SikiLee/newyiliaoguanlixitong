#ifndef PATIENT_H
#define PATIENT_H

#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

void makeNextPatientId(char *out, int outSize);
Patient *findPatientById(const char *id);
Patient *findPatientByCardId(const char *cardId);
int addPatient(const char *id, const char *name, const char *gender, int age,
    const char *phone, const char *cardId, const char *status);
int updatePatient(const char *id, const char *phone, const char *status);
int updatePatientStatus(const char *id, const char *status);
int deletePatient(const char *id);
int collectPatientsByName(const char *name, Patient *list[], int maxCount);
void listPatients(char *out, int outSize);
void queryPatientByName(const char *name, char *out, int outSize);

#ifdef __cplusplus
}
#endif

#endif
