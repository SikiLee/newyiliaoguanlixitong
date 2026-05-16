#ifndef RECORD_H
#define RECORD_H

#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

void makeNextRecordId(char *out, int outSize);
MedicalRecord *findRecordById(const char *id);
int addMedicalRecord(const char *id, const char *patientId, const char *doctorId,
    const char *deptId, const char *type, const char *date, double fee,
    const char *relatedId, const char *note, int regStatus);
void listRecords(char *out, int outSize);
void listRecordsByPatient(const char *patientId, char *out, int outSize);
void listRecordsByType(const char *type, char *out, int outSize);
int countRegisterRecordsByDoctor(const char *doctorId);
int hasPatientDeptRegisterOnDate(const char *patientId, const char *deptId, const char *date);
int patientHasVisitOnDate(const char *patientId, const char *date);
MedicalRecord *findLatestRegisterByPatient(const char *patientId);
MedicalRecord *findLatestRegisterByPatientDoctor(const char *patientId, const char *doctorId);
MedicalRecord *findLatestVisitByPatient(const char *patientId);
MedicalRecord *findLatestVisitByPatientDoctor(const char *patientId, const char *doctorId);
void listPatientMedicalPatientView(const char *patientId, const char *mode,
    const char *visitSubtype, const char *dateBegin, const char *dateEnd,
    char *out, int outSize);

#ifdef __cplusplus
}
#endif

#endif
