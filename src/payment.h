#ifndef PAYMENT_H
#define PAYMENT_H

#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

void makeNextPaymentId(char *out, int outSize);
Payment *findPaymentById(const char *id);
Payment *findPaymentBySource(const char *sourceType, const char *sourceId);
int addPayment(const char *id, const char *patientId, const char *sourceType,
    const char *sourceId, double amount, const char *status, const char *date, const char *note);
int deletePaymentById(const char *id);
int payPayment(const char *id, const char *date);
int isSourcePaid(const char *sourceType, const char *sourceId);
void listPayments(char *out, int outSize);
void listPaymentsByPatient(const char *patientId, char *out, int outSize);
void paymentSummaryByPatient(const char *patientId, char *out, int outSize);
void paymentSummaryByPatientInRange(const char *patientId,
    const char *dateBegin, const char *dateEnd, char *out, int outSize);
void listPaymentsByPatientPatientView(const char *patientId,
    const char *dateBegin, const char *dateEnd, char *out, int outSize);
int collectUnpaidPaymentsByPatient(const char *patientId, Payment *list[], int max);

#ifdef __cplusplus
}
#endif

#endif
