#include "MyElectricFieldMap.hh"

#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "G4Exception.hh"

#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>

// ============================================================
// 构造函数
// ============================================================

MyElectricFieldMap::MyElectricFieldMap(
    const std::string& filename,
    G4double lenUnit,
    G4double fieldUnit)
    : fNx(0),
      fNy(0),
      fNz(0),
      fMinX(0.0),
      fMaxX(0.0),
      fDx(0.0),
      fMinY(0.0),
      fMaxY(0.0),
      fDy(0.0),
      fMinZ(0.0),
      fMaxZ(0.0),
      fDz(0.0),
      fLenUnit(lenUnit),
      fFieldUnit(fieldUnit)
{
    LoadFieldMap(filename);
}

// ============================================================
// 析构函数
// ============================================================

MyElectricFieldMap::~MyElectricFieldMap()
{
}

// ============================================================
// 读取 COMSOL 电场文件
// ============================================================

void MyElectricFieldMap::LoadFieldMap(const std::string& filename)
{
    std::ifstream infile(filename);

    if (!infile.is_open())
    {
        G4Exception(
            "MyElectricFieldMap::LoadFieldMap",
            "FieldMapFileError",
            FatalException,
            ("Cannot open electric field map file: " + filename).c_str()
        );
        return;
    }

    G4cout << "============================================" << G4endl;
    G4cout << "Loading electric field map:" << G4endl;
    G4cout << filename << G4endl;

    struct FieldPoint
    {
        G4double x;
        G4double y;
        G4double z;
        G4double Ex;
        G4double Ey;
        G4double Ez;
    };

    std::vector<FieldPoint> points;
    std::vector<G4double> xValues;
    std::vector<G4double> yValues;
    std::vector<G4double> zValues;

    std::string line;

    while (std::getline(infile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::stringstream ss(line);
        FieldPoint p;

        if (!(ss >> p.x >> p.y >> p.z >> p.Ex >> p.Ey >> p.Ez))
        {
            continue;
        }

        p.x *= fLenUnit;
        p.y *= fLenUnit;
        p.z *= fLenUnit;

        p.Ex *= fFieldUnit;
        p.Ey *= fFieldUnit;
        p.Ez *= fFieldUnit;

        points.push_back(p);
        xValues.push_back(p.x);
        yValues.push_back(p.y);
        zValues.push_back(p.z);
    }

    infile.close();

    if (points.empty())
    {
        G4Exception(
            "MyElectricFieldMap::LoadFieldMap",
            "FieldMapEmpty",
            FatalException,
            "Electric field map contains no valid data."
        );
        return;
    }

    std::sort(xValues.begin(), xValues.end());
    std::sort(yValues.begin(), yValues.end());
    std::sort(zValues.begin(), zValues.end());

    const G4double tolerance = 1.0e-12;

    auto uniqueValues = [tolerance](std::vector<G4double>& values)
    {
        std::vector<G4double> result;
        for (auto value : values)
        {
            if (result.empty())
            {
                result.push_back(value);
            }
            else
            {
                G4double scale = std::max(std::fabs(value), std::fabs(result.back()));
                if (std::fabs(value - result.back()) > tolerance * std::max(1.0, scale))
                {
                    result.push_back(value);
                }
            }
        }
        values = result;
    };

    uniqueValues(xValues);
    uniqueValues(yValues);
    uniqueValues(zValues);

    fNx = static_cast<int>(xValues.size());
    fNy = static_cast<int>(yValues.size());
    fNz = static_cast<int>(zValues.size());

    if (fNx < 2 || fNy < 2 || fNz < 2)
    {
        G4Exception(
            "MyElectricFieldMap::LoadFieldMap",
            "FieldMapGridError",
            FatalException,
            "Electric field map must contain at least 2 points in x, y and z."
        );
        return;
    }

    fMinX = xValues.front();
    fMaxX = xValues.back();
    fMinY = yValues.front();
    fMaxY = yValues.back();
    fMinZ = zValues.front();
    fMaxZ = zValues.back();

    fDx = xValues[1] - xValues[0];
    fDy = yValues[1] - yValues[0];
    fDz = zValues[1] - zValues[0];

    const G4double gridTolerance = 1.0e-8;

    for (int i = 1; i < fNx - 1; ++i)
    {
        G4double dx = xValues[i + 1] - xValues[i];
        if (std::fabs(dx - fDx) > gridTolerance * std::max(1.0, std::fabs(fDx)))
        {
            G4Exception("MyElectricFieldMap::LoadFieldMap", "NonUniformGrid", FatalException, "X grid is not uniform.");
            return;
        }
    }

    for (int i = 1; i < fNy - 1; ++i)
    {
        G4double dy = yValues[i + 1] - yValues[i];
        if (std::fabs(dy - fDy) > gridTolerance * std::max(1.0, std::fabs(fDy)))
        {
            G4Exception("MyElectricFieldMap::LoadFieldMap", "NonUniformGrid", FatalException, "Y grid is not uniform.");
            return;
        }
    }

    for (int i = 1; i < fNz - 1; ++i)
    {
        G4double dz = zValues[i + 1] - zValues[i];
        if (std::fabs(dz - fDz) > gridTolerance * std::max(1.0, std::fabs(fDz)))
        {
            G4Exception("MyElectricFieldMap::LoadFieldMap", "NonUniformGrid", FatalException, "Z grid is not uniform.");
            return;
        }
    }

    const std::size_t totalSize = static_cast<std::size_t>(fNx) * static_cast<std::size_t>(fNy) * static_cast<std::size_t>(fNz);

    fExMap.assign(totalSize, 0.0);
    fEyMap.assign(totalSize, 0.0);
    fEzMap.assign(totalSize, 0.0);

    auto findIndex = [](const std::vector<G4double>& values, G4double value) -> int
    {
        auto it = std::lower_bound(values.begin(), values.end(), value);
        if (it == values.end()) return -1;
        return static_cast<int>(std::distance(values.begin(), it));
    };

    for (const auto& p : points)
    {
        int ix = findIndex(xValues, p.x);
        int iy = findIndex(yValues, p.y);
        int iz = findIndex(zValues, p.z);

        if (ix < 0 || iy < 0 || iz < 0)
            continue;

        std::size_t index = (static_cast<std::size_t>(ix) * static_cast<std::size_t>(fNy) + static_cast<std::size_t>(iy)) * static_cast<std::size_t>(fNz) + static_cast<std::size_t>(iz);

        fExMap[index] = p.Ex;
        fEyMap[index] = p.Ey;
        fEzMap[index] = p.Ez;
    }

    G4cout << "Electric field map loaded successfully." << G4endl;
    G4cout << "Number of data points = " << points.size() << G4endl;
    G4cout << "Grid size = " << fNx << " x " << fNy << " x " << fNz << G4endl;
    G4cout << "X range = " << fMinX / mm << " mm  ->  " << fMaxX / mm << " mm" << G4endl;
    G4cout << "Y range = " << fMinY / mm << " mm  ->  " << fMaxY / mm << " mm" << G4endl;
    G4cout << "Z range = " << fMinZ / mm << " mm  ->  " << fMaxZ / mm << " mm" << G4endl;
    G4cout << "Grid spacing = " << fDx / mm << " mm, " << fDy / mm << " mm, " << fDz / mm << " mm" << G4endl;
    G4cout << "============================================" << G4endl;
}

// ============================================================
// 三线性插值
// ============================================================

void MyElectricFieldMap::GetFieldValue(
    const G4double Point[4],
    G4double* field) const
{
    field[0] = 0.0;
    field[1] = 0.0;
    field[2] = 0.0;

    field[3] = 0.0;
    field[4] = 0.0;
    field[5] = 0.0;

    const G4double x = Point[0];
    const G4double y = Point[1];
    const G4double z = Point[2];

    if (x < fMinX || x > fMaxX ||
        y < fMinY || y > fMaxY ||
        z < fMinZ || z > fMaxZ)
    {
        return;
    }

    G4double fx = (x - fMinX) / fDx;
    G4double fy = (y - fMinY) / fDy;
    G4double fz = (z - fMinZ) / fDz;

    int ix = static_cast<int>(std::floor(fx));
    int iy = static_cast<int>(std::floor(fy));
    int iz = static_cast<int>(std::floor(fz));

    if (ix >= fNx - 1) ix = fNx - 2;
    if (iy >= fNy - 1) iy = fNy - 2;
    if (iz >= fNz - 1) iz = fNz - 2;

    if (ix < 0 || iy < 0 || iz < 0) return;

    G4double tx = (x - (fMinX + ix * fDx)) / fDx;
    G4double ty = (y - (fMinY + iy * fDy)) / fDy;
    G4double tz = (z - (fMinZ + iz * fDz)) / fDz;

    auto index = [this](int i, int j, int k) -> std::size_t
    {
        return (static_cast<std::size_t>(i) * static_cast<std::size_t>(fNy) + static_cast<std::size_t>(j)) * static_cast<std::size_t>(fNz) + static_cast<std::size_t>(k);
    };

    const std::size_t i000 = index(ix,     iy,     iz);
    const std::size_t i100 = index(ix + 1, iy,     iz);
    const std::size_t i010 = index(ix,     iy + 1, iz);
    const std::size_t i110 = index(ix + 1, iy + 1, iz);
    const std::size_t i001 = index(ix,     iy,     iz + 1);
    const std::size_t i101 = index(ix + 1, iy,     iz + 1);
    const std::size_t i011 = index(ix,     iy + 1, iz + 1);
    const std::size_t i111 = index(ix + 1, iy + 1, iz + 1);

    auto interpolate = [tx, ty, tz](
        G4double c000, G4double c100, G4double c010, G4double c110,
        G4double c001, G4double c101, G4double c011, G4double c111)
    {
        G4double c00 = c000 * (1.0 - tx) + c100 * tx;
        G4double c10 = c010 * (1.0 - tx) + c110 * tx;
        G4double c01 = c001 * (1.0 - tx) + c101 * tx;
        G4double c11 = c011 * (1.0 - tx) + c111 * tx;

        G4double c0 = c00 * (1.0 - ty) + c10 * ty;
        G4double c1 = c01 * (1.0 - ty) + c11 * ty;

        return c0 * (1.0 - tz) + c1 * tz;
    };

    field[3] = interpolate(fExMap[i000], fExMap[i100], fExMap[i010], fExMap[i110], fExMap[i001], fExMap[i101], fExMap[i011], fExMap[i111]);
    field[4] = interpolate(fEyMap[i000], fEyMap[i100], fEyMap[i010], fEyMap[i110], fEyMap[i001], fEyMap[i101], fEyMap[i011], fEyMap[i111]);
    field[5] = interpolate(fEzMap[i000], fEzMap[i100], fEzMap[i010], fEzMap[i110], fEzMap[i001], fEzMap[i101], fEzMap[i011], fEzMap[i111]);
}