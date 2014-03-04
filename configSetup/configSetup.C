/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2014 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Application
    surfaceBoundingBox

Description
    Prints the bounding box of triSurfaces specified in a configDict file
    and writes it to the dictionary.

\*---------------------------------------------------------------------------*/

#include "triSurface.H"
//#include "triSurfaceSearch.H"
#include "argList.H"
#include "IOdictionary.H"
#include "Time.H"
#include "regExp.H"
#include "polyMesh.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::noParallel();
//    argList::validArgs.append("surfaceFile");
    #include "setRootCase.H"
    #include "createTime.H"
    runTime.functionObjects().off();

    const word dictName("geometryDict");
    IOdictionary configDict
    (
        IOobject
        (
            dictName,
            runTime.system(),
            runTime,
            IOobject::MUST_READ_IF_MODIFIED,
            IOobject::AUTO_WRITE
        )
    );

    wordList geomTypes
    (
        IStringStream("(solids blanks baffles rotations refinements)")()
    );
    forAll( geomTypes, i )
    {
        if ( ! configDict.found( geomTypes[i] ) )
        {
            dictionary dict;
            configDict.add( geomTypes[i], dict );
        }
    }

    fileName triSurfacePath = runTime.constantPath()/"triSurface";
    fileNameList triSurfaces = readDir(triSurfacePath);

    List<point> boundPoints;

    wordList allSurfaces;

    forAll( triSurfaces, i )
    {
        fileName surfFileName = triSurfaces[i];
        word geom = surfFileName.lessExt();
        allSurfaces.append(geom);

        Info<< "Reading surface from " << surfFileName << " ..." << endl;
        triSurface surf(triSurfacePath/surfFileName);
        boundBox bound(surf.points());
        boundPoints.append(bound.min());
        boundPoints.append(bound.max());

        if ( regExp("ROT.*").match(geom) )
        {
            dictionary& typeDict = configDict.subDict("rotations");
            if ( ! typeDict.found(geom) )
            {
                dictionary dict;
                dict.add("elementLength", 0.001);
                dict.add("radius", 0.018);
                dict.add("rpm", 4500.0);
                vectorField axis(2, vector(0, 0, 0));
                axis[1] = vector(1, 0, 0);
                dict.add("axis", axis);
                typeDict.add(geom, dict);
            }
        }
        else if ( regExp("FINE.*").match(geom) )
        {
            dictionary& typeDict = configDict.subDict("refinements");
            if ( ! typeDict.found(geom) )
            {
                dictionary dict;
                dict.add("elementLength", 0.001);
                typeDict.add(geom, dict);
            }
        }
        else if ( regExp("__.*").match(geom) )
        {
            dictionary& typeDict = configDict.subDict("baffles");
            if ( ! typeDict.found(geom) )
            {
                dictionary dict;
                dict.add("elementLength", 0.001);
                dict.add("thickness", 0.001);
                dict.add("conductivity", 1.0);
                typeDict.add(geom, dict);
            }
        }
        else if ( regExp("_.*").match(geom) )
        {
            dictionary& typeDict = configDict.subDict("blanks");
            if ( ! typeDict.found(geom) )
            {
                dictionary dict;
                dict.add("elementLength", 0.001);
                typeDict.add(geom, dict);
            }
        }
        else
        {
            dictionary& typeDict = configDict.subDict("solids");
            if ( ! typeDict.found(geom) )
            {
                dictionary dict;
                dict.add("elementLength", 0.001);
                dict.add("conductivity", 210);
                dict.add("emissivity", 0.8);
                dict.add("power", 0);
                typeDict.add(geom, dict);
            }
        }
    }

    pointField boundingBoxPoints(2);
    boundBox boundingBox(boundPoints);
    boundingBoxPoints[0] = boundingBox.min();
    boundingBoxPoints[1] = boundingBox.max();

    Info<< nl << "Bounding Box: " << boundingBox << nl << nl;

    forAll( geomTypes, i )
    {
        dictionary& typeDict = configDict.subDict(geomTypes[i]);
        wordList geoms = typeDict.toc();
        forAll( geoms, i )
        {
            word geom = geoms[i];
            if ( findIndex(allSurfaces, geom) == -1 )
            {
                typeDict.remove(geom);
                Info<< "Remove " << geom << " from dictionary" << nl;
            }
        }



    }


    Info<< nl << "Write " << dictName << " ..." << endl;

    configDict.set("boundingBox", boundingBoxPoints);

    configDict.regIOobject::write();

    Info<< "\nEnd\n" << endl;

    return 0;
}


// ************************************************************************* //
