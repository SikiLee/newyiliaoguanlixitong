#ifndef DATA_H
#define DATA_H

#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

extern Patient *patientHead;
extern Doctor *doctorHead;
extern Department *departmentHead;
extern Medicine *medicineHead;
extern Ward *wardHead;
extern Bed *bedHead;
extern MedicalRecord *recordHead;
extern Prescription *prescriptionHead;
extern PrescriptionItem *prescriptionItemHead;
extern Payment *paymentHead;

void clearAllData(void);

#ifdef __cplusplus
}
#endif

#endif
