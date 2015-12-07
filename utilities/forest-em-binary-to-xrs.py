import forest
from sys import stdin, stdout
import re

if __name__ == "__main__":

    blank = re.compile('^\s*$')
    for line in stdin:
        if re.match(blank,line):
            print ""
        else:
            f = parser.parse(line)
            print forest.forest_explosion(f,lambda x : x == "0")
