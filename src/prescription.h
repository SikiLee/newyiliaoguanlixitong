#ifndef PRESCRIPTION_H
#define PRESCRIPTION_H

#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

void makeNextPrescriptionId(char *out, int outSize);
Prescription *findPrescriptionById(const char *id);
int addPrescription(const char *id, const char *patientId, const char *doctorId,
    const char *deptId, const char *date, const char *medicalRecordId, const char *advice);
int setPrescriptionMedicalRecordId(const char *id, const char *medicalRecordId);
int addPrescriptionItem(const char *prescriptionId, const char *medicineId, int count);
int setPrescriptionPaid(const char *id);
int sendPrescription(const char *id);
void listPrescriptions(char *out, int outSize);
void listPrescriptionItems(const char *prescriptionId, char *out, int outSize);
void listPrescriptionItemsForPatient(const char *prescriptionId, char *out, int outSize);
int collectPrescriptionsByPatient(const char *patientId, Prescription *list[], int max);
int deletePrescriptionById(const char *id);
int deletePrescriptionItemsByRxId(const char *prescriptionId);

#ifdef __cplusplus
}
#endif

#endif
