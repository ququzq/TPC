# AGENTS.md

This directory started as the stock Geant4 `basic/B1` example but has been heavily
customized into a liquid-argon (LAr) TPC simulation: analytic PMMA/LAr/Clevios/Cu
geometry, a COMSOL 3D electric field, scintillation + optical photons, and photon
spectrum output. `README.md` and `History` still describe the unmodified tutorial —
do not trust them for current behavior; read `src/` instead.

## Build & run

Geant4 is installed at `/home/ququz/geant4/install` (found via `find_package`).
`build/` is already configured (`CMAKE_BUILD_TYPE=Debug`).

```
cmake --build build -j16                 # incremental build
cmake -S . -B build                      # reconfigure only if CMakeLists changes
./exampleB1 run1.mac                     # batch, run from build/ (see below)
./exampleB1                              # interactive + vis.mac via init_vis.mac
```

`src/*.cc` and `include/*.hh` are globbed by CMake. Adding a new source file requires
re-running `cmake -S . -B build` before `cmake --build`; a plain rebuild will not pick
it up.

**Always run the executable from `build/`.** `DetectorConstruction` hardcodes the
relative path `../file/geant4_3d_field.txt`, and macros/outputs are resolved against
the CWD. Running from the source dir fails to find the field map.

## Data / analysis workflow

- Input field map: `file/geant4_3d_field.txt` (~140 MB COMSOL export, cols x y z Ex Ey Ez).
  `file/*.stl` are CAD assets; geometry currently uses `G4Tubs`, not CADMesh.
- Sim writes `photons_in_LAr.txt` (photon ID + energy in eV) to the CWD (i.e. `build/`).
- `draw.py` reads `photons_in_LAr.txt`, converts eV -> nm, writes `photon_wavelength.txt`
  and plots. Run it from `build/` (`python3 ../draw.py`). Needs numpy + matplotlib.
- `photons_in_LAr.txt` / `photon_wavelength.txt` are large generated outputs, not sources.

## Code notes

- Namespace `B1`; comments are Chinese.
- `PhysicsList` = `G4EmStandardPhysics_option4` + decay + radioactive decay + `FTFP_BERT`
  + `G4OpticalPhysics` (scintillation/Rayleigh/absorption/boundary), cut 0.1 mm.
- `MyMaterials` sets LAr/GAr/Clevios optical + scintillation properties; `GetMaterial()`
  calls `Construct()` on every invocation. Default gun: 100 keV gamma at (0,0,0.5 m), +z.
- `RunAction` accumulates photons into a static vector guarded by a mutex and writes the
  output only on the master thread; `SteppingAction` records only optical photons whose
  first step starts in a `phys*LAr*` volume and kills tracks that reach `World`.
- No test suite, linter, formatter, or git repo. Verification is "it compiles and a run
  produces sane `photons_in_LAr.txt`".
