#ifndef TREELEXHEAD_H
#define TREELEXHEAD_H

/* the head lists for lexicalization */

int nADJP = 19;
char* ADJP[19] = { "NNS", "QP", "NN", "$", "ADVP", "JJ", "VBN", "VBG", "ADJP", "JJR", "NP", "NPB", "JJS", "DT", "FW", "RBR", "RBS", "SBAR", "RB" }; 

int nADVP = 14;
char* ADVP[14] = { "RB", "RBR", "RBS", "FW", "ADVP", "TO", "CD", "JJR", "JJ", "IN", "NP", "NPB", "JJS", "NN" };

int nCONJP = 3;
char* CONJP[3] = { "CC", "RB", "IN" };

int nFRAG = 0;
char* FRAG[] = {};

int nINTJ = 0;
char* INTJ[] = {};

int nLST = 2;
char* LST[2] = { "LS", ":" };

int nNAC = 18;
char* NAC[18] = { "NN", "NNS", "NNP", "NNPS", "NP", "NPB", "NAC", "EX", "$", "CD", "QP", "PRP", "VBG", "JJ", "JJS", "JJR", "ADJP", "FW" };

int nPP = 6;
char* PP[6] = { "IN", "TO", "VBG", "VBN", "RP", "FW" }; /* ?RB added for WSJ_0214.MRG/65 wall */

int nPRN = 0;
char* PRN[] = {};

int nPRT = 1;
char* PRT[1] = { "RP" };

int nQP = 12;
char* QP[12] = { "$", "IN", "NNS", "NN", "JJ", "RB", "DT", "CD", "NCD", "QP", "JJR", "JJS" };

int nRRC = 6;
char* RRC[6] = { "VP", "NP", "NPB", "ADVP", "ADJP", "PP" };

int nS = 9;
char* S[9] = { "TO", "IN", "VP", "S", "SBAR", "ADJP", "UCP", "NP", "NPB" };

int nSBAR = 11;
char* SBAR[11] = { "WHNP", "WHPP", "WHADVP", "WHADJP", "IN", "DT", "S", "SQ", "SINV", "SBAR", "FRAG" };

int nSBARQ = 5;
char* SBARQ[5] = { "SQ", "S", "SINV", "SBARQ", "FRAG" };

int nSINV = 11;
char* SINV[11] = { "VBZ", "VBD", "VBP", "VB", "MD", "VP", "S", "SINV", "ADJP", "NP", "NPB" };

int nSQ = 7;
char* SQ[7] = { "VBZ", "VBD", "VBP", "VB", "MD", "VP", "SQ" };

int nUCP = 0;
char* UCP[] = {};

int nVP = 14;
char* VP[14] = { "TO", "VBD", "VBN", "MD", "VBZ", "VB", "VBG", "VBP", "VP", "ADJP", "NN", "NNS", "NP", "NPB" };

int nWHADJP = 4;
char* WHADJP[4] = { "CC", "WRB", "JJ", "ADJP" };

int nWHADVP = 2;
char* WHADVP[2] = { "CC", "WRB" };

int nWHNP = 6;
char* WHNP[6] = { "WDT", "WP", "WP$", "WHADJP", "WHPP", "WHNP" };

int nWHPP = 3;
char* WHPP[3] = { "IN", "TO", "FW" };

int nNP1 = 7;
char* NP1[7] = { "NN", "NNP", "NNPS", "NNS", "NX", "POS", "JJR" };
int nNP2 = 2;
char* NP2[2] = { "NP", "NPB" };
int nNP3 = 3;
char* NP3[3] = { "$", "ADJP", "PRN" };
int nNP4 = 1;
char* NP4[1] = { "CD" };
int nNP5 = 4;
char* NP5[4] = { "JJ", "JJS", "RB", "QP" };

#endif
