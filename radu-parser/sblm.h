#ifndef SBLM_H_inc
#define SBLM_H_inc

void initAPI(int aSBLM, int aIRENEHACK, int awVOC, int PPS);
void initSBLM(char *train, char *grammar, int THCNT);
void trainModel2(char *file1, char *file2);

void readGrammar(char *grammar, int setTHCNT);

void updatePT(int L, int w, int cnt);
void updatePrior(int M, int t, int w, int cnt);

void trainHeadFrameAndPriorFromFile(char *fname);
void trainModAndPriorFromFile(char *fname);

void trainSBLMpps(char *train, char *rtype, int cnt);
void trainHeadFrameModAndPriorwVoc(char *fname);

char* readeventPrior(char *p, char buf[], unsigned char joint[], int *jn, unsigned char cond[], int *cn, int *flag1, int *flag2, int *flag3);
char* readeventPT(char *p, char buf[], unsigned char joint[], int *jn, unsigned char cond[], int *cn, int *flag1, int *flag2, int *flag3);
char* readeventTop(char *p, char buf[], unsigned char joint[], int *jn, unsigned char cond[], int *cn, int *flag1, int *flag2, int *flag3);
char* readeventHead(char *p, char buf[], unsigned char joint[], int *jn, unsigned char cond[], int *cn, int *flag1, int *flag2, int *flag3);
char* readeventFrame(char *p, char buf[], unsigned char joint[], unsigned char joint2[], int *jn, unsigned char cond[], int *cn, int *flag1, int *flag2, int *side);
char* readeventMod(char *p, char buf[], unsigned char joint[], int *jn, unsigned char cond[], int *cn, int *flag1, int *flag2, int *flag3, int *side);
char* readeventCP(char *p, char buf[], unsigned char joint[], int *jn, unsigned char cond[], int *cn, int *flag1, int *flag2, int *flag3);

void updateHead(int P, int t, int w, int H, int cnt);
void updateFrame(int P, int H, int t, int w, int C, int side, int cnt);
void updateTop(int P, int H, int t, int w, int cnt);
void updateMod1(int P, int C, int V, int T, int H, int t, int w, int M, int mt, int cc, int punc, int side, int cnt);
void updateMod2(int P, int C, int V, int T, int H, int t, int w, int M, int mt, int mw, int cc, int punc, int side, int cnt);
void updateCP1(int P, int H, int t, int w, int M, int mt, int mw, int ct, int cnt, int type);
void updateCP2(int P, int H, int t, int w, int M, int mt, int mw, int ct, int cw, int cnt, int type);

double probSBLMprior(char *node);
double probSBLM(char *node, int stopFlag);
int readNode(char buf[], char *info, int *NI, int *tI, int *wI, int *cvI);

double probSBLM2(int arr[], int stopFlag);
int readNode2(int arr[], int *parr, int type, int *NI, int *tI, int *wI, int *cvI);
int unknownSBLM(int wI);

double computePriorProb(int M, int t, int w);
double computeHeadProb(int H, int P, int t, int w, int logFlag);
double computeFrameProb(int C, int P, int H, int t, int w, int side, int logFlag);
double computeModProb(int M, int mt, int mw, int P, int H, int t, int w, int C, int V, int T, int cc, int punc, int side, int logFlag);
double computeMod1Prob(int M, int mt, int P, int H, int t, int w, int C, int V, int T, int cc, int punc, int side, int logFlag);
double computeMod2Prob(int mw, int M, int mt, int P, int H, int t, int w, int C, int V, int T, int cc, int punc, int side, int logFlag);
double computeCPProb(int t, int w, int P, int H1, int t1, int w1, int H2, int t2, int w2, int type, int logFlag);
double computeCP1Prob(int t, int P, int H1, int t1, int w1, int H2, int t2, int w2, int type, int logFlag);
double computeCP2Prob(int w, int t, int P, int H1, int t1, int w1, int H2, int t2, int w2, int type, int logFlag);

void updateCnt(unsigned char joint[], int nj, unsigned char cond[], int nc, int cnt, int jointFlag, int condFlag, int side);
void updateCntAndOutcome(unsigned char joint[], int nj, unsigned char cond[], int nc, int cnt, int jointFlag, int condFlag, int outcomeFlag, int side);
void setCntAndOutcome(char type, unsigned char joint[], int nj, int jcnt, unsigned char cond[], int nc, int ccnt, int ucnt, int jointFlag, int condFlag, int outcomeFlag, int side);

void computeProbAndLambda(unsigned char joint[], int nj, unsigned char cond[], int nc, int jointFlag, int condFlag, int outcomeFlag, int side, int fterm, int ffactor, double *p, double *l, int *cjoint, int *ccond, int *cu);

double smoothProb(double lambda_1, double p_1, double lambda_2, double p_2, double lambda_3, double p_3, double lambda_4, double p_4, double lambda_5, double p_5);

int getSide(char *side);
int getTau(char *tau);

/* void printHashDensity(Hash *hash, char *name); */

int mapSubcat(char *label);
int removeSubcat(char *label, int subcat);
int removeSubcatI(int labelI, int subcat);
int addSubcat(char *label, int subcat);
int addSubcatI(int labelI, int subcat);
int encodeFrame(char *frame);

#endif
