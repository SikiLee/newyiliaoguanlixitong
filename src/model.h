#ifndef MODEL_H
#define MODEL_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Patient {
    char id[ID_LEN];
    char name[NAME_LEN];
    char gender[STATUS_LEN];
    int age;
    char phone[PHONE_LEN];
    char cardId[NAME_LEN];
    char status[STATUS_LEN];
    struct Patient *next;
} Patient;

typedef struct Doctor {
    char id[ID_LEN];
    char name[NAME_LEN];
    char deptId[ID_LEN];
    char title[NAME_LEN];
    char workTime[NAME_LEN];
    int maxCount;
    struct Doctor *next;
} Doctor;

typedef struct Department {
    char id[ID_LEN];
    char name[NAME_LEN];
    char intro[TEXT_LEN];
    char wardType[NAME_LEN];
    struct Department *next;
} Department;

typedef struct Medicine {
    char id[ID_LEN];
    char name[NAME_LEN];
    char commonName[NAME_LEN];
    char tradeName[NAME_LEN];
    char alias[NAME_LEN];
    char deptId[ID_LEN];
    double price;
    int stock;
    int lowLimit;
    struct Medicine *next;
} Medicine;

typedef struct Ward {
    char id[ID_LEN];
    char type[NAME_LEN];
    char deptId[ID_LEN];
    double dailyFee;
    int totalBeds;
    int freeBeds;
    struct Ward *next;
} Ward;

typedef struct Bed {
    char id[ID_LEN];
    char wardId[ID_LEN];
    char status[STATUS_LEN];
    char patientId[ID_LEN];
    char inDate[DATE_LEN];
    struct Bed *next;
} Bed;

typedef struct MedicalRecord {
    char id[ID_LEN];
    char patientId[ID_LEN];
    char doctorId[ID_LEN];
    char deptId[ID_LEN];
    char type[STATUS_LEN];
    char date[DATE_LEN];
    double fee;
    char relatedId[ID_LEN];
    char note[TEXT_LEN];
    int regStatus;
    struct MedicalRecord *next;
} MedicalRecord;

typedef struct Prescription {
    char id[ID_LEN];
    char patientId[ID_LEN];
    char doctorId[ID_LEN];
    char deptId[ID_LEN];
    char date[DATE_LEN];
    char medicalRecordId[ID_LEN];
    char advice[TEXT_LEN];
    int itemCount;
    double totalFee;
    char status[STATUS_LEN];
    struct Prescription *next;
} Prescription;

typedef struct PrescriptionItem {
    char prescriptionId[ID_LEN];
    char medicineId[ID_LEN];
    int count;
    double price;
    struct PrescriptionItem *next;
} PrescriptionItem;

typedef struct Payment {
    char id[ID_LEN];
    char patientId[ID_LEN];
    char sourceType[STATUS_LEN];
    char sourceId[ID_LEN];
    double amount;
    char status[STATUS_LEN];
    char date[DATE_LEN];
    char note[TEXT_LEN];
    struct Payment *next;
} Payment;

#ifdef __cplusplus
}
#endif

#endif
