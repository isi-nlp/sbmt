// From J. Rennie (http://people.csail.mit.edu/jrennie/python/#gamma)

#include <Python.h>

#define RUN_ERROR(x) { PyErr_SetString(PyExc_RuntimeError,x); return NULL; }
#define ASSERT_ERROR(x) { PyErr_SetString(PyExc_AssertionError,x); return NULL; }

#define __GAMMALN__(x,ret) { \
  int i; \
  double y,tmp,ser=1.000000000190015; \
  y=(x); \
  tmp=(x)+5.5; \
  tmp-=((x)+0.5)*log(tmp); \
  for (i=0; i < coflen; ++i) \
    ser += cof[i]/++y; \
  ret = -tmp+log(2.5066282746310005*ser/(x)); \
}

#define __DIGAMMA__(x,ret) { \
  int i; \
  double y[coflen],tmp,ser=1.000000000190015,ser2=0.0; \
  tmp = (x)+5.5; \
  tmp = log(tmp)-(5.0/tmp+1.0/(x)); \
  for (i=0; i < coflen; ++i) y[i] = 1.0/((x)+i+1.0); \
  for (i=0; i < coflen; ++i) ser += y[i]*cof[i]; \
  for (i=0; i < coflen; ++i) ser2 += y[i]*y[i]*cof[i]; \
  ret = tmp-ser2/ser; \
}

#define __DIGAMMA_PRIME__(x,ret) { \
  int i; \
  double y[coflen],tmp,ser=1.000000000190015,ser2=0.0,ser3=0.0; \
  tmp = (x)+5.5; \
  tmp = 5.0/(tmp*tmp)+1.0/((x)*(x))+1.0/tmp; \
  for (i=0; i < coflen; ++i) y[i] = 1.0/((x)+i+1.0); \
  for (i=0; i < coflen; ++i) ser += y[i]*cof[i]; \
  for (i=0; i < coflen; ++i) ser2 += y[i]*y[i]*cof[i]; \
  for (i=0; i < coflen; ++i) ser3 += y[i]*y[i]*y[i]*cof[i]; \
  ser3 *= 2.0; \
  ret = tmp+ser3/ser-ser2*ser2/(ser*ser); \
}

double fabs(double x) {
  if (x > 0.0) return x;
  else return -x;
}

static const int coflen = 6;
static const double cof[6]={76.18009172947146,-86.50532032941677,
			    24.01409824083091,-1.231739572450155,
			    0.1208650973866179e-2,-0.5395239384953e-5};
static PyObject*
gammaln(PyObject *self, PyObject *args)
{
  double x,ret;
  if (!PyFloat_Check(args)) RUN_ERROR("Bad argument to gammaln()");
  x = PyFloat_AS_DOUBLE(args);
  if (x <= 0.0) ASSERT_ERROR("digamma(): x must be positive");
  __GAMMALN__(x,ret);
  return Py_BuildValue("d",ret);
}

static PyObject*
digamma(PyObject *self, PyObject *args)
{
  double x,ret;
  if (!PyFloat_Check(args)) RUN_ERROR("Bad argument to digamma()");
  x = PyFloat_AS_DOUBLE(args);
  if (x <= 0.0) ASSERT_ERROR("digamma(): x must be positive");
  __DIGAMMA__(x,ret);
  return Py_BuildValue("d",ret);
}

static PyObject*
digamma_prime(PyObject *self, PyObject *args)
{
  double x,ret;
  if (!PyFloat_Check(args)) RUN_ERROR("Bad argument to digammaPrime()");
  x = PyFloat_AS_DOUBLE(args);
  if (x <= 0.0) ASSERT_ERROR("digamma(): x must be positive");
  __DIGAMMA_PRIME__(x,ret);
  return Py_BuildValue("d",ret);
}

static PyObject*
digamma_inv(PyObject *self, PyObject *args)
{
  double y,x=0.5,xlast=0.0,ret1,ret2;
  if (!PyFloat_Check(args)) RUN_ERROR("Bad argument to gammaln()");
  y = PyFloat_AS_DOUBLE(args);
  while (fabs(x-xlast) > 1e-10) {
    xlast = x;
    __DIGAMMA__(x,ret1);
    __DIGAMMA_PRIME__(x,ret2);
    x-=(ret1-y)/ret2;
  }
  return Py_BuildValue("d",x);
}

static PyObject*
beta_coeff(PyObject *self, PyObject *args)
{
  double alpha,beta,ret1,ret2,ret3;
  if (!PyArg_ParseTuple(args,"dd",&alpha,&beta))
    RUN_ERROR("Bad arguments to betaCoeff()");
  if (alpha <= 0.0) ASSERT_ERROR("digamma(): alpha must be positive");
  if (beta <= 0.0) ASSERT_ERROR("digamma(): beta must be positive");
  __GAMMALN__(alpha+beta,ret1);
  __GAMMALN__(alpha,ret2);
  __GAMMALN__(beta,ret3);
  return Py_BuildValue("d",exp(ret1-ret2-ret3));
}

static char gammaln_docs[] =
"Natural logarithm of the Gamma function.\n"
"Accepts a single, floating point number, x, as its only arugment.\n"
"Returns the natural logarithm of the Gamma function applied to x.\n";

static char digamma_docs[] =
"Digamma function.\n"
"Accepts a single, floating point number, x, as its only arugment.\n"
"Returns the result of the Digamma function applied to x.\n";

static char digamma_prime_docs[] =
"Derivative of the Digamma function.\n"
"Accepts a single, floating point number, x, as its only argument.\n"
"Returns the value of the derivative of the Digamma function at x.\n";

static char digamma_inv_docs[] =
"Inverse of the Digamma function.\n"
"Accepts a single, floating point number, y, as its only arugment.\n"
"Returns the value, x, such that y=digamma(x).\n";

static char beta_coeff_docs[] = 
"Coefficient of the Beta distribution.\n"
"Accepts exactly two parameters, alpha and beta.\n"
"Returns gamma(alpha+beta)/gamma(alpha)/gamma(beta).\n";

static PyMethodDef gamma_funcs[] = {
  {"gammaln", gammaln, METH_O, gammaln_docs},
  {"digamma", digamma, METH_O, digamma_docs},
  {"digammaPrime", digamma_prime, METH_O, digamma_prime_docs},
  {"digammaInv", digamma_inv, METH_O, digamma_inv_docs},
  {"betaCoeff", beta_coeff, METH_VARARGS, beta_coeff_docs},
  {NULL}
};

void
initgamma(void)
{
  Py_InitModule3("gamma", gamma_funcs, "Library of Gamma-type functions");
}
