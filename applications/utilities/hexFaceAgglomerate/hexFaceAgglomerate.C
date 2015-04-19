/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
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
    hexFaceAgglomerate

Description
    Agglomerate boundary faces by hex regions.
    It writes a map from the fine to coarse grid.

\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "fvMesh.H"
#include "Time.H"
#include "boundaryMesh.H"
#include "volFields.H"
#include "CompactListList.H"
#include "unitConversion.H"
#include "labelListIOList.H"
#include "syncTools.H"
#include "globalIndex.H"
#include "labelVector.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    #include "addRegionOption.H"
    #include "addDictOption.H"
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createNamedMesh.H"

    const word dictName("viewFactorsDict");

    #include "setConstantMeshDictionaryIO.H"

    // Read control dictionary
    const IOdictionary agglomDict(dictIO);

    bool writeAgglom = readBool(agglomDict.lookup("writeFacesAgglomeration"));


    const polyBoundaryMesh& boundary = mesh.boundaryMesh();


    labelListIOList finalAgglom
    (
        IOobject
        (
            "finalAgglom",
            mesh.facesInstance(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            false
        ),
        boundary.size()
    );

    label nCoarseFaces = 0;

    forAllConstIter(dictionary, agglomDict, iter)
    {
        labelList patchIds = boundary.findIndices(iter().keyword());
        forAll(patchIds, i)
        {
            label patchI =  patchIds[i];
            const polyPatch& pp = boundary[patchI];

            if (!pp.coupled())
            {
                Info << "\nAgglomerating patch : " << pp.name() << endl;

                HashTable<label, word> indexMap1(10);
                indexMap1.insert( "hallo", 1 );
                Info<< indexMap1["hallo"] << nl;

//                HashTable<label, Point> indexMap(10);
//                indexMap.insert( Point(0, 0, 0), 1 );

//                Info<< indexMap["hallo"] << nl;

                pointField faceCentres = pp.faceCentres();
                vectorField faceNormals = pp.faceNormals();

                labelList patchAgglomeration(pp.size());

                labelList agglomIDs(2);
                agglomIDs[0] = -1;
                agglomIDs[1] = -1;

                label currentID = 0;


                forAll(faceCentres, faceI)
                {
                    point faceCentre = faceCentres[faceI];

                    if (faceCentre.x() > 0)
                    {
                        if (agglomIDs[0] == -1)
                        {
                            agglomIDs[0] = currentID;
                            currentID++;
                        }
                        patchAgglomeration[faceI] = agglomIDs[0];
                    }
                    else
                    {
                        if (agglomIDs[1] == -1)
                        {
                            agglomIDs[1] = currentID;
                            currentID++;
                        }
                        patchAgglomeration[faceI] = agglomIDs[1];
                    }
                }






                finalAgglom[patchI] = patchAgglomeration;


                if (finalAgglom[patchI].size())
                {
                    nCoarseFaces += max(finalAgglom[patchI] + 1);
                }
            }
        }
    }


    // - All patches which are not agglomarated are identity for finalAgglom
    forAll(boundary, patchId)
    {
        if (finalAgglom[patchId].size() == 0)
        {
            finalAgglom[patchId] = identity(boundary[patchId].size());
        }
    }

    // Sync agglomeration across coupled patches
    labelList nbrAgglom(mesh.nFaces() - mesh.nInternalFaces(), -1);

    forAll(boundary, patchId)
    {
        const polyPatch& pp = boundary[patchId];
        if (pp.coupled())
        {
            finalAgglom[patchId] = identity(pp.size());
            forAll(pp, i)
            {
                nbrAgglom[pp.start() - mesh.nInternalFaces() + i] =
                    finalAgglom[patchId][i];
            }
        }
    }

    syncTools::swapBoundaryFaceList(mesh, nbrAgglom);
    forAll(boundary, patchId)
    {
        const polyPatch& pp = boundary[patchId];
        if (pp.coupled() && !refCast<const coupledPolyPatch>(pp).owner())
        {
            forAll(pp, i)
            {
                finalAgglom[patchId][i] =
                    nbrAgglom[pp.start() - mesh.nInternalFaces() + i];
            }
        }
    }

    finalAgglom.write();

    if (writeAgglom)
    {
        globalIndex index(nCoarseFaces);
        volScalarField facesAgglomeration
        (
            IOobject
            (
                "facesAgglomeration",
                mesh.time().timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh,
            dimensionedScalar("facesAgglomeration", dimless, 0)
        );

        label coarsePatchIndex = 0;
        forAll(boundary, patchId)
        {
            const polyPatch& pp = boundary[patchId];
            if (pp.size() > 0)
            {
                fvPatchScalarField& bFacesAgglomeration =
                    facesAgglomeration.boundaryField()[patchId];

                forAll(bFacesAgglomeration, j)
                {
                    bFacesAgglomeration[j] =
                        index.toGlobal
                        (
                            Pstream::myProcNo(),
                            finalAgglom[patchId][j] + coarsePatchIndex
                        );
                }

                coarsePatchIndex += max(finalAgglom[patchId]) + 1;
            }
        }

        Info<< "\nWriting facesAgglomeration" << endl;
        facesAgglomeration.write();
    }

    Info<< "End\n" << endl;
    return 0;
}


// ************************************************************************* //
