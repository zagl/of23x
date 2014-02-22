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
    Prints the bounding box of a surface.

\*---------------------------------------------------------------------------*/

#include "triSurface.H"
#include "triSurfaceSearch.H"
#include "argList.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::noParallel();
    argList::validArgs.append("surfaceFile");
    argList args(argc, argv);

    const fileName surfFileName = args[1];

    Info<< "Reading surface from " << surfFileName << " ..." << nl << endl;

    // Read
    // ~~~~

    triSurface surf(surfFileName);

    // write bounding box
    Info<<"Bounding Box: " << boundBox(surf.points()) << nl;

    Info<< "\nEnd\n" << endl;

    return 0;
}


// ************************************************************************* //
