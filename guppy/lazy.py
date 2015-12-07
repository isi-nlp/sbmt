import heap
import itertools
import unittest
import exceptions

class IllegalAccess(exceptions.Exception):
    pass
    

def peek(obj):
    return obj.__peek__()

class PeekableIterator:
    def __init__(self, iterable):
        self._more = True
        self._iter = iterable
        try:
            self._current = self._iter.next()
        except StopIteration:
            self._more = False
            
    def iter(self):
        return self
        
    def next(self):
        if not self._more:
            raise StopIteration
        ret = self._current
        try:
            self._current = self._iter.next()
        except StopIteration:
            self._more = False
        return ret
    
    def peek(self):
        if not self._more:
            raise IllegalAccess
        return self._current
        
    def __peek__(self):
        return self.peek()
    def __next__(self):
        return self.next()
    def __iter__(self):
        return self.iter()

#decorator for generators
class peekable:
    def __init__(self,gen):
        self._gen = gen
    def __call__(self,*args,**keywords):
        return PeekableIterator(self._gen(*args,**keywords))
            
        

#--------------------------------------------------------------------------------

class LazyList(object):
    """A Sequence whose values are computed lazily by an iterator.
    """
    def __init__(self, iterable):
        self._exhausted = False
        self._iterator = iter(iterable)
        self._data = []

    def __len__(self):
        """Get the length of a LazyList's computed data."""
        return len(self._data)

    def __getitem__(self, i):
        """Get an item from a LazyList.
        i should be a positive integer or a slice object."""
        if isinstance(i, int):
            #index has not yet been yielded by iterator (or iterator exhausted
            #before reaching that index)
            if i >= len(self):
                self.exhaust(i)
            elif i < 0:
                raise ValueError('cannot index LazyList with negative number')
            return self._data[i]

        #LazyList slices are iterators over a portion of the list.
        elif isinstance(i, slice):
            start, stop, step = i.start, i.stop, i.step
            if any(x is not None and x < 0 for x in (start, stop, step)):
                raise ValueError('cannot index or step through a LazyList with'
                                 'a negative number')
            #set start and step to their integer defaults if they are None.
            if start is None:
                start = 0
            if step is None:
                 step = 1
            
            def LazyListIterator():
                count = start
                predicate = ((lambda: True) if stop is None
                             else (lambda: count < stop))
                while predicate():
                    try:
                        yield self[count]
                    #slices can go out of actual index range without raising an
                    #error
                    except IndexError:
                        break
                    count += step
            return LazyListIterator()

        raise TypeError('i must be an integer or slice')

    def __iter__(self):
        """return an iterator over each value in the sequence,
        whether it has been computed yet or not."""
        return self[:]

    def computed(self):
        """Return an iterator over the values in a LazyList that have
        already been computed."""
        return self[:len(self)]

    def exhaust(self, index = None):
        """Exhaust the iterator generating this LazyList's values.
        if index is None, this will exhaust the iterator completely.
        Otherwise, it will iterate over the iterator until either the list
        has a value for index or the iterator is exhausted.
        """
        if self._exhausted:
            return
        if index is None:
            ind_range = itertools.count(len(self))
        else:
            ind_range = xrange(len(self), index + 1)

        for ind in ind_range:
            try:
                self._data.append(self._iterator.next())
            except StopIteration: #iterator is fully exhausted
                self._exhausted = True
                break

#-------------------------------------------------------------------------------

class RecursiveLazyList(LazyList):
    def __init__(self, prod, *args, **kwds):
        super(RecursiveLazyList,self).__init__(prod(self,*args, **kwds))
        
#-------------------------------------------------------------------------------

class RecursiveLazyListFactory:
    def __init__(self, producer):
        self._gen = producer
    def __call__(self,*a,**kw):
        return RecursiveLazyList(self._gen,*a,**kw)

#-------------------------------------------------------------------------------
# decorator

def memoized(gen):
    return RecursiveLazyListFactory(gen)

#-------------------------------------------------------------------------------

def lazylist(gen):
    return LazyList(gen)

#-------------------------------------------------------------------------------

def product_heap(*args,**kv):
    def multiply(*lst):
        def times(a,b): return a * b
        return reduce(times,lst)
        
    lists = args
    product = kv['map'] if 'map' in kv else multiply
    compare = kv['cmp'] if 'cmp' in kv else cmp
    
    def compare_first(x,y):
        return compare(x[0],y[0])
        
    def next_positions(pos):
        
        def next_position(pos,d):
            newpos = pos[:]
            newpos[d] += 1
            return newpos

        for d in xrange(len(pos)):
            yield next_position(pos,d)
            if pos[d] != 0:
                break
    
    prodheap = heap.Heap(compare_first)
    try:
        pos = [0 for x in xrange(len(lists))]
        prod = product(*[x[y] for x,y in itertools.izip(lists,pos)])
        prodheap.push((prod,pos))
    except IndexError:
        pass
    while len(prodheap) > 0:
        top = prodheap.pop()
        yield top[0]
        for p in next_positions(top[1]):
            try:
                prod = product(*[x[y] for x,y in itertools.izip(lists,p)])
                prodheap.push((prod,p))
            except IndexError:
                pass
                
#-------------------------------------------------------------------------------

def merge_lists(list_of_lists_,**kv):
    list_of_lists = PeekableIterator(iter(list_of_lists_))
    compare = kv['cmp'] if 'cmp' in kv else cmp
    def compare_peekable(x,y):
        a = getattr(x,"__peek__",None)
        if callable(a):
            return compare_peekable(a(),y)
        a = getattr(y,"__peek__",None)
        if callable(a):
            return compare_peekable(x,a())
        #print "comparing", x, "vs", y
        return compare(x,y)
        
    pq = heap.Heap(compare_peekable)
    
    for alist in list_of_lists:
        toplist = alist
        while True:
            n = toplist.next()
            yield n
            try:
                peek(toplist)
                pq.push(toplist)
            except IllegalAccess:
                pass
            if len(pq) > 0:
                toplist = pq.pop()
            else:
                break
            try:
                peek(list_of_lists)
                if compare_peekable(list_of_lists,toplist) < 0:
                    pq.push(toplist)
                    break
            except IllegalAccess:
                pass

                
            
                
        
def finite_merge_heap(*args,**kv):
    compare = kv['cmp'] if 'cmp' in kv else cmp
    def compare_first(x,y):
        return compare(x[0],y[0])
    unionheap = heap.Heap(compare_first)
    for a in args:
        try:
            i = iter(a)
            ix = i.next()
            unionheap.push((ix,i))
        except StopIteration:
            pass
    while len(unionheap) > 0:
        top = unionheap.pop()
        yield top[0]
        i = top[1]
        try:
            ix = i.next()
            unionheap.push((ix,i))
        except StopIteration:
            pass

class ProductHeapTests(unittest.TestCase):
    def test_arg_style(self):
        @memoized
        def primegen(lst):
            yield 2
            for candidate in itertools.count(3): #start at next number after 2
                #if candidate is not divisible by any smaller prime numbers,
                #it is a prime.
                if all(candidate % p for p in lst.computed()):
                    yield candidate
        primes = primegen()
        
        primes2 = primegen()
        x = 0
        #while primes2[x] < 10000:
        #    print primes2[x]
        #    x = x + 1
             
        sumprimes = sorted([x + y for x in primes[:40] for y in primes[:40]])[:40]
        
        def sum(a,b): return a + b
        prime_products = lazylist(product_heap(*(primes,primes),**{'map':sum}))
        self.assertEquals(list(prime_products[:40]),sumprimes)
        
        prime_products = lazylist(product_heap(primes,primes,map=sum))
        self.assertEquals(list(prime_products[:40]),sumprimes)

    def test_finite_products(self):
        three = [1,2,3]
        cube = sorted(x*y*z for x in three for y in three for z in three)
        self.assertEquals(list(product_heap(three,three,three)),cube)
        
    def test_empty_product(self):
        def seq(): 
            i = 0
            while True:
                yield i
                i += 1
        
        self.assertEquals([],list(product_heap([],seq,seq)))

class UnionHeapTest(unittest.TestCase):
    def test_finite(self):
        lst1 = [1,4,7]
        lst2 = [2,5,8]
        lst3 = [3,6,9]
        lst = sorted(itertools.chain(lst1,lst2,lst3))
        self.assertEquals(lst,list(finite_merge_heap(lst1,lst2,lst3)))
    
    def test_infinite(self):
        @peekable
        def integers(x):
            while True:
                yield x
                x += 1
        
        lst1 = integers(1)
        lst2 = integers(2)
        lst3 = integers(3)
        
        xlst1 = integers(1)
        xlst2 = integers(2)
        xlst3 = integers(3)

        lst = sorted(itertools.chain(lazylist(lst1)[:50],lazylist(lst2)[:50],lazylist(lst3)[:50]))[:50]
    
        self.assertEquals(lst,list(lazylist(merge_lists([xlst1,xlst2,xlst3]))[:50]))

if __name__ == "__main__":
    unittest.main()
