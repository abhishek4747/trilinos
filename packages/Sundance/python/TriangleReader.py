#@HEADER@
# ************************************************************************
# 
#                              Sundance
#                 Copyright (2005) Sandia Corporation
# 
# Copyright (year first published) Sandia Corporation.  Under the terms 
# of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
# retains certain rights in this software.
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
# Questions? Contact Kevin Long (krlong@sandia.gov), 
# Sandia National Laboratories, Livermore, California, USA
# 
# ************************************************************************
#@HEADER@


############################################################################
#
# Read Triangle file and create Python mesh object
#
############################################################################

from Mesh import Mesh
from sets import Set

class TriangleReader :

    def __init__(self, filename, indexOffset) :
        self.filename_ = filename
        self.indexOffset_ = indexOffset

    def getMesh(self) :

        mesh = Mesh()

        self.readPoints(mesh)

        self.readElems(mesh)

        return mesh

    # Read the .node file
    def readPoints(self, mesh) :

        f = file(self.filename_ + '.node')

        # read header, which looks like [nNodes, dim, nAttr, nBdry] 
        while 1 :
            line = f.readline()
            if line[0]=='#': continue
            headerline = line
            header = line.split()
            nNodes = int(header[0])
            d = int(header[1])
            nAttr = int(header[2])
            nMark = int(header[3])
            break

        mesh.setDimension(d)

        # read remaining lines, adding the points
        # each line looks like [gid, x_1 ... x_d , attrs, bdry]
        while 1:
            line = f.readline()
            if not line : break
            if line[0]=='#': continue
            data = map(float, line.split()[1:d+1])
            mesh.addPoint(data)


    # Read the .ele file
    def readElems(self, mesh) :
        
        f = file(self.filename_ + '.ele')

        # read header 
        while 1 :
            line = f.readline()
            if line[0]=='#': continue
            header = line.split()
            nElems = int(header[0])
            d = int(header[1])-1
            break

         # read lines, building elements and the element-to-node map
        while 1:
             line = f.readline()
             if not line : break
             if line[0]=='#': continue
             toks = line.split()
             ele = int(toks[0])
             verts = Set()
             for i in range(d+1) :
                 node = int(toks[i+1])-self.indexOffset_
                 verts.add(node)
             mesh.addElem(verts)
        
    
    
