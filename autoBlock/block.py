#!/usr/bin/env python2

import sys, math

xval = ["$fminX", "$bminX", "$bmaxX", "$fmaxX"]
yval = ["$fminY", "$bminY", "$bmaxY", "$fmaxY"]
zval = ["$fminZ", "$bminZ", "$bmaxZ", "$fmaxZ"]

for z in zval:
    for y in yval:
        for x in xval:
            print "(%s %s %s)" % (x, y, z)


fminX = -3.0
fminY = -3.0
fminZ = -3.0

fmaxX =  3.0
fmaxY =  3.0
fmaxZ =  3.0

bminX = -1.0
bminY = -1.0
bminZ = -1.0

bmaxX =  1.0
bmaxY =  1.0
bmaxZ =  1.0



e = 0.2
g = 1.3

fmin = [-2.0, -2.0, -2.0]
fmax = [ 2.0,  2.0,  2.0]
bmin = [-1.0, -1.0, -1.0]
bmax = [ 1.0,  1.0,  1.0]

c = [
    fmin,
    bmin,
    bmax,
    fmax
]
eminX = int(math.log((bminX-fminX)/e*(g-1)+1, g))
eminY = int(math.log((bminY-fminY)/e*(g-1)+1, g))
eminZ = int(math.log((bminZ-fminZ)/e*(g-1)+1, g))

emidX = int((bmaxX - bminX)/e)
emidY = int((bmaxY - bminY)/e)
emidZ = int((bmaxZ - bminZ)/e)

emaxX = int(math.log((fmaxX-bmaxX)/e*(g-1)+1, g))
emaxY = int(math.log((fmaxY-bmaxY)/e*(g-1)+1, g))
emaxZ = int(math.log((fmaxZ-bmaxZ)/e*(g-1)+1, g))

e = [
    [eminX, emidX, emaxX],
    [eminY, emidY, emaxY],
    [eminZ, emidZ, emaxZ],
]

#eX = [eminX, emidX, emaxX]
#eY = [eminY, emidY, emaxY]
#eZ = [eminZ, emidZ, emaxZ]

eX = ["$eminX", "$emidX", "$emaxX"]
eY = ["$eminY", "$emidY", "$emaxY"]
eZ = ["$eminZ", "$emidZ", "$emaxZ"]

gminX = 1/math.pow(g, eminX-1)
gminY = 1/math.pow(g, eminY-1)
gminZ = 1/math.pow(g, eminZ-1)

gmidX = 1
gmidY = 1
gmidZ = 1

gmaxX = math.pow(g, eminX-1)
gmaxY = math.pow(g, eminY-1)
gmaxZ = math.pow(g, eminZ-1)

#gX = [gminX, gmidX, gmaxX]
#gY = [gminY, gmidY, gmaxY]
#gZ = [gminZ, gmidZ, gmaxZ]

gX = ["$gminX", "$gmidX", "$gmaxX"]
gY = ["$gminY", "$gmidY", "$gmaxY"]
gZ = ["$gminZ", "$gmidZ", "$gmaxZ"]

for k in [0,1,2]:
    for j in [0,1,2]:
        for i in [0,1,2]:
            sys.stdout.write("    hex ( ")
            for n in range(0,8):
                vertex = i+(n+1>>1&1) + 4*(j+(n>>1&1)) + 16*(k+(n>>2))
                sys.stdout.write("%3d " % vertex)
            sys.stdout.write(") ")
            #sys.stdout.write("( %3d %3d %3d )" % (eX[i], eY[j], eZ[k]))
            sys.stdout.write("( %s %s %s )" % (eX[i], eY[j], eZ[k]))
            sys.stdout.write(" simpleGrading ")
            #sys.stdout.write("( %.2f %.2f %.2f )" % (gX[i], gY[j], gZ[k]))
            sys.stdout.write("( %s %s %s )" % (gX[i], gY[j], gZ[k]))
            sys.stdout.write("\n")

print "minX"
print '''{
    type patch;
    faces
    ('''
for j in range(0,3):
    for i in range(0,3):
        sys.stdout.write("        ( ")
        for n in range(0,4):
            vertex = 16*i + 16*(n+1>>1&1) +4*(n>>1) +4*j
            sys.stdout.write("%3d " % vertex)
        sys.stdout.write(")\n")
print '''    );
}'''

print "maxX"
print '''{
    type patch;
    faces
    ('''
for j in range(0,3):
    for i in range(0,3):
        sys.stdout.write("        ( ")
        for n in range(3,-1,-1):
            vertex = 16*i + 16*(n+1>>1&1) +4*(n>>1) +4*j + 3
            sys.stdout.write("%3d " % vertex)
        sys.stdout.write(")\n")
print '''    );
}'''

print "minY"
print '''{
    type patch;
    faces
    ('''
for j in range(0,3):
    for i in range(0,3):
        sys.stdout.write("        ( ")
        for n in range(0,4):
            vertex = i+(n+1>>1&1) +16*(n>>1) +16*j
            sys.stdout.write("%3d " % vertex)
        sys.stdout.write(")\n")
print '''    );
}'''

print "maxY"
print '''{
    type patch;
    faces
    ('''
for j in range(0,3):
    for i in range(0,3):
        sys.stdout.write("        ( ")
        for n in range(3,-1,-1):
            vertex = i+(n+1>>1&1) +16*(n>>1) +16*j + 12
            sys.stdout.write("%3d " % vertex)
        sys.stdout.write(")\n")
print '''    );
}'''

print "minZ"
print '''{
    type patch;
    faces
    ('''
for j in range(0,3):
    for i in range(0,3):
        sys.stdout.write("        ( ")
        for n in range(0,4):
            vertex = 4*i + 4*(n+1>>1&1) +(n>>1) +j
            sys.stdout.write("%3d " % vertex)
        sys.stdout.write(")\n")
print '''    );
}'''

print "maxZ"
print '''{
    type patch;
    faces
    ('''
for j in range(0,3):
    for i in range(0,3):
        sys.stdout.write("        ( ")
        for n in range(3,-1,-1):
            vertex = 4*i + 4*(n+1>>1&1) +(n>>1) +j +48
            sys.stdout.write("%3d " % vertex)
        sys.stdout.write(")\n")
print '''    );
}'''

