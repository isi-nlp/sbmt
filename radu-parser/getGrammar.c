#include "grammar.h"

char RulesTrain[] = "../TRAINING/";
char RulesPrefix[] = "rules";
char RulesSufix[] = "joint=2";

int allb;

int main(int argc, char **argv)
{
  char headframefile[80], modfile[80], Grammar[80];

  if( argc !=2 )
    { fprintf(stderr, "Usage: %s PTB|MT|PTB-MT\n", argv[0]); exit(33); }

  sprintf(headframefile, "%s%s.%s.headframe.%s", RulesTrain, RulesPrefix, argv[1], RulesSufix);
  sprintf(modfile, "%s%s.%s.mod.%s", RulesTrain, RulesPrefix, argv[1], RulesSufix);
  sprintf(Grammar, "../GRAMMAR/%s", argv[1]);

  grammar(headframefile, modfile, Grammar);
}
