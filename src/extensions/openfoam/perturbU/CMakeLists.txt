
set(SRC perturbU.C)

set(OF_INCLUDE_DIRS
#  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
#  turbulenceModels/incompressible/LES turbulenceModels/LES/LESfilters
#  meshTools sampling surfMesh
)

set(OF_LIBS 
#finiteVolume LESfilters sampling
)

set(INCLUDE_DIRS 
  ${insight_INCLUDE_DIR}
)

setup_exe_target_OF(perturbU "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "uniof" "")
