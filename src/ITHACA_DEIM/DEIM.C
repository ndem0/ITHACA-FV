/*---------------------------------------------------------------------------*\
     ██╗████████╗██╗  ██╗ █████╗  ██████╗ █████╗       ███████╗██╗   ██╗
     ██║╚══██╔══╝██║  ██║██╔══██╗██╔════╝██╔══██╗      ██╔════╝██║   ██║
     ██║   ██║   ███████║███████║██║     ███████║█████╗█████╗  ██║   ██║
     ██║   ██║   ██╔══██║██╔══██║██║     ██╔══██║╚════╝██╔══╝  ╚██╗ ██╔╝
     ██║   ██║   ██║  ██║██║  ██║╚██████╗██║  ██║      ██║      ╚████╔╝
     ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝      ╚═╝       ╚═══╝

 * In real Time Highly Advanced Computational Applications for Finite Volumes
 * Copyright (C) 2017 by the ITHACA-FV authors
-------------------------------------------------------------------------------

License
    This file is part of ITHACA-FV

    ITHACA-FV is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ITHACA-FV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with ITHACA-FV. If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "DEIM.H"

// Template function constructor
template<typename T>
DEIM<T>::DEIM (PtrList<T>& s, int MaxModes, word FunctionName)
    :
    SnapShotsMatrix(s),
    MaxModes(MaxModes),
    FunctionName(FunctionName)
{
    Eigen::MatrixXd A;
    Eigen::VectorXd b;
    Eigen::VectorXd c;
    Eigen::VectorXd r;
    Eigen::VectorXd rho(1);
    modes = ITHACAPOD::DEIMmodes(SnapShotsMatrix, MaxModes, FunctionName);
    MatrixModes = Foam2Eigen::PtrList2Eigen(modes);
    int ind_max, c1;
    double max = MatrixModes.cwiseAbs().col(0).maxCoeff(&ind_max, &c1);
    rho(0) = max;
    magicPoints.append(ind_max);
    U = MatrixModes.col(0);
    P.resize(MatrixModes.rows(), 1);
    P.insert(ind_max, 0) = 1;

    for (int i = 1; i < MaxModes; i++)
    {
        A = P.transpose() * U;
        b = P.transpose() * MatrixModes.col(i);
        c = A.lu().solve(b);
        r = MatrixModes.col(i) - U * c;
        max = r.cwiseAbs().maxCoeff(&ind_max, &c1);
        P.conservativeResize(MatrixModes.rows(), i + 1);
        P.insert(ind_max, i) = 1;
        U.conservativeResize(MatrixModes.rows(), i + 1);
        U.col(i) =  MatrixModes.col(i);
        rho.conservativeResize(i + 1);
        rho(i) = max;
        magicPoints.append(ind_max);
    }

    MatrixOnline = U * ((P.transpose() * U).inverse());
}

template DEIM<volScalarField>::DEIM(PtrList<volScalarField>& s, int MaxModes,
                                    word FunctionName);
template DEIM<volVectorField>::DEIM(PtrList<volVectorField>& s, int MaxModes,
                                    word FunctionName);


template<typename T>
DEIM<T>::DEIM (PtrList<T>& s, int MaxModesA, int MaxModesB, word MatrixName)
    :
    SnapShotsMatrix(s),
    MaxModesA(MaxModesA),
    MaxModesB(MaxModesB),
    MatrixName(MatrixName)

{
    Eigen::MatrixXd AA;
    Eigen::VectorXd bA;
    Eigen::MatrixXd cA;
    Eigen::SparseMatrix<double> rA;
    Eigen::VectorXd rhoA(1);
    Matrix_Modes = ITHACAPOD::DEIMmodes(SnapShotsMatrix, MaxModesA, MaxModesB,
                                        MatrixName);
    sizeM = std::get<1>(Matrix_Modes)[0].rows();
    int ind_rowA, ind_colA, xyz_rowA, xyz_colA;
    ind_rowA = ind_colA = xyz_rowA = xyz_colA = 0;
    double maxA = EigenFunctions::max(std::get<0>(Matrix_Modes)[0], ind_rowA,
                                      ind_colA);
    int ind_rowAOF = ind_rowA;
    int ind_colAOF = ind_colA;
    check3D_indices(ind_rowAOF, ind_colAOF, xyz_rowA, xyz_colA);
    Pair <int> indA(ind_rowAOF, ind_colAOF);
    Pair <int> xyzA(xyz_rowA, xyz_colA);
    xyz_A.append(xyzA);
    rhoA(0) = maxA;
    magicPointsA.append(indA);
    UA.append(std::get<0>(Matrix_Modes)[0]);
    Eigen::SparseMatrix<double> Pnow(std::get<0>(Matrix_Modes)[0].rows(),
                                     std::get<0>(Matrix_Modes)[0].cols());
    Pnow.insert(ind_rowA, ind_colA) = 1;
    PA.append(Pnow);

    for (int i = 1; i < MaxModesA; i++)
    {
        AA = EigenFunctions::innerProduct(PA, UA);
        bA = EigenFunctions::innerProduct(PA, std::get<0>(Matrix_Modes)[i]);
        cA = AA.colPivHouseholderQr().solve(bA);
        rA = std::get<0>(Matrix_Modes)[i] - EigenFunctions::MVproduct(UA, cA);
        double maxA = EigenFunctions::max(rA, ind_rowA, ind_colA);
        rhoA.conservativeResize(i + 1);
        rhoA(i) = maxA;
        int ind_rowAOF = ind_rowA;
        int ind_colAOF = ind_colA;
        check3D_indices(ind_rowAOF, ind_colAOF, xyz_rowA, xyz_colA);
        Pair <int> indA(ind_rowAOF, ind_colAOF);
        Pair <int> xyzA(xyz_rowA, xyz_colA);
        xyz_A.append(xyzA);
        magicPointsA.append(indA);
        UA.append(std::get<0>(Matrix_Modes)[i]);
        Eigen::SparseMatrix<double> Pnow(std::get<0>(Matrix_Modes)[0].rows(),
                                         std::get<0>(Matrix_Modes)[0].cols());
        Pnow.insert(ind_rowA, ind_colA) = 1;
        PA.append(Pnow);
    }

    Eigen::MatrixXd Aaux = EigenFunctions::innerProduct(PA, UA).inverse();
    MatrixOnlineA = EigenFunctions::MMproduct(UA, Aaux);
    Eigen::MatrixXd AB;
    Eigen::VectorXd bB;
    Eigen::VectorXd cB;
    Eigen::VectorXd rB;
    Eigen::VectorXd rhoB(1);
    int ind_rowB, xyz_rowB, c1;
    double maxB = std::get<1>(Matrix_Modes)[0].maxCoeff(&ind_rowB, &c1);
    int ind_rowBOF = ind_rowB;
    check3D_indices(ind_rowBOF, xyz_rowB);
    rhoB(0) = maxB;
    xyz_B.append(xyz_rowB);
    magicPointsB.append(ind_rowBOF);
    UB = std::get<1>(Matrix_Modes)[0];
    PB.resize(UB.rows(), 1);
    PB.insert(ind_rowB, 0) = 1;

    for (int i = 1; i < MaxModesB; i++)
    {
        AB = PB.transpose() * UB;
        bB = PB.transpose() * std::get<1>(Matrix_Modes)[i];
        cB = AB.colPivHouseholderQr().solve(bB);
        rB = std::get<1>(Matrix_Modes)[i] - UB * cB;
        maxB = rB.cwiseAbs().maxCoeff(&ind_rowB, &c1);
        ind_rowBOF = ind_rowB;
        check3D_indices(ind_rowBOF, xyz_rowB);
        xyz_B.append(xyz_rowB);
        PB.conservativeResize((std::get<1>(Matrix_Modes)[i]).size(), i + 1);
        PB.insert(ind_rowB, i) = 1;
        UB.conservativeResize((std::get<1>(Matrix_Modes)[i]).size(), i + 1);
        UB.col(i) =  std::get<1>(Matrix_Modes)[i];
        rhoB.conservativeResize(i + 1);
        rhoB(i) = maxB;
        magicPointsB.append(ind_rowBOF);
    }

    if (MaxModesB == 1 && std::get<1>(Matrix_Modes)[0].norm() < 1e-8)
    {
        MatrixOnlineB = Eigen::MatrixXd::Zero(std::get<1>(Matrix_Modes)[0].rows(), 1);
    }
    else if (MaxModesB != 1)
    {
        MatrixOnlineB = UB * ((PB.transpose() * UB).inverse());
    }
    else
    {
        MatrixOnlineB = UB;
    }
}

template DEIM<fvScalarMatrix>::DEIM (PtrList<fvScalarMatrix>& s, int MaxModesA,
                                     int MaxModesB, word MatrixName);
template DEIM<fvVectorMatrix>::DEIM (PtrList<fvVectorMatrix>& s, int MaxModesA,
                                     int MaxModesB, word MatrixName);


template<typename T>
template<typename S>
PtrList<S> DEIM<T>::generateSubmeshes(int layers, fvMesh& mesh, S field,
                                      int secondTime)
{
    fvMeshSubset* submesh;
    PtrList<S> fields;
    List<int> indices;
    volScalarField Indici
    (
        IOobject
        (
            FunctionName + "_indices",
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionSet(0, 0, 0, 0, 0)
    );

    if (!secondTime)
    {
        Indici = Indici * 0;
    }

    for (int i = 0; i < magicPoints.size(); i++)
    {
        submesh = new fvMeshSubset(mesh);
        indices = ITHACAutilities::getIndices(mesh, magicPoints[i], layers);

        if (!secondTime)
        {
            ITHACAutilities::assignONE(Indici, indices);
        }

        std::cout.setstate(std::ios_base::failbit);
#if OPENFOAM >= 1812
        submesh->setCellSubset(indices);
#else
        submesh->setLargeCellSubset(indices);
#endif
        submesh->subMesh().fvSchemes::readOpt() = mesh.fvSchemes::readOpt();
        submesh->subMesh().fvSolution::readOpt() = mesh.fvSolution::readOpt();
        submesh->subMesh().fvSchemes::read();
        submesh->subMesh().fvSolution::read();
        std::cout.clear();
        S f = submesh->interpolate(field);
        fields.append(f);

        if (!secondTime)
        {
            submeshList.append(submesh);
        }
    }

    if (!secondTime)
    {
        localMagicPoints = global2local(magicPoints, submeshList);
        ITHACAstream::exportSolution(Indici, "1", "./ITHACAoutput/DEIM/" + FunctionName
                                    );
    }

    return fields;
}

template PtrList<volScalarField> DEIM<volScalarField>::generateSubmeshes(
    int layers, fvMesh& mesh, volScalarField field,
    int secondTime);
template PtrList<volVectorField> DEIM<volVectorField>::generateSubmeshes(
    int layers, fvMesh& mesh, volVectorField field,
    int secondTime);
template PtrList<volScalarField> DEIM<volVectorField>::generateSubmeshes(
    int layers, fvMesh& mesh, volScalarField field,
    int secondTime);
template PtrList<volVectorField> DEIM<volScalarField>::generateSubmeshes(
    int layers, fvMesh& mesh, volVectorField field,
    int secondTime);

template<typename T>
template<typename S>
PtrList<S> DEIM<T>::generateSubmeshesMatrix(int layers, fvMesh& mesh, S field,
        int secondTime)
{
    fvMeshSubset* submesh;
    PtrList<S> fieldsA;
    List<int> indices;
    volScalarField Indici
    (
        IOobject
        (
            MatrixName + "_A_indices",
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionSet(0, 0, 0, 0, 0)
    );

    if (!secondTime)
    {
        Indici = Indici * 0;
    }

    fieldsA.resize(0);

    for (int i = 0; i < magicPointsA.size(); i++)
    {
        submesh = new fvMeshSubset(mesh);
        indices = ITHACAutilities::getIndices(mesh, magicPointsA[i].first(), layers);
        indices.append(ITHACAutilities::getIndices(mesh, magicPointsA[i].second(),
                       layers));

        if (!secondTime)
        {
            ITHACAutilities::assignONE(Indici, indices);
        }

        std::cout.setstate(std::ios_base::failbit);
#if OPENFOAM >= 1812
        submesh->setCellSubset(indices);
#else
        submesh->setLargeCellSubset(indices);
#endif
        submesh->subMesh().fvSchemes::readOpt() = mesh.fvSchemes::readOpt();
        submesh->subMesh().fvSolution::readOpt() = mesh.fvSolution::readOpt();
        submesh->subMesh().fvSchemes::read();
        submesh->subMesh().fvSolution::read();
        std::cout.clear();
        S f = submesh->interpolate(field);
        fieldsA.append(f);

        if (!secondTime)
        {
            submeshListA.append(submesh);
        }
    }

    if (!secondTime)
    {
        localMagicPointsA = global2local(magicPointsA, submeshListA);
        ITHACAstream::exportSolution(Indici, "1", "./ITHACAoutput/DEIM/" + MatrixName
                                    );
    }

    return fieldsA;
}

template PtrList<volScalarField> DEIM<fvScalarMatrix>::generateSubmeshesMatrix(
    int layers, fvMesh& mesh, volScalarField field, int secondTime);
template PtrList<volVectorField> DEIM<fvScalarMatrix>::generateSubmeshesMatrix(
    int layers, fvMesh& mesh, volVectorField field, int secondTime);
template PtrList<surfaceScalarField>
DEIM<fvScalarMatrix>::generateSubmeshesMatrix(int layers, fvMesh& mesh,
        surfaceScalarField field, int secondTime);
template PtrList<surfaceVectorField>
DEIM<fvScalarMatrix>::generateSubmeshesMatrix(int layers, fvMesh& mesh,
        surfaceVectorField field, int secondTime);
template PtrList<volScalarField> DEIM<fvVectorMatrix>::generateSubmeshesMatrix(
    int layers, fvMesh& mesh, volScalarField field, int secondTime);
template PtrList<volVectorField> DEIM<fvVectorMatrix>::generateSubmeshesMatrix(
    int layers, fvMesh& mesh, volVectorField field, int secondTime);
template PtrList<surfaceScalarField>
DEIM<fvVectorMatrix>::generateSubmeshesMatrix(int layers, fvMesh& mesh,
        surfaceScalarField field, int secondTime);
template PtrList<surfaceVectorField>
DEIM<fvVectorMatrix>::generateSubmeshesMatrix(int layers, fvMesh& mesh,
        surfaceVectorField field, int secondTime);

template<typename T>
template<typename S>
PtrList<S> DEIM<T>::generateSubmeshesVector(int layers, fvMesh& mesh, S field,
        int secondTime)
{
    fvMeshSubset* submesh;
    List<int> indices;
    PtrList<S> fieldsB;
    volScalarField Indici
    (
        IOobject
        (
            MatrixName + "_B_indices",
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionSet(0, 0, 0, 0, 0)
    );

    if (!secondTime)
    {
        Indici = Indici * 0;
    }

    fieldsB.resize(0);

    for (int i = 0; i < magicPointsB.size(); i++)
    {
        submesh = new fvMeshSubset(mesh);
        indices = ITHACAutilities::getIndices(mesh, magicPointsB[i], layers);

        if (!secondTime)
        {
            ITHACAutilities::assignONE(Indici, indices);
        }

        std::cout.setstate(std::ios_base::failbit);
#if OPENFOAM >= 1812
        submesh->setCellSubset(indices);
#else
        submesh->setLargeCellSubset(indices);
#endif
        submesh->subMesh().fvSchemes::readOpt() = mesh.fvSchemes::readOpt();
        submesh->subMesh().fvSolution::readOpt() = mesh.fvSolution::readOpt();
        submesh->subMesh().fvSchemes::read();
        submesh->subMesh().fvSolution::read();
        std::cout.clear();
        S f = submesh->interpolate(field);
        fieldsB.append(f);

        if (!secondTime)
        {
            submeshListB.append(submesh);
        }
    }

    if (!secondTime)
    {
        localMagicPointsB = global2local(magicPointsB, submeshListB);
        ITHACAstream::exportSolution(Indici, "1", "./ITHACAoutput/DEIM/" + MatrixName
                                    );
    }

    return fieldsB;
}

template PtrList<volScalarField> DEIM<fvScalarMatrix>::generateSubmeshesVector(
    int layers, fvMesh& mesh, volScalarField field, int secondTime);
template PtrList<volVectorField> DEIM<fvScalarMatrix>::generateSubmeshesVector(
    int layers, fvMesh& mesh, volVectorField field, int secondTime);
template PtrList<surfaceScalarField>
DEIM<fvScalarMatrix>::generateSubmeshesVector(int layers, fvMesh& mesh,
        surfaceScalarField field, int secondTime);
template PtrList<surfaceVectorField>
DEIM<fvScalarMatrix>::generateSubmeshesVector(int layers, fvMesh& mesh,
        surfaceVectorField field, int secondTime);
template PtrList<volScalarField> DEIM<fvVectorMatrix>::generateSubmeshesVector(
    int layers, fvMesh& mesh, volScalarField field, int secondTime);
template PtrList<volVectorField> DEIM<fvVectorMatrix>::generateSubmeshesVector(
    int layers, fvMesh& mesh, volVectorField field, int secondTime);
template PtrList<surfaceScalarField>
DEIM<fvVectorMatrix>::generateSubmeshesVector(int layers, fvMesh& mesh,
        surfaceScalarField field, int secondTime);
template PtrList<surfaceVectorField>
DEIM<fvVectorMatrix>::generateSubmeshesVector(int layers, fvMesh& mesh,
        surfaceVectorField field, int secondTime);


template<typename T>
List<int> DEIM<T>::global2local(List<int>& points,
                                PtrList<fvMeshSubset>& submeshList)
{
    List<int> localPoints;

    for (int i = 0; i < points.size(); i++)
    {
        for (int j = 0; j < submeshList[i].cellMap().size(); j++)
        {
            if (submeshList[i].cellMap()[j] == points[i])
            {
                localPoints.append(j);
                break;
            }
        }
    }

    return localPoints;
}

template<typename T>
List<Pair <int >> DEIM<T>::global2local(List<Pair <int >>& points,
                                        PtrList<fvMeshSubset>& submeshList)
{
    List< Pair <int>> localPoints(points.size());

    for (int i = 0; i < points.size(); i++)
    {
        for (int j = 0; j < submeshList[i].cellMap().size(); j++)
        {
            if (submeshList[i].cellMap()[j] == points[i].first())
            {
                localPoints[i].first() = j;
                break;
            }
        }

        for (int j = 0; j < submeshList[i].cellMap().size(); j++)
        {
            if (submeshList[i].cellMap()[j] == points[i].second())
            {
                localPoints[i].second() = j;
                break;
            }
        }
    }

    return localPoints;
}

template<typename T>
void DEIM<T>::check3D_indices(int& ind_rowA, int&  ind_colA, int& xyz_rowA,
                              int& xyz_colA)
{
    if (ind_rowA < sizeM)
    {
        xyz_rowA = 0;
    }
    else if (ind_rowA < sizeM * 2)
    {
        xyz_rowA = 1;
        ind_rowA = ind_rowA - sizeM;
    }
    else
    {
        xyz_rowA = 2;
        ind_rowA = ind_rowA - 2 * sizeM;
    }

    if (ind_colA < sizeM )
    {
        xyz_colA = 0;
    }
    else if (ind_colA < sizeM * 2)
    {
        xyz_colA = 1;
        ind_colA = ind_colA - 2 * sizeM;
    }
    else
    {
        xyz_colA = 2;
        ind_colA = ind_colA - 2 * sizeM;
    }
};

template<typename T>
void DEIM<T>::check3D_indices(int& ind_rowA, int& xyz_rowA)
{
    if (ind_rowA < sizeM)
    {
        xyz_rowA = 0;
    }
    else if (ind_rowA < sizeM * 2)
    {
        xyz_rowA = 1;
        ind_rowA = ind_rowA - sizeM;
    }
    else
    {
        xyz_rowA = 2;
        ind_rowA = ind_rowA - 2 * sizeM;
    }
};

