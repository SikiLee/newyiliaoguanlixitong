#include "data.h"

Patient *patientHead = NULL;
Doctor *doctorHead = NULL;
Department *departmentHead = NULL;
Medicine *medicineHead = NULL;
Ward *wardHead = NULL;
Bed *bedHead = NULL;
MedicalRecord *recordHead = NULL;
Prescription *prescriptionHead = NULL;
PrescriptionItem *prescriptionItemHead = NULL;
Payment *paymentHead = NULL;

void clearAllData(void)
{
    Patient *p1;
    Doctor *p2;
    Department *p3;
    Medicine *p4;
    Ward *p5;
    Bed *p6;
    MedicalRecord *p7;
    Prescription *p8;
    PrescriptionItem *p9;
    Payment *p10;

    while (patientHead != NULL) {
        p1 = patientHead;
        patientHead = patientHead->next;
        free(p1);
    }
    while (doctorHead != NULL) {
        p2 = doctorHead;
        doctorHead = doctorHead->next;
        free(p2);
    }
    while (departmentHead != NULL) {
        p3 = departmentHead;
        departmentHead = departmentHead->next;
        free(p3);
    }
    while (medicineHead != NULL) {
        p4 = medicineHead;
        medicineHead = medicineHead->next;
        free(p4);
    }
    while (wardHead != NULL) {
        p5 = wardHead;
        wardHead = wardHead->next;
        free(p5);
    }
    while (bedHead != NULL) {
        p6 = bedHead;
        bedHead = bedHead->next;
        free(p6);
    }
    while (recordHead != NULL) {
        p7 = recordHead;
        recordHead = recordHead->next;
        free(p7);
    }
    while (prescriptionHead != NULL) {
        p8 = prescriptionHead;
        prescriptionHead = prescriptionHead->next;
        free(p8);
    }
    while (prescriptionItemHead != NULL) {
        p9 = prescriptionItemHead;
        prescriptionItemHead = prescriptionItemHead->next;
        free(p9);
    }
    while (paymentHead != NULL) {
        p10 = paymentHead;
        paymentHead = paymentHead->next;
        free(p10);
    }
}
