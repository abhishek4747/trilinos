#! /usr/bin/env python

# @HEADER
# ************************************************************************
#
#                PyTrilinos: Python Interface to Trilinos
#                   Copyright (2005) Sandia Corporation
#
# Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
# license for use of this work by or on behalf of the U.S. Government.
#
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of the
# License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA
# Questions? Contact Michael A. Heroux (maherou@sandia.gov)
#
# ************************************************************************
# @HEADER

# Imports.  Users importing an installed version of PyTrilinos should use the
# "from PyTrilinos import ..." syntax.  Here, the setpath module adds the build
# directory, including "PyTrilinos", to the front of the search path.  We thus
# use "import ..." for Trilinos modules.  This prevents us from accidentally
# picking up a system-installed version and ensures that we are testing the
# build module.
from   numpy    import *
from   optparse import *
import sys
import unittest

parser = OptionParser()
parser.add_option("-b", "--use-boost", action="store_true", dest="boost",
                  default=False,
                  help="test the experimental boost-generated PyTrilinos package")
parser.add_option("-t", "--testharness", action="store_true",
                  dest="testharness", default=False,
                  help="test local build modules; prevent loading system-installed modules")
parser.add_option("-v", "--verbosity", type="int", dest="verbosity", default=2,
                  help="set the verbosity level [default 2]")
options,args = parser.parse_args()
if options.testharness:
    import setpath
    if options.boost: setpath.setpath("src-boost")
    else:             setpath.setpath()
    import Epetra
else:
    try:
        import setpath
        if options.boost: setpath.setpath("src-boost")
        else:             setpath.setpath()
        import Epetra
    except ImportError:
        from PyTrilinos import Epetra
        print >>sys.stderr, "Using system-installed Epetra"

##########################################################################

class EpetraBlockMapTestCase(unittest.TestCase):
    "TestCase class for BlockMap objects"

    def setUp(self):
        self.comm             = Epetra.PyComm()
        self.numProc          = self.comm.NumProc()
        self.myPID            = self.comm.MyPID()
        self.numMyElConst     = 4
        self.numMyEl          = self.numMyElConst + self.myPID
        self.numGlobalElConst = self.numMyElConst * self.numProc
        self.numGlobalEl      = self.comm.SumAll(self.numMyEl)
        self.elSizeConst      = 10
        self.elSizeList       = range(5,5+self.numMyEl)
        self.indexBase        = 0
        globalEls             = range(self.numGlobalEl)
        globalEls.reverse()  # Force this to be different from the linear distribution
        self.start            = 4*self.myPID + self.myPID*(self.myPID-1)/2
        self.myGlobalEls      = globalEls[self.start:self.start+self.numMyEl]

        self.map1             = Epetra.BlockMap(self.numGlobalElConst,
                                                self.elSizeConst,
                                                self.indexBase,
                                                self.comm)

        self.map2             = Epetra.BlockMap(self.numGlobalEl,
                                                self.numMyEl,
                                                self.elSizeConst,
                                                self.indexBase,
                                                self.comm)

        self.map3             = Epetra.BlockMap(self.numGlobalEl,
                                                self.myGlobalEls,
                                                self.elSizeConst,
                                                self.indexBase,
                                                self.comm)

        self.map4             = Epetra.BlockMap(self.numGlobalEl,
                                                self.myGlobalEls,
                                                self.elSizeList,
                                                self.indexBase,
                                                self.comm)

        self.comm.Barrier()

    def tearDown(self):
        self.comm.Barrier()

    def testConstructor01(self):
        "Test Epetra.BlockMap uniform constructor"
        self.assertEqual(self.map1.NumGlobalElements(), self.numGlobalElConst)
        self.assertEqual(self.map1.ElementSize(),       self.elSizeConst     )
        self.assertEqual(self.map1.IndexBase(),         self.indexBase       )

    def testConstructor02(self):
        "Test Epetra.BlockMap uniform constructor, negative global size"
        size = -2 * self.numProc
        self.assertRaises(Epetra.Error, Epetra.BlockMap, size, 1, 0, self.comm)

    def testConstructor03(self):
        "Test Epetra.BlockMap uniform constructor, zero element size"
        size = 2 * self.numProc
        self.assertRaises(Epetra.Error, Epetra.BlockMap, size, 0, 0, self.comm)

    def testConstructor04(self):
        "Test Epetra.BlockMap nonuniform constructor"
        self.assertEqual(self.map2.NumGlobalElements(), self.numGlobalEl)
        self.assertEqual(self.map2.NumMyElements(),     self.numMyEl    )
        self.assertEqual(self.map2.ElementSize(),       self.elSizeConst)
        self.assertEqual(self.map2.IndexBase(),         self.indexBase  )

    def testConstructor05(self):
        "Test Epetra.BlockMap nonuniform constructor, negative global size"
        size = -2 * self.numProc
        self.assertRaises(Epetra.Error, Epetra.BlockMap, size, 2, 1, 0, self.comm)

    def testConstructor06(self):
        "Test Epetra.BlockMap nonuniform constructor, negative num my elements"
        self.assertRaises(Epetra.Error, Epetra.BlockMap, self.numGlobalEl, -2, 1, 0, self.comm)

    def testConstructor07(self):
        "Test Epetra.BlockMap nonuniform constructor, negative element size"
        self.assertRaises(Epetra.Error, Epetra.BlockMap, self.numGlobalEl,
                          self.numMyEl, -1, 0, self.comm)

    def testConstructor08(self):
        "Test Epetra.BlockMap nonuniform, arbitrary, same-size element constructor"
        self.assertEqual(self.map3.NumGlobalElements(), self.numGlobalEl)
        self.assertEqual(self.map3.NumMyElements(),     self.numMyEl    )
        self.assertEqual(self.map3.ElementSize(),       self.elSizeConst)
        self.assertEqual(self.map3.IndexBase(),         self.indexBase  )

    def testConstructor09(self):
        "Test Epetra.BlockMap nonuniform, arbitrary constructor, bad list"
        self.myGlobalEls[-1] = "pi"
        self.assertRaises((TypeError,ValueError), Epetra.BlockMap,
                          self.numGlobalEl, self.myGlobalEls, self.elSizeConst,
                          self.indexBase, self.comm)

    def testConstructor10(self):
        "Test Epetra.BlockMap nonuniform, arbitrary constructor, negative global size"
        size = -2 * self.numProc
        self.assertRaises(Epetra.Error, Epetra.BlockMap, size, self.myGlobalEls,
                          1, 0, self.comm)

    def testConstructor11(self):
        "Test Epetra.BlockMap nonuniform, arbitrary constructor, negative element size"
        size = -2 * self.numProc
        self.assertRaises(Epetra.Error, Epetra.BlockMap, self.numGlobalEl,
                          self.myGlobalEls, -2, 0, self.comm)

#     def testConstructor12(self):
#         "Test Epetra.BlockMap uniform linear constructor, min GID < index base"
#         self.myGlobalEls[0] = self.indexBase-1
#         print self.myGlobalEls
#         self.assertRaises(Epetra.Error, Epetra.BlockMap, self.numGlobalEl,
#                           self.myGlobalEls, self.elSizeConst, self.indexBase,
#                           self.comm)

    def testConstructor13(self):
        "Test Epetra.BlockMap nonuniform, arbitrary, variable-size element constructor"
        self.assertEqual(self.map4.NumGlobalElements(),   self.numGlobalEl    )
        self.assertEqual(self.map4.NumMyElements(),       self.numMyEl        )
        self.assertEqual(self.map4.ConstantElementSize(), False               )
        self.assertEqual(self.map4.NumMyPoints(),         sum(self.elSizeList))
        self.assertEqual(self.map4.IndexBase(),           self.indexBase      )

    def testConstructor14(self):
        "Test Epetra.BlockMap copy constructor"
        map1 = Epetra.BlockMap(self.map1)
        map2 = Epetra.BlockMap(self.map2)
        map3 = Epetra.BlockMap(self.map3)
        map4 = Epetra.BlockMap(self.map4)
        self.assertEqual(self.map1.SameAs(map1),True)
        self.assertEqual(self.map2.SameAs(map2),True)
        self.assertEqual(self.map3.SameAs(map3),True)
        self.assertEqual(self.map4.SameAs(map4),True)

    def testConstructor15(self):
        "Test Epetra.BlockMap uniform linear constructor, negative local size"
        size = self.numProc
        self.assertRaises(Epetra.Error, Epetra.BlockMap, size, -1, 1, 0, self.comm)

    def testRemoteIDList1(self):
        "Test Epetra.BlockMap RemoteIDList method for constant element size"
        gidList  = range(self.numGlobalEl)
        gidList.reverse()  # Match the setUp pattern
        pidList  = zeros( self.numGlobalEl)
        lidList  = arange(self.numGlobalEl)
        sizeList = ones(  self.numGlobalEl) * self.elSizeConst
        for p in range(1,self.numProc):
            start  = 4*p + p*(p-1)/2
            length = 4 + p
            pidList[start:] += 1
            lidList[ start:start+length] = range(length)
        result = self.map3.RemoteIDList(gidList)
        self.failUnless((result[0] == pidList ).all())
        self.failUnless((result[1] == lidList ).all())
        self.failUnless((result[2] == sizeList).all())

    def testRemoteIDList2(self):
        "Test Epetra.BlockMap RemoteIDList method for variable element size"
        gidList  = range(self.numGlobalEl)
        gidList.reverse()  # Match the setUp pattern
        pidList  = zeros( self.numGlobalEl)
        lidList  = arange(self.numGlobalEl)
        sizeList = arange(self.numGlobalEl) + 5
        for p in range(1,self.numProc):
            start  = 4*p + p*(p-1)/2
            length = 4 + p
            pidList[start:] += 1
            lidList[ start:start+length] = range(length)
            sizeList[start:start+length] = range(5,5+length)
        result = self.map4.RemoteIDList(gidList)
        self.failUnless((result[0] == pidList ).all())
        self.failUnless((result[1] == lidList ).all())
        self.failUnless((result[2] == sizeList).all())

    def testLID(self):
        "Test Epetra.BlockMap LID method"
        for gid in range(self.map3.NumGlobalElements()):
            if gid in self.myGlobalEls:
                lid = self.myGlobalEls.index(gid)
            else:
                lid = -1
            self.assertEqual(self.map3.LID(gid),lid)

    def testGID(self):
        "Test Epetra.BlockMap GID method"
        for lid in range(self.map3.NumMyElements()):
            self.assertEqual(self.map3.GID(lid), self.myGlobalEls[lid])
        for lid in range(self.map4.NumMyElements()):
            self.assertEqual(self.map4.GID(lid), self.myGlobalEls[lid])

    def testFindLocalElementID(self):
        "Test Epetra.BlockMap FindLocalElementID method"
        pointID = 0
        for lid in range(self.map4.NumMyElements()):
            for offset in range(self.map4.ElementSize(lid)):
                result = self.map4.FindLocalElementID(pointID)
                self.assertEqual(result[0], lid   )
                self.assertEqual(result[1], offset)
                pointID += 1

    def testMyGID(self):
        "Test Epetra.BlockMap MyGID method"
        for gid in range(self.map3.NumGlobalElements()):
            if gid in self.myGlobalEls:
                lid = self.myGlobalEls.index(gid)
            else:
                lid = -1
            self.assertEqual(self.map3.MyGID(gid), (lid != -1))

    def testMyLID(self):
        "Test Epetra.BlockMap MyLID method"
        for gid in range(self.map3.NumGlobalElements()):
            if gid in self.myGlobalEls:
                lid = self.myGlobalEls.index(gid)
            else:
                lid = -1
            self.assertEqual(self.map3.MyLID(lid), (lid != -1))

    def testMinAllGID(self):
        "Test Epetra.BlockMap MinAllGID method"
        self.assertEqual(self.map1.MinAllGID(), self.indexBase)
        self.assertEqual(self.map2.MinAllGID(), self.indexBase)
        self.assertEqual(self.map3.MinAllGID(), self.indexBase)
        self.assertEqual(self.map4.MinAllGID(), self.indexBase)

    def testMaxAllGID(self):
        "Test Epetra.BlockMap MaxAllGID method"
        self.assertEqual(self.map1.MaxAllGID(), self.numGlobalElConst-1)
        self.assertEqual(self.map2.MaxAllGID(), self.numGlobalEl     -1)
        self.assertEqual(self.map3.MaxAllGID(), self.numGlobalEl     -1)
        self.assertEqual(self.map4.MaxAllGID(), self.numGlobalEl     -1)

    def testMinMyGID(self):
        "Test Epetra.BlockMap MinMyGID method"
        self.assertEqual(self.map1.MinMyGID(), self.myPID*self.numMyElConst)
        self.assertEqual(self.map2.MinMyGID(), self.start                  )
        self.assertEqual(self.map3.MinMyGID(), min(self.myGlobalEls)       )
        self.assertEqual(self.map4.MinMyGID(), min(self.myGlobalEls)       )

    def testMaxMyGID(self):
        "Test Epetra.BlockMap MaxMyGID method"
        self.assertEqual(self.map1.MaxMyGID(), (self.myPID+1)*self.numMyElConst-1)
        self.assertEqual(self.map2.MaxMyGID(), self.start+self.numMyEl-1         )
        self.assertEqual(self.map3.MaxMyGID(), max(self.myGlobalEls)             )
        self.assertEqual(self.map4.MaxMyGID(), max(self.myGlobalEls)             )

    def testMinLID(self):
        "Test Epetra.BlockMap MinLID method"
        self.assertEqual(self.map1.MinLID(), self.indexBase)
        self.assertEqual(self.map2.MinLID(), self.indexBase)
        self.assertEqual(self.map3.MinLID(), self.indexBase)
        self.assertEqual(self.map4.MinLID(), self.indexBase)

    def testMaxLID(self):
        "Test Epetra.BlockMap MaxLID method"
        self.assertEqual(self.map1.MaxLID(), self.numMyElConst-1)
        self.assertEqual(self.map2.MaxLID(), self.numMyEl-1     )
        self.assertEqual(self.map3.MaxLID(), self.numMyEl-1     )
        self.assertEqual(self.map4.MaxLID(), self.numMyEl-1     )

    def testNumGlobalElements(self):
        "Test Epetra.BlockMap NumGlobalElements method"
        self.assertEqual(self.map1.NumGlobalElements(), self.numGlobalElConst)
        self.assertEqual(self.map2.NumGlobalElements(), self.numGlobalEl     )
        self.assertEqual(self.map3.NumGlobalElements(), self.numGlobalEl     )
        self.assertEqual(self.map4.NumGlobalElements(), self.numGlobalEl     )

    def testNumMyElements(self):
        "Test Epetra.BlockMap NumMyElements method"
        self.assertEqual(self.map1.NumMyElements(), self.numMyElConst)
        self.assertEqual(self.map2.NumMyElements(), self.numMyEl     )
        self.assertEqual(self.map3.NumMyElements(), self.numMyEl     )
        self.assertEqual(self.map4.NumMyElements(), self.numMyEl     )

    def testMyGlobalElements(self):
        "Test Epetra.BlockMap MyGlobalElements method"
        result = self.map4.MyGlobalElements()
        self.failUnless((result == self.myGlobalEls).all())

    def testElementSize1(self):
        "Test Epetra.BlockMap ElementSize method for constant element sizes"
        self.assertEqual(self.map1.ElementSize(), self.elSizeConst)
        self.assertEqual(self.map2.ElementSize(), self.elSizeConst)
        self.assertEqual(self.map3.ElementSize(), self.elSizeConst)
        self.assertEqual(self.map4.ElementSize(), 0               )

    def testElementSize2(self):
        "Test Epetra.BlockMap ElementSize method for specified LID"
        for lid in range(self.map1.NumMyElements()):
            self.assertEqual(self.map1.ElementSize(lid), self.elSizeConst)
        for lid in range(self.map2.NumMyElements()):
            self.assertEqual(self.map2.ElementSize(lid), self.elSizeConst)
        for lid in range(self.map3.NumMyElements()):
            self.assertEqual(self.map3.ElementSize(lid), self.elSizeConst)
        for lid in range(self.map4.NumMyElements()):
            self.assertEqual(self.map4.ElementSize(lid), self.elSizeList[lid])

    def testFirstPointInElement(self):
        "Test Epetra.BlockMap FirstPointInElement method"
        for lid in range(self.map1.NumMyElements()):
            self.assertEqual(self.map1.FirstPointInElement(lid),
                             lid*self.elSizeConst)
        for lid in range(self.map2.NumMyElements()):
            self.assertEqual(self.map2.FirstPointInElement(lid),
                             lid*self.elSizeConst)
        for lid in range(self.map3.NumMyElements()):
            self.assertEqual(self.map3.FirstPointInElement(lid),
                             lid*self.elSizeConst)
        for lid in range(self.map4.NumMyElements()):
            self.assertEqual(self.map4.FirstPointInElement(lid),
                             sum(self.elSizeList[:lid]))

    def testIndexBase(self):
        "Test Epetra.BlockMap IndexBase method"
        self.assertEqual(self.map1.IndexBase(), self.indexBase)
        self.assertEqual(self.map2.IndexBase(), self.indexBase)
        self.assertEqual(self.map3.IndexBase(), self.indexBase)
        self.assertEqual(self.map4.IndexBase(), self.indexBase)

    def testNumGlobalPoints(self):
        "Test Epetra.BlockMap NumGlobalPoints method"
        self.assertEqual(self.map1.NumGlobalPoints(), self.numGlobalElConst *
                         self.elSizeConst)
        self.assertEqual(self.map2.NumGlobalPoints(), self.numGlobalEl *
                         self.elSizeConst)
        self.assertEqual(self.map3.NumGlobalPoints(), self.numGlobalEl *
                         self.elSizeConst)
        self.assertEqual(self.map4.NumGlobalPoints(),
                         self.comm.SumAll(sum(self.elSizeList))) 

    def testNumMyPoints(self):
        "Test Epetra.BlockMap NumMyPoints method"
        self.assertEqual(self.map1.NumMyPoints(), self.numMyElConst *
                         self.elSizeConst)
        self.assertEqual(self.map2.NumMyPoints(), self.numMyEl *
                         self.elSizeConst)
        self.assertEqual(self.map3.NumMyPoints(), self.numMyEl *
                         self.elSizeConst)
        self.assertEqual(self.map4.NumMyPoints(), sum(self.elSizeList))

    def testMinMyElementSize(self):
        "Test Epetra.BlockMap MinMyElementSize method"
        self.assertEqual(self.map1.MinMyElementSize(), self.elSizeConst    )
        self.assertEqual(self.map2.MinMyElementSize(), self.elSizeConst    )
        self.assertEqual(self.map3.MinMyElementSize(), self.elSizeConst    )
        self.assertEqual(self.map4.MinMyElementSize(), min(self.elSizeList))

    def testMaxMyElementSize(self):
        "Test Epetra.BlockMap MaxMyElementSize method"
        self.assertEqual(self.map1.MaxMyElementSize(), self.elSizeConst    )
        self.assertEqual(self.map2.MaxMyElementSize(), self.elSizeConst    )
        self.assertEqual(self.map3.MaxMyElementSize(), self.elSizeConst    )
        self.assertEqual(self.map4.MaxMyElementSize(), max(self.elSizeList))

    def testMinElementSize(self):
        "Test Epetra.BlockMap MinElementSize method"
        self.assertEqual(self.map1.MinElementSize(), self.elSizeConst    )
        self.assertEqual(self.map2.MinElementSize(), self.elSizeConst    )
        self.assertEqual(self.map3.MinElementSize(), self.elSizeConst    )
        self.assertEqual(self.map4.MinElementSize(),
                         self.comm.MinAll(min(self.elSizeList)))

    def testMaxElementSize(self):
        "Test Epetra.BlockMap MaxElementSize method"
        self.assertEqual(self.map1.MaxElementSize(), self.elSizeConst    )
        self.assertEqual(self.map2.MaxElementSize(), self.elSizeConst    )
        self.assertEqual(self.map3.MaxElementSize(), self.elSizeConst    )
        self.assertEqual(self.map4.MaxElementSize(),
                         self.comm.MaxAll(max(self.elSizeList)))

    def testConstantElementSize(self):
        "Test Epetra.BlockMap ConstantElementSize method"
        self.assertEqual(self.map1.ConstantElementSize(), True )
        self.assertEqual(self.map2.ConstantElementSize(), True )
        self.assertEqual(self.map3.ConstantElementSize(), True )
        self.assertEqual(self.map4.ConstantElementSize(), False)

    def testSameAs(self):
        "Test Epetra.BlockMap SameAs method"
        self.assertEqual(self.map1.SameAs(self.map1), True           )
        self.assertEqual(self.map1.SameAs(self.map2), self.numProc==1)
        self.assertEqual(self.map1.SameAs(self.map3), False          )
        self.assertEqual(self.map1.SameAs(self.map4), False          )
        self.assertEqual(self.map2.SameAs(self.map2), True           )
        self.assertEqual(self.map2.SameAs(self.map3), False          )
        self.assertEqual(self.map2.SameAs(self.map4), False          )
        self.assertEqual(self.map3.SameAs(self.map3), True           )
        self.assertEqual(self.map3.SameAs(self.map4), False          )
        self.assertEqual(self.map4.SameAs(self.map4), True           )

    def testPointSameAs(self):
        "Test Epetra.BlockMap PointSameAs method"
        self.assertEqual(self.map1.PointSameAs(self.map1), True           )
        self.assertEqual(self.map1.PointSameAs(self.map2), self.numProc==1)
        self.assertEqual(self.map1.PointSameAs(self.map3), self.numProc==1)
        self.assertEqual(self.map1.PointSameAs(self.map4), False          )
        self.assertEqual(self.map2.PointSameAs(self.map2), True           )
        self.assertEqual(self.map2.PointSameAs(self.map3), True           ) # Same structure!
        self.assertEqual(self.map2.PointSameAs(self.map4), False          )
        self.assertEqual(self.map3.PointSameAs(self.map3), True           )
        self.assertEqual(self.map3.PointSameAs(self.map4), False          )
        self.assertEqual(self.map4.PointSameAs(self.map4), True           )

    def testLinearMap(self):
        "Test Epetra.BlockMap LinearMap method"
        self.assertEqual(self.map1.LinearMap(), True )
        self.assertEqual(self.map2.LinearMap(), True )
        self.assertEqual(self.map3.LinearMap(), False)
        self.assertEqual(self.map4.LinearMap(), False)

    def testDistributedGlobal(self):
        "Test Epetra.BlockMap DistributedGlobal method"
        distributedGlobal = (self.comm.Label() == "Epetra::MpiComm" and
                             self.numProc > 1)
        self.assertEqual(self.map1.DistributedGlobal(), distributedGlobal)
        self.assertEqual(self.map2.DistributedGlobal(), distributedGlobal)
        self.assertEqual(self.map3.DistributedGlobal(), distributedGlobal)
        self.assertEqual(self.map4.DistributedGlobal(), distributedGlobal)

    def testFirstPointInElementList(self):
        "Test Epetra.BlockMap FirstPointInElementList method"
        firstPoints1 = [lid*self.elSizeConst for lid in range(self.numMyElConst)]
        firstPoints2 = [lid*self.elSizeConst for lid in range(self.numMyEl     )]
        firstPoints3 = firstPoints2
        firstPoints4 = [sum(self.elSizeList[:lid]) for lid in range(self.numMyEl)]
        result1      = self.map1.FirstPointInElementList()
        result2      = self.map2.FirstPointInElementList()
        result3      = self.map3.FirstPointInElementList()
        result4      = self.map4.FirstPointInElementList()
        self.failUnless((result1 == firstPoints1).all())
        self.failUnless((result2 == firstPoints2).all())
        self.failUnless((result3 == firstPoints3).all())
        self.failUnless((result4 == firstPoints4).all())

    def testElementSizeList(self):
        "Test Epetra.BlockMap ElementSizeList method"
        size1 = [self.elSizeConst for lid in range(self.numMyElConst)]
        size2 = [self.elSizeConst for lid in range(self.numMyEl     )]
        size3 = size2
        size4 = self.elSizeList
        result1 = self.map1.ElementSizeList()
        result2 = self.map2.ElementSizeList()
        result3 = self.map3.ElementSizeList()
        result4 = self.map4.ElementSizeList()
        self.failUnless((result1 == size1).all())
        self.failUnless((result2 == size2).all())
        self.failUnless((result3 == size3).all())
        self.failUnless((result4 == size4).all())

    def testPointToElementList(self):
        "Test Epetra.BlockMap PointToElementList method"
        elementList = []
        for lid in range(self.map4.NumMyElements()):
            for offset in range(self.map4.ElementSize(lid)):
                elementList.append(lid)
        result = self.map4.PointToElementList()
        self.failUnless((result == elementList).all())

    def testStr(self):
        "Test Epetra.BlockMap __str__ method"
        lines   = 7 + self.numMyElConst
        if self.myPID == 0: lines += 7
        s = str(self.map1)
        s = s.splitlines()
        self.assertEquals(len(s), lines)

    def testPrint(self):
        "Test Epetra.BlockMap Print method"
        myPID = self.myPID
        filename = "testBlockMap%d.dat" % myPID
        f = open(filename, "w")
        self.map2.Print(f)
        f.close()
        s = open(filename, "r").readlines()
        lines = 7 + self.numMyEl
        if myPID == 0: lines += 7
        self.assertEquals(len(s), lines)

    def testComm(self):
        "Test Epetra.BlockMap Comm method"
        comms = [self.map1.Comm(),
                 self.map2.Comm(),
                 self.map3.Comm(),
                 self.map4.Comm()]
        for comm in comms:
            self.assertEqual(comm.NumProc(),self.comm.NumProc())
            self.assertEqual(comm.MyPID()  ,self.comm.MyPID()  )

##########################################################################

if __name__ == "__main__":

    # Create the test suite object
    suite = unittest.TestSuite()

    # Add the test cases to the test suite
    suite.addTest(unittest.makeSuite(EpetraBlockMapTestCase))

    # Create a communicator
    comm    = Epetra.PyComm()
    iAmRoot = comm.MyPID() == 0
    comm.SetTracebackMode(0)    # Turn off errors printed to stderr

    # Run the test suite
    if iAmRoot: print >>sys.stderr, \
       "\n***********************\nTesting Epetra.BlockMap\n***********************\n"
    verbosity = options.verbosity * int(iAmRoot)
    result = unittest.TextTestRunner(verbosity=verbosity).run(suite)

    # Exit with a code that indicates the total number of errors and failures
    errsPlusFails = comm.SumAll(len(result.errors) + len(result.failures))
    if errsPlusFails == 0 and iAmRoot: print "End Result: TEST PASSED"
    sys.exit(errsPlusFails)
