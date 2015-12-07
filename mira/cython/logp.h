#ifndef LOGP_H
#define LOGP_H

#ifdef __cplusplus
extern "C" {
#endif

double logp_add(double x, double y);
double logp_subtract(double x, double y);
double log10p_add(double x, double y);
double log10p_subtract(double x, double y);
double neglogp_add(double x, double y);
double neglogp_subtract(double x, double y);
double neglog10p_add(double x, double y);
double neglog10p_subtract(double x, double y);

#ifdef __cplusplus
}
#endif

#endif
