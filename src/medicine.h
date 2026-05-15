#ifndef MEDICINE_H
#define MEDICINE_H

#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

void makeNextMedicineId(char *out, int outSize);
Medicine *findMedicineById(const char *id);
int collectMedicinesByName(const char *name, Medicine *list[], int maxCount);
int addMedicine(const char *id, const char *name, const char *commonName,
    const char *tradeName, const char *alias, const char *deptId,
    double price, int stock, int lowLimit);
int updateMedicine(const char *id, double price, int stock, int lowLimit);
int deleteMedicine(const char *id);
int reduceMedicineStock(const char *id, int count);
void listMedicines(char *out, int outSize);

#ifdef __cplusplus
}
#endif

#endif
