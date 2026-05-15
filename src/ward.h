#ifndef WARD_H
#define WARD_H

#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

void makeNextWardId(char *out, int outSize);
void makeNextBedId(char *out, int outSize);
Ward *findWardById(const char *id);
Bed *findBedById(const char *id);
int addWard(const char *id, const char *type, const char *deptId, double dailyFee,
    int totalBeds, int freeBeds);
int updateWard(const char *id, double dailyFee);
int deleteWard(const char *id);
int addBed(const char *id, const char *wardId, const char *status, const char *patientId,
    const char *inDate);
int deleteBed(const char *id);
Bed *findFreeBed(const char *deptId, const char *wardType);
int occupyBed(const char *bedId, const char *patientId, const char *inDate);
int releasePatientBed(const char *patientId);
Bed *findBedByPatient(const char *patientId);
void listWards(char *out, int outSize);
void listBeds(char *out, int outSize);

#ifdef __cplusplus
}
#endif

#endif
