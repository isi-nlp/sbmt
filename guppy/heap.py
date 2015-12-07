
"""
Handle priority heaps.

Heaps are arrays for which a[k] <= a[2*k+1] and a[k] <= a[2*k+2] for all k,
counting elements from 0.  For the sake of comparison, unexisting elements
are considered to be infinite.  The interesting property of a heap is that
a[0] is always its smallest element.

The strange invariant above is meant to be an efficient memory representation
for a tournament.  The numbers below are `k', not a[k]:

                                   0

                  1                                 2

          3               4                5               6

      7       8       9       10      11      12      13      14

    15 16   17 18   19 20   21 22   23 24   25 26   27 28   29 30


In the tree above, each cell `k' is topping `2*k+1' and `2*k+2'.  In an
usual binary tournament we see in sports, each cell is the winner over
the two cells it tops, and we can trace the winner down the tree to see
all opponents s/he had.  However, in many computer applications of such
tournaments, we do not need to trace the history of a winner.  To be
more memory efficient, when a winner is promoted, we try to replace it by
something else at a lower level, and the rule becomes that a cell and the
two cells it tops contain three different items, but the top cell "wins"
over the two topped cells.

If this heap invariant is protected at all time, index 0 is clearly the
overall winner.  The simplest algorithmic way to remove it and find the
"next" winner is to move some looser (let's say cell 30 in the diagram
above) into the 0 position, and then percolate this new 0 down the tree,
exchanging values, until the invariant is re-established.  This is clearly
logarithmic on the total number of items in the tree.  By iterating over
all items, you get an O(n ln n) sort.

A nice feature of this sort is that you can efficiently insert new items
while the sort is going on, provided that the inserted items are not
"better" than the last 0'th element you extracted.  This is especially
useful in simulation contexts, where the tree holds all incoming events,
and the "win" condition means the smallest scheduled time.  When an event
schedule other events for execution, they are scheduled into the future,
so they can easily go into the heap.  So, a heap is a good structure for
implementing schedulers (this is what I used for my MIDI sequencer :-).

Various structures for implementing schedulers have been extensively
studied, and heaps are good for this, as they are reasonably speedy,
the speed is almost constant, and the worst case is not much different
than the average case.  However, there are other representations which
are more efficient overall, yet the worst cases might be terrible.

Heaps are also very useful in big disk sorts.  You most probably all know
that a big sort implies producing "runs" (which are pre-sorted sequences,
which size is usually related to the amount of CPU memory), followed by
a merging passes for these runs, which merging is often very cleverly
organised[1].  It is very important that the initial sort produces the
longest runs possible.  Tournaments are a good way to that.  If, using
all the memory available to hold a tournament, you replace and percolate
items that happen to fit the current run, you'll produce runs which are
twice the size of the memory for random input, and much better for input
fuzzily ordered.

Moreover, if you output the 0'th item on disk and get an input which may
not fit in the current tournament (because the value "wins" over the last
output value), it cannot fit in the heap, so the size of the heap decreases.
The freed memory could be cleverly reused immediately for progressively
building a second heap, which grows at exactly the same rate the first
heap is melting.  When the first heap completely vanishes, you switch
heaps and start a new run.  Clever and quite effective!

In a word, heaps are useful memory structures to know.  I use them in a
few applications, and I think it is good to keep a `heap' module around. :-)

--------------------
[1] The disk balancing algorithms which are current, nowadays, are more
annoying than clever, and this is a consequence of the seeking capabilities
of the disks.  On devices which cannot seek, like big tape drives, the
story was quite different, and one had to be very clever to ensure (far
in advance) that each tape movement will be the most effective possible
(that is, will best participate at "progressing" the merge).  Some tapes
were even able to read backwards, and this was also used to avoid the
rewinding time.  Believe me, real good tape sorts were quite spectacular
to watch!  From all times, sorting has always been a Great Art! :-)
"""

class Heap:

    def __init__(self, compare=cmp,key=None):
        """
        Set a new heap.  If COMPARE is given, use it instead of built-in 
        comparison.

        COMPARE, given two items, should return negative, zero or positive 
        depending on the fact the first item compares smaller, equal or greater 
        than the second item.
        """
        self.compare = compare
        self.array = []

    def __call__(self):
        """
        A heap instance, when called as a function, return all its items.
        """
        return self.array

    def __len__(self):
        """
        Return the number of items in the current heap instance.
        """
        return len(self.array)

    def __getitem__(self, index):
        """
        Return the INDEX-th item from the heap instance.  INDEX is usually zero.
        """
        return self.array[index]

    def push(self, item):
        """
        Add ITEM to the current heap instance.
        """
        array = self.array
        compare = self.compare
        array.append(item)
        high = len(array) - 1
        while high > 0:
            low = (high-1)/2
            if compare(array[low], array[high]) <= 0:
                break
            array[low], array[high] = array[high], array[low]
            high = low
        #print "push(%s) --> heap=%s" % (item,array)

    def pop(self):
        """
        Remove and return the smallest item from the current heap instance.
        """
        array = self.array
        item = array[0]
        if len(array) == 1:
            del array[0]
        else:
            compare = self.compare
            array[0] = array.pop()
            low, high = 0, 1
            while high < len(array):
                if ((high+1 < len(array)
                     and compare(array[high], array[high+1]) > 0)):
                    high = high+1
                if compare(array[low], array[high]) <= 0:
                    break
                array[low], array[high] = array[high], array[low]
                low, high = high, 2*high+1
        #print "%s = pop() --> heap=%s" % (item,array)
        return item

def test(n=2000):
    heap = Heap()
    for k in range(n-1, -1, -1):
        heap.push(k)
    for k in range(n):
        assert k+len(heap) == n
        assert k == heap.pop()
        
if __name__ == '__main__':
    test()