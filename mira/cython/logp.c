#include <stdio.h>
#include <math.h>

double exp10(double x) {
  return pow(10.0, x);
}

/* From http://www.hadron.org/~hatch/rightway.php, attributed to Kahan */

#ifndef HAVE_EXPM1
double expm1(double x)
{
  double u = exp(x);
  if (u == 1.)
    return x;
  if (u-1. == -1.)
    return -1.;
  return (u-1.)*x/log(u);
}
#endif

#ifndef HAVE_LOG1P
double log1p(double x)
{
  double u = 1.+x;
  if (u == 1.)
    return x;
  else
    return log(u)*x/(u-1.);
}
#endif

double logp_add(double x, double y) {
  return x>y ? x+log1p(exp(y-x)) : y+log1p(exp(x-y));
}

double logp_subtract(double x, double y) {
  /* We want x-log(1-exp(x-y)), and use log1p or expm1 depending on 
     whether exp(x-y) or |x-y| is small */
  return x-y>1 ? x+log1p(-exp(y-x)) : x+log(-expm1(y-x));
}

double log10p_add(double x, double y) {
  return x>y ? x+log1p(exp10(y-x))/M_LN10 : y+log1p(exp10(x-y))/M_LN10;
}

double log10p_subtract(double x, double y) {
  return x-y>1 ? x+log1p(-exp10(y-x))/M_LN10 : x+log10(-expm1((y-x)*M_LN10));
}

double neglogp_add(double x, double y) {
  return y>x ? x-log1p(exp(x-y)) : y-log1p(exp(y-x));
}

double neglogp_add2(double x, double y) {
  return y>x ? log1p(-exp(-y) + -expm1(-x)) : log1p(-exp(-x) + -expm1(-y));
}

double neglogp_subtract(double x, double y) {
  /* We want x-log(1-exp(x-y)), and use log1p or expm1 depending on 
     whether exp(x-y) or |x-y| is small */
  return y-x>1 ? x-log1p(-exp(x-y)) : x-log(-expm1(x-y));
}

double neglog10p_add(double x, double y) {
  return  y>x ? x-log1p(exp10(x-y))/M_LN10 : y-log1p(exp10(y-x))/M_LN10;
}

// for small results?
double neglog10p_add2(double x, double y) {
  return y>x ? log1p(-exp10(-y) + -expm1(-x*M_LN10))/M_LN10 : log1p(-exp10(-x) + -expm1(-y*M_LN10))/M_LN10;
}

double neglog10p_subtract(double x, double y) {
  return y-x > 1 ? x-log1p(-exp10(x-y))/M_LN10 : x-log10(-expm1((x-y)*M_LN10));
}

#ifdef MAIN
int main (int argc, char *argv[]) {
  double x, y;
  int i;
  for (x=0.0; x<=50.0; x+=1.0) {
    y = neglog10p_subtract(0.0, x);
    printf("x=%g\t1-x=%g\t1-(1-x)=%g\tx+(1-x)=%g %g\n", x, y, neglog10p_subtract(0.0, y), neglog10p_add(x,y), neglog10p_add2(x,y)); 
  }
}
#endif
