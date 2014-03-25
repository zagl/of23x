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
    createInclude

Description
    Writes configuration parameters to includeDict.

\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "IOdictionary.H"
#include "Time.H"
#include "vectorField.H"
#include "IFstream.H"
#include "Tuple2.H"
#include "Pair.H"
#include "cellSet.H"
#include "HashTable.H"
#include "dictionary.H"
#include "Ostream.H"
#include "Pstream.H"
#include "unitConversion.H"

using namespace Foam;

typedef Pair<word> wordPair;
typedef List<Pair<word> > wordPairList;

typedef Tuple2<label, scalar> Level;
typedef List<Tuple2<label, scalar> > levelList;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::noParallel();
//    argList::validArgs.append("surfaceFile");
    #include "setRootCase.H"
    #include "createTime.H"
    runTime.functionObjects().off();

    const word includeDictName("includeDict");
    IOdictionary includeDict
    (
        IOobject
        (
            includeDictName,
            runTime.system(),
            runTime,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        )
    );

    const word dictName("configDict");
    #include "setSystemRunTimeDictionaryIO.H"
    IOdictionary dict(dictIO);

    dictionary config = dict.subDict("config");
    const dictionary& configDict = dict.subDict("config");
    const dictionary& solidsDict = configDict.subDict("solids");
    const dictionary& fluidsDict = configDict.subDict("fluids");
    const dictionary& rotationsDict = configDict.subDict("rotations");
    const dictionary& contactsDict = configDict.subDict("contacts");
    const dictionary& domainDict = configDict.subDict("domain");

    wordList fluids;
    forAllConstIter(dictionary, fluidsDict, iter)
    {
        fluids.append( iter().keyword() );
    }

    wordList solids;
    forAllConstIter(dictionary, solidsDict, iter)
    {
        solids.append( iter().keyword() );
    }

    wordList regions;
    regions.append(fluids);
    regions.append(solids);

/*---------------------------------------------------------------------------*\
                                    blockMesh
\*---------------------------------------------------------------------------*/

    scalar globalLength  = readScalar(domainDict.lookup("elementLength"));
    scalar elementGradient = readScalar(domainDict.lookup("gradient"));
    vectorField boundingBox = configDict.lookup("boundingBox");
    vectorField distance = domainDict.lookup("width");

    vectorField l(4);
    vectorField divisions(3);
    vectorField edgeGradient(3);
    for ( int i=0; i<3; i++ )
    {
        l[0][i] = boundingBox[0][i] - distance[0][i];
        l[1][i] = boundingBox[0][i];
        l[2][i] = boundingBox[1][i];
        l[3][i] = boundingBox[1][i] + distance[1][i];

        divisions[0][i] = int(Foam::log((distance[0][i])/globalLength
            * (elementGradient - 1) + 1)/Foam::log(elementGradient ) + 0.5);

        divisions[1][i] = int((boundingBox[1][i] - boundingBox[0][i])
            / globalLength  + 0.5);

        divisions[2][i] = int(Foam::log((distance[1][i])/globalLength 
            * (elementGradient - 1) + 1)/Foam::log(elementGradient ) + 0.5);

        edgeGradient[0][i] = 1/Foam::pow(elementGradient, divisions[0][i]-1);
        edgeGradient[1][i] = 1;
        edgeGradient[2][i] = Foam::pow(elementGradient, divisions[2][i]-1);
    }

    vectorField vertices;
    for ( int k=0; k<4; k++ )
    {
        for ( int j=0; j<4; j++ )
        {
            for ( int i=0; i<4; i++ )
            {
                vector p(l[i].x(), l[j].y(), l[k].z());
                vertices.append(p);
            }
        }
    }

    wordList blocks;
    for ( int k=0; k<3; k++ )
    {
        for ( int j=0; j<3; j++ )
        {
            for ( int i=0; i<3; i++ )
            {
                labelList corners(8);
                for ( int n=0; n<8; n++ )
                {
                    label corner = i+((n+1)>>1&1) + 4*(j+(n>>1&1))
                        + 16*(k+(n>>2));
                    corners[n] = corner;
                }
                vector division
                (
                    divisions[i].x(),
                    divisions[j].y(),
                    divisions[k].z()
                );
                vector gradient
                (
                    edgeGradient[i].x(),
                    edgeGradient[j].y(),
                    edgeGradient[k].z()
                );

                OStringStream block;
                block << "hex " << corners << division 
                    << " simpleGrading " << gradient;
                blocks.append(block.str());
            }
        }
    }

    dictionary blockMesh;
    blockMesh.add("blocks", blocks);
    blockMesh.add("vertices", vertices);

    config.add("blockMesh", blockMesh);


/*---------------------------------------------------------------------------*\
                                  snappyHexMesh
\*---------------------------------------------------------------------------*/

    dictionary snappyHexMesh;
    dictionary geometry;
    dictionary refinementSurfaces;
    dictionary refinementRegions;

    fileName constantDir("constant");
    fileName triSurfaceDir(constantDir/"triSurface");
    fileNameList triSurfaces = readDir(triSurfaceDir);
    HashTable<word> surfFileNames;
    forAll( triSurfaces, surfI )
    {
        surfFileNames.insert(triSurfaces[surfI].lessExt(), triSurfaces[surfI]);
    }

    wordList geomTypes
    (
        IStringStream("(solids blanks baffles rotations refinements)")()
    );

    forAll( geomTypes, i )
    {
        word geomType = geomTypes[i];
        forAllConstIter(dictionary, configDict.subDict(geomType), iter)
        {
            const dictionary& dict = iter().dict();
            word name = iter().keyword();

            dictionary geomDict;
            geomDict.add("name", name);
            geomDict.add("type", "triSurfaceMesh");
            geometry.add(surfFileNames[name], geomDict);

            scalar localLength = readScalar(dict.lookup("elementLength"));
            label refinementLevel =
                int(Foam::log(globalLength/localLength)
                / Foam::log(2.0) + 0.5);

            if ( geomType == "refinements" )
            {
                dictionary region;
                levelList levels(1);
                levels[0] = Level(1.0, refinementLevel);
                region.add("levels", levels);
                region.add("mode", "inside");
                refinementRegions.add(name, region);
            }
            else
            {
                dictionary surface;
                Pair<label> level(2, refinementLevel);
                surface.add("level", level);
                if ( geomType == "solids" )
                {
                    surface.add("cellZone", name);
                    surface.add("faceZone", name);
                    surface.add("cellZoneInside", "inside");
                }
                else if ( geomType == "baffles" )
                {
                    surface.add("faceZone", name);
                }
                refinementSurfaces.add(name, surface);
            }
        }
    }

    vector locationInMesh = boundingBox[0] + vector(0.00001, 0.00001, 0.00001);

    snappyHexMesh.add("geometry", geometry);
    snappyHexMesh.add("refinementSurfaces", refinementSurfaces);
    snappyHexMesh.add("refinementRegions", refinementRegions);
    snappyHexMesh.add("locationInMesh", locationInMesh);
    config.add("snappyHexMesh", snappyHexMesh);


/*---------------------------------------------------------------------------*\
                                changeDictionary
\*---------------------------------------------------------------------------*/

    dictionary changeDictionary;
    dictionary changeDictRegions;

    forAll( regions, i )
    {
        dictionary emptyDict;
        changeDictRegions.add(regions[i], emptyDict);
    }

    wordList groups;
    dictionary isolation;
    isolation.add("type", "wall");
    isolation.add("inGroups", groups);

    dictionary isolationsDict = contactsDict.subDict("isolations");
    wordPairList isolationPairs = isolationsDict.lookup("pairs");
    forAll( isolationPairs, i )
    {
        wordPair isolationPair = isolationPairs[i];
        for ( int j=0; j<2; j++ )
        {
            int k = (j+1)%2;
            word first = isolationPair[j];
            word second = isolationPair[k];
            word contactName = first + "_to_" + second;

            changeDictRegions.subDict(first).add(contactName, isolation);
        }
    }
    changeDictionary.add("regions", changeDictRegions);

    word radiation = configDict.subDict("radiation").lookup("model");
    word coupledWallType =
        radiation == "none" ? "coupledWall" : "coupledRadiationWall";
    changeDictionary.add("coupledWallType", coupledWallType);


    vector gravity = configDict.subDict("convection").lookup("gravity");
    word outlet = "noOutlet";
    const char* componentNames[] = {"X", "Y", "Z"};
    forAll( gravity, i )
    {
        if (gravity[i] < 0)
        {
            outlet = "max" + word(componentNames[i]);
        }
        else if (gravity[i] > 0)
        {
            outlet = "min" + word(componentNames[i]);
        }
    }
    changeDictionary.add("outlet", outlet);
    config.add("changeDictionary", changeDictionary);

/*---------------------------------------------------------------------------*\
                                   fields
\*---------------------------------------------------------------------------*/

    dictionary boundaryFields;
    dictionary fieldT;

    forAll( regions, i )
    {
        dictionary emptyDict;
        fieldT.add(regions[i], emptyDict);
    }

    dictionary boundaryField;
    boundaryField.add("type", "compressible::myturbulentTemperatureCoupledBaffleMixed");
    boundaryField.add("Tnbr", "T");
    boundaryField.add("kappa", "solidThermo");
    boundaryField.add("kappaName", "none");
    boundaryField.add("value", word("uniform $:config.temperature.start"));

    dictionary resistancesDict = contactsDict.subDict("resistances");
    wordPairList resistancePairs = resistancesDict.lookup("pairs");
    scalarList resistances = resistancesDict.lookup("resistances");
    forAll( resistancePairs, i )
    {
        wordPair resistancePair = resistancePairs[i];
        scalar resistance = resistances[i];

        dictionary boundary = boundaryField;
        boundary.add("resistance", resistance);

        for ( int j=0; j<2; j++ )
        {
            int k = (j+1)%2;
            word first = resistancePair[j];
            word second = resistancePair[k];
            word contactName = first + "_to_" + second;

            fieldT.subDict(first).add(contactName, boundary);
        }
    }

    dictionary thermalLayersDict = contactsDict.subDict("thermalLayers");
    wordPairList thermalLayerPairs = thermalLayersDict.lookup("pairs");
    scalarListList thicknessLayers = thermalLayersDict.lookup("thicknessLayers");
    scalarListList kappaLayers = thermalLayersDict.lookup("kappaLayers");
    forAll( thermalLayerPairs, i )
    {
        wordPair thermalLayerPair = thermalLayerPairs[i];
        scalarList thicknessLayer = thicknessLayers[i];
        scalarList kappaLayer = kappaLayers[i];

        dictionary boundary = boundaryField;
        boundary.add("thicknessLayers", thicknessLayer);
        boundary.add("kappaLayers", kappaLayer);

        for ( int j=0; j<2; j++ )
        {
            int k = (j+1)%2;
            word first = thermalLayerPair[j];
            word second = thermalLayerPair[k];
            word contactName = first + "_to_" + second;

            fieldT.subDict(first).add(contactName, boundary);
        }
    }

    boundaryFields.add("T", fieldT);

    config.add("boundaryFields", boundaryFields);

/*---------------------------------------------------------------------------*\
                                  fvOptions
\*---------------------------------------------------------------------------*/
    dictionary fvOptions;
    forAllConstIter(dictionary, solidsDict, iter)
    {
        const dictionary& dict = iter().dict();
        word name = iter().keyword();
        scalar power = readScalar(dict.lookup("power"));

        dictionary solid;

        if ( power > 0 ) 
        {
            Pair<scalar> h(power, 0.0);
            dictionary injectionRateSuSp;
            injectionRateSuSp.add("h", h);

            dictionary coeffs;
            coeffs.add("volumeMode", "absolute");
            coeffs.add("injectionRateSuSp", injectionRateSuSp);

            dictionary heatSource;
            heatSource.add("active", "true");
            heatSource.add("selectionMode", "all");
            heatSource.add("type", "scalarSemiImplicitSource");
            heatSource.add("scalarSemiImplicitSourceCoeffs", coeffs);
            solid.add("heatSource", heatSource);
        }

        fvOptions.add(name, solid);
    }

    forAllConstIter(dictionary, rotationsDict, iter)
    {
        const dictionary& dict = iter().dict();
        word name = iter().keyword();
        scalar rpm = readScalar(dict.lookup("rpm"));
        vectorField axis = dict.lookup("axis");

        dictionary coeffs;
        coeffs.add("origin", axis[0]);
        coeffs.add("axis", axis[1] - axis[0]);
        coeffs.add("omega", constant::mathematical::twoPi * rpm / 60);

        dictionary option;
        option.add("active", "true");
        option.add("selectionMode", "cellZone");
        option.add("cellZone", name);
        option.add("type", "MRFSource");
        option.add("MRFSourceCoeffs", coeffs);

        dictionary fluid;
        fluid.add(name, option);

        fvOptions.add("FLUID", fluid);
    }
    config.add("fvOptions", fvOptions);

/*---------------------------------------------------------------------------*\
                                  topoSet
\*---------------------------------------------------------------------------*/

    List<dictionary> actions;
    forAllConstIter(dictionary, rotationsDict, iter)
    {
        const dictionary& dict = iter().dict();
        word name = iter().keyword();
        vectorField axis = dict.lookup("axis");
        scalar radius = readScalar(dict.lookup("radius"));
        vector v = axis[1] - axis[0];
        vector extension = v / mag(v) * 0.001;

        dictionary source;
        source.add("p1", axis[0] - extension);
        source.add("p2", axis[1] + extension);
        source.add("radius", radius);
        dictionary action;
        action.add("name", name);
        action.add("type", "cellSet");
        action.add("action", "new");
        action.add("source", "cylinderToCell");
        action.add("sourceInfo", source);
        actions.append(action);

        source.clear();
        source.add("set", name);
        action.clear();
        action.add("name", name);
        action.add("type", "cellZoneSet");
        action.add("action", "new");
        action.add("source", "setToCellZone");
        action.add("sourceInfo", source);
        actions.append(action);

    }
    
    dictionary topoSet;
    topoSet.add("actions", actions);
    config.add("topoSet", topoSet);

/*---------------------------------------------------------------------------*\
                               regionProperties
\*---------------------------------------------------------------------------*/

    dictionary regionProperties;
    regionProperties.set("fluids", fluids);
    regionProperties.set("solids", solids);
    config.add("regionProperties", regionProperties);

/*---------------------------------------------------------------------------*\
                                  modifyRegions
\*---------------------------------------------------------------------------*/

    word defaultCellZone;
    List<dictionary> cellZones;

    forAllConstIter(dictionary, fluidsDict, iter)
    {
        const dictionary& dict = iter().dict();
        word name = iter().keyword();

        pointField insidePoints = dict.lookup("insidePoints");

        if (insidePoints.size() > 0)
        {
            dictionary cellZone;
            cellZone.add("name", name);
            cellZone.add("insidePoints", insidePoints);
            cellZones.append(cellZone);
        }
        else
        {
            defaultCellZone = name;
        }
    }

    dictionary modifyRegions;
    modifyRegions.set("defaultCellZone", defaultCellZone);
    modifyRegions.set("cellZones", cellZones);
    config.add("modifyRegions", modifyRegions);


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    includeDict.add("config", config);

    Info<< nl << "Write " << includeDictName << " ..." << endl;

    includeDict.regIOobject::write();

    Info<< "\nEnd\n" << endl;

    return 0;
}


// ************************************************************************* //


