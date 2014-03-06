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
    modifyRegions

Description
    Modify regions created by splitMeshRegions -makeCellZones
\*---------------------------------------------------------------------------*/


#include "argList.H"
#include "Time.H"
#include "polyMesh.H"
#include "topoSetSource.H"
#include "cellSet.H"
#include "faceSet.H"
#include "pointSet.H"
#include "globalMeshData.H"
#include "timeSelector.H"
#include "IOobjectList.H"
#include "cellZoneSet.H"
#include "faceZoneSet.H"
#include "pointZoneSet.H"



using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //



int main(int argc, char *argv[])
{
    timeSelector::addOptions(true, false);
    #include "addDictOption.H"
    argList::addBoolOption
    (
        "merge",
        "Merge new regions into one single region."
    );

    #include "setRootCase.H"
    #include "createTime.H"

    instantList timeDirs = timeSelector::selectIfPresent(runTime, args);

    #include "createNamedPolyMesh.H"

    const word dictName("modifyRegionsDict");
    #include "setSystemMeshDictionaryIO.H"
    IOdictionary modifyRegionsDict(dictIO);

    const bool relativeSize = readBool(modifyRegionsDict.lookup("relativeSize"));
    label minRegionSize;

    if (relativeSize)
    {
        const scalar ratio = readScalar(modifyRegionsDict.lookup("minRegionSize"));
        minRegionSize = int(mesh.nCells()*ratio + 0.5);
    }
    else
    {
        minRegionSize = readLabel(modifyRegionsDict.lookup("minRegionSize"));
    }


    forAll(timeDirs, timeI)
    {
        runTime.setTime(timeDirs[timeI], timeI);
        Info<< "Time = " << runTime.timeName() << endl;

        const cellZoneMesh& cellZones = mesh.cellZones();
        const wordList cellZoneNames = cellZones.names();
        wordList solids;
        wordList regions;

        forAll( cellZoneNames, i )
        {
            word cellZoneName = cellZoneNames[i];
            if ( regExp("region.*").match(cellZoneName) )
                regions.append( cellZoneName );
            else
                solids.append( cellZoneName );
        }

        forAll( regions, i )
        {
            word region = regions[i];
            autoPtr<topoSet> currentSet;
            const word setType = "cellSet";

            currentSet = topoSet::New
            (
                setType,
                mesh,
                region,
                IOobject::MUST_READ
            );

            Info<< "Read set " << currentSet().type() << " "
                << region<< " with size "
                << returnReduce(currentSet().size(), sumOp<label>())
                << endl;

            forAll( solids, i )
            {
                word solid = solids[i];
                word sourceType = "cellToCell";
                dictionary sourceInfo;
                sourceInfo.add("set", solid);
                topoSetSource::setAction action = topoSetSource::DELETE;

                Info<< " Applying source " << sourceType << endl;
                autoPtr<topoSetSource> source = topoSetSource::New
                (
                    sourceType,
                    mesh,
                    sourceInfo
                );

                source().applyToSet(action, currentSet());
                currentSet().write();
            }
        }
        forAll( regions, i )
        {
            word region = regions[i];
            autoPtr<topoSet> currentSet;
            const word setType = "cellSet";
            currentSet = topoSet::New
            (
                setType,
                mesh,
                region,
                IOobject::MUST_READ
            );

            label regionSize = returnReduce(currentSet().size(), sumOp<label>());

            Info<< "Read set " << currentSet().type() << " "
                << region<< " with size "
                << returnReduce(currentSet().size(), sumOp<label>())
                << endl
                << currentSet().size()
                << endl;

            autoPtr<topoSet> removeSet;
            removeSet = topoSet::New
            (
                setType,
                mesh,
                "remove",
                10000
            );

            if ( regionSize > minRegionSize )
            {
            }
            else if ( regionSize != 0 )
            {
                word sourceType = "cellToCell";
                dictionary sourceInfo;
                sourceInfo.add("set", region);
                topoSetSource::setAction action = topoSetSource::ADD;

                Info<< " Applying source " << sourceType << endl;
                autoPtr<topoSetSource> source = topoSetSource::New
                (
                    sourceType,
                    mesh,
                    sourceInfo
                );

                source().applyToSet(action, removeSet());
                removeSet().write();
            }
        }
    }










    Info<< "\nEnd\n" << endl;

    return 0;
}


// ************************************************************************* //
