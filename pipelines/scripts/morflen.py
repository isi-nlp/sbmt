class Comparator:
    def __init__(self,*args):
        self.maxlen = int(args[0])
    def __call__(self,s):
        return len(s.split()) < self.maxlen