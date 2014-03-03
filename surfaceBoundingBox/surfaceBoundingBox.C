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

    const word dictName("variableConfigDict");
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
    const dictionary& solidsDict = configDict.subDict("solids");

    List<point> boundPoints;

    forAllConstIter(dictionary, solidsDict, iter)
    {
        if (!iter().isDict())
        {
            continue;
        }

        const dictionary& solidDict = iter().dict();
        const word surfName = iter().keyword();
        word surfFileType = solidDict.lookup("fileType");
        const fileName surfFileName = surfName + "." + surfFileType;

        Info<< "Reading surface from " << surfFileName << " ..." << endl;

        triSurface surf(runTime.constantPath()/"triSurface"/surfFileName);

        boundBox bound(surf.points());
        boundPoints.append(bound.min());
        boundPoints.append(bound.max());
    }

    pointField boundingBoxPoints(2);
    boundBox boundingBox(boundPoints);
    boundingBoxPoints[0] = boundingBox.min();
    boundingBoxPoints[1] = boundingBox.max();

    Info<< nl << "Bounding Box: " << boundingBox << nl << nl;

    Info<<"Write " << dictName << " ..." << endl;

    configDict.set("boundingBox", boundingBoxPoints);

    configDict.regIOobject::write();

    Info<< "\nEnd\n" << endl;

    
    return 0;
}


// ************************************************************************* //
