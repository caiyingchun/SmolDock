cmake_minimum_required(VERSION 3.12)

################ BUILD CONFIGURATION SECTION     ############################

# Point this to the rdkit install directory (See Readme)
SET(RDKIT_ROOT /home/eliane/Projects/rdkit_releases/rdkit-Release_2019_09_1/rdkit_install/ CACHE PATH "RDKit install directory"  )

# Command to run : mpic++ -showme:compile. Only add the argument part of the command
#set(MPI_COMPILE_FLAGS -I/home/sdv/m2isdd/ebriand/local/include )

#Command to run  : mpic++  -showme:link
#set(MPI_LINK_FLAGS  " -v")

# Version of python installed with header and libraries
set(PY_VERSION 3.6)

################ END BUILD CONFIGURATION SECTION ############################



set(CMAKE_CXX_STANDARD 17)

# where is the target environment
#SET(CMAKE_FIND_ROOT_PATH  /home/sdv/m2isdd/ebriand/local/)
#SET(CMAKE_PREFIX_PATH /home/sdv/m2isdd/ebriand/local/)

SET(CMAKE_CXX_FLAGS "-Wall")
#SET(CMAKE_CXX_FLAGS "-Wall -fsanitize=adress -fsanitize=undefined")
#SET(CMAKE_CXX_FLAGS "-Wall -fsanitize=memory")
SET(CMAKE_CXX_FLAGS_DEBUG "-g ")
SET(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -march=native -g  ")

set(default_build_type "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
            STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            "Debug" "Release" "RelWithDebInfo")
endif()


project(smoldock)


option(COVERAGE_BUILD "Create a GCov-enabled build." OFF)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

if(COVERAGE_BUILD)
    message("Enabling code coverage ...")
    if(CMAKE_COMPILER_IS_GNUCXX)
        include(CodeCoverage)
        APPEND_COVERAGE_COMPILER_FLAGS()
        set(COVERAGE_LCOV_EXCLUDES '/usr/*' '*/UnitTests/*' '*/Dependencies/*' '*/include/rdkit/*' '*/Frontends/*')
        SETUP_TARGET_FOR_COVERAGE_LCOV(
                NAME ${PROJECT_NAME}_coverage                    # New target name
                EXECUTABLE complete_test_suite # Executable in PROJECT_BINARY_DIR
                DEPENDENCIES complete_test_suite                     # Dependencies to build first
        )
    endif()
endif()


option(VC_NO_FOR_EACH_N "Disable std::for_each_n in Vc for old libstdc++" OFF)
if(VC_NO_FOR_EACH_N)
    add_compile_definitions(Vc_GCC)
endif()



###########################################################################
######################## BUILD SETUP SECTION ##############################
###########################################################################

include(ExternalProject)
ExternalProject_Add(PDBPC
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/PDBPC
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/Dependencies/PDBPC
        INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/PDBPC_install
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        INSTALL_COMMAND ""
        )
set(PDBPC_LIB_PATH  ${CMAKE_CURRENT_BINARY_DIR}/PDBPC/src/PDBPC-build/libpdbpc_static_pic.a)
## MPI #######################
# Edit this if you have a specific MPI setup
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

## END MPI ###################

## Boost #######################
# Edit this if you have a specific Boost setup
find_package(Boost REQUIRED log log_setup regex system serialization iostreams)
include_directories(${Boost_INCLUDE_DIRS})

#add_compile_definitions(BOOST_ALLOW_DEPRECATED_HEADERS)
## END Boost ###################

## Python #######################
# Edit version and other setup if needed

find_package(PythonLibs ${PY_VERSION} REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})
## END Python ###################

######## VC SETUP ######
# Adjust this according to where your Vc build is :
#set(VC_ROOT /usr/)
#
#include_directories(${VC_ROOT}/include/)
#LINK_DIRECTORIES(${VC_ROOT}/lib/)
######## END OF VC  ######

######## EIGEN SETUP ######
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
include_directories(${EIGEN3_INCLUDE_DIR})
#set(EIGEN_ROOT /usr/)
#include_directories(${EIGEN_ROOT}/include)
######## END OF EIGEN  ######

######## RDKIT SETUP ######
# Adjust this according to where your RDKit build is :
#set(RDKIT_ROOT /home/eliane/Projects/RDKit_Git/rdkit-Release_2018_09_1)

# This directory should contain ./Code ./lib and ./lib/staticlib as setup in the build guide
include(${RDKIT_ROOT}/lib/cmake/rdkit/rdkit-config.cmake)
include_directories(${RDKIT_ROOT}/include/)
include_directories(${RDKIT_ROOT}/include/rdkit)
LINK_DIRECTORIES(${RDKIT_ROOT}/lib)
######## END OF RDKIT ######



######## TBB SETUP ######
find_package( Threads )
#add_subdirectory(Dependencies/tbb)
#include_directories(Dependencies/tbb/include)
######## END OF TBB ######



# Comment out the following for quieter runs (generally speaking, do not affect runtime either way)
add_compile_definitions(SMOLDOCK_VERBOSE_DEBUG)

# For Intel VTune profiler tools, set this to true (necessary if building a static executable that can be profiled)
set(SMOLDOCK_INTEL_VTUNE_EXPORT_SYMBOL FALSE)

###########################################################################
######################## END  BUILD SETUP SECTION #########################
###########################################################################

include_directories(Dependencies)
include_directories(Dependencies/ensmallen/include/)
include_directories(.)



set(SOURCE_LIST Structures/Structure.cpp
        Structures/Molecule.cpp
        Structures/Protein.cpp
        Structures/Atom.cpp
        Structures/Bond.cpp
        Structures/AminoAcid.cpp
        Engines/MDStyleDockingEngine.cpp
        Engines/VinaCompatibleDockingEngine.cpp
        Structures/Results/DockingResult.cpp
        Utilities/DockingResultPrinter.cpp
        Engines/ConformerDockingEngine.cpp
        Engines/ScoringFunctions/VinaLikeRigid.cpp
        Utilities/PDBWriter.cpp
        Utilities/PDBLigandUtils.cpp
        Engines/LocalOptimizers/GradientDescentLineSearch.cpp
        Utilities/IntermediateConformerCollector.cpp
        Utilities/ReScorer.cpp
        Structures/Common/ResiduePropertiesAssignation.cpp
        Structures/Common/PDBResiduePropertiesTable.cpp
        Structures/InputModifiers/VinaCompatibility.cpp
        Engines/LocalOptimizers/L_BFGS.cpp
        Engines/GlobalHeuristics/RandomRestart.cpp
        Engines/ScoringFunctions/ScoringFunctionFactory.cpp
        Engines/LocalOptimizers/OptimizerFactory.cpp
        Engines/GlobalHeuristics/HeuristicFactory.cpp
        Engines/PoseRefiner.cpp
        Engines/GlobalHeuristics/OnlyLocal.cpp
        Engines/GlobalHeuristics/IteratedLocalSearch.cpp
        Engines/GlobalHeuristics/SimulatedAnnealing.cpp
        Engines/GlobalHeuristics/Utilities/MetropolisHastings.cpp
        Utilities/Version.cpp
        Engines/ScoringFunctions/VinaLike.cpp
        Engines/GlobalHeuristics/DifferentialEvolution.cpp
        Engines/GlobalHeuristics/Evolution.cpp
        Engines/ScoringFunctions/VinaLikeCovalentReversible.cpp
        Structures/InputModifiers/RotatableBondRemover.cpp
        Engines/ScoringFunctions/VinaLikeCovalentReversible.cpp
        Utilities/Calibration/Calibrator.cpp
        Utilities/SMARTSMatcher.cpp
        Utilities/AdvancedErrorHandling.cpp
        Utilities/CSVReader.cpp
        Frontends/CLI/FileInputOutput.cpp
        Engines/Utils/ExtractProteinFromBox.cpp
        )

set(MPI_SOURCE_LIST Utilities/Calibration/MPICalibratorDirector.cpp
        Utilities/Calibration/MPICalibratorNode.cpp
        )

set(PY_EXPORT_SOURCE_LIST Frontends/Python/PyStructures.cpp
        Frontends/Python/PySTLWrapper.cpp
        Frontends/Python/PyUtilities.cpp
        Frontends/Python/PyEngine.cpp
        )




set(BPRINTER_SOURCE_LIST
        Dependencies/bprinter/table_printer.h
        Dependencies/bprinter/impl/table_printer.tpp.h
        Dependencies/bprinter/table_printer.cpp
        )

add_compile_definitions(BOOST_ENABLE_ASSERT_HANDLER)


ADD_LIBRARY(libsmoldock SHARED ${SOURCE_LIST} ${BPRINTER_SOURCE_LIST})
target_compile_definitions(libsmoldock PUBLIC ARMA_DONT_USE_WRAPPER)
set_target_properties(libsmoldock PROPERTIES PREFIX "")
add_dependencies(libsmoldock rdkit_base)
target_compile_definitions(libsmoldock PUBLIC BOOST_LOG_DYN_LINK)
add_dependencies(libsmoldock PDBPC)
target_link_libraries(libsmoldock ${PDBPC_LIB_PATH})


add_executable(smoldock Frontends/CLI_frontend.cpp)
target_link_libraries(smoldock libsmoldock)
target_link_libraries(smoldock tbb)
target_link_libraries(smoldock rdkit_base avalon_clib AvalonLib freesasa_clib FreeSASALib maeparser )
target_link_libraries(smoldock coordgen RingDecomposerLib URFLib RDGeneral RDStreams DataStructs )
target_link_libraries(smoldock RDGeometryLib Alignment EigenSolvers Optimizer ForceField DistGeometry)
target_link_libraries(smoldock Catalogs GraphMol Depictor SmilesParse FileParsers SubstructMatch ChemReactions ChemTransforms Subgraphs)
target_link_libraries(smoldock FilterCatalog FragCatalog Descriptors Fingerprints PartialCharges MolTransforms ForceFieldHelpers)
target_link_libraries(smoldock DistGeomHelpers MolAlign MolChemicalFeatures ShapeHelpers MolCatalog MolDraw2D FMCS MolHash MMPA)
target_link_libraries(smoldock StructChecker ReducedGraphs Trajectory SubstructLibrary RGroupDecomposition MolInterchange SLNParse)
target_link_libraries(smoldock MolStandardize SimDivPickers hc InfoTheory ChemicalFeatures ConformerParser )
target_link_libraries(smoldock boost_log_setup boost_log boost_thread boost_filesystem boost_program_options boost_iostreams)
target_link_libraries(smoldock unwind)
target_link_libraries(smoldock Vc)
target_link_libraries(smoldock lapack blas)
target_link_libraries(smoldock z zstd zip lzma bz2 pthread)


if(SMOLDOCK_INTEL_VTUNE_EXPORT_SYMBOL)
    target_link_options(smoldock PRIVATE "-u_init" "-usetenv" "-ugetenv" "-u__errno_location" "-upthread_create" "-upthread_getattr_np")
    target_link_options(smoldock PRIVATE "-upthread_self" "-upthread_getspecific" "-upthread_setspecific")
    target_link_options(smoldock PRIVATE "-upthread_getattr_np" "-upthread_attr_destroy" "-upthread_attr_setstack" "-upthread_attr_getstack")
    target_link_options(smoldock PRIVATE "-upthread_attr_getstacksize" "-upthread_attr_setstacksize" "-upthread_cancel" "-u_pthread_cleanup_push")
    target_link_options(smoldock PRIVATE "-u_pthread_cleanup_pop" "-upthread_attr_setstacksize" "-upthread_cancel" "-u_pthread_cleanup_push")
    target_link_options(smoldock PRIVATE "-upthread_mutex_lock" "-upthread_mutex_trylock" "-upthread_spin_lock" "-upthread_spin_trylock")
endif()


add_executable(goldenRecordGenerator Frontends/goldenRecordGenerator.cpp)
target_link_libraries(goldenRecordGenerator libsmoldock)
target_link_libraries(goldenRecordGenerator rdkit_base avalon_clib AvalonLib freesasa_clib FreeSASALib maeparser )
target_link_libraries(goldenRecordGenerator coordgen RingDecomposerLib URFLib RDGeneral RDStreams DataStructs )
target_link_libraries(goldenRecordGenerator RDGeometryLib Alignment EigenSolvers Optimizer ForceField DistGeometry)
target_link_libraries(goldenRecordGenerator Catalogs GraphMol Depictor SmilesParse FileParsers SubstructMatch ChemReactions ChemTransforms Subgraphs)
target_link_libraries(goldenRecordGenerator FilterCatalog FragCatalog Descriptors Fingerprints PartialCharges MolTransforms ForceFieldHelpers)
target_link_libraries(goldenRecordGenerator DistGeomHelpers MolAlign MolChemicalFeatures ShapeHelpers MolCatalog MolDraw2D FMCS MolHash MMPA)
target_link_libraries(goldenRecordGenerator StructChecker ReducedGraphs Trajectory SubstructLibrary RGroupDecomposition MolInterchange SLNParse)
target_link_libraries(goldenRecordGenerator MolStandardize SimDivPickers hc InfoTheory ChemicalFeatures ConformerParser)
target_link_libraries(goldenRecordGenerator boost_log_setup boost_log boost_thread boost_filesystem boost_program_options boost_iostreams)
target_link_libraries(goldenRecordGenerator tbb)
target_link_libraries(goldenRecordGenerator unwind)
target_link_libraries(goldenRecordGenerator Vc)
target_link_libraries(goldenRecordGenerator lapack blas)
target_link_libraries(goldenRecordGenerator z zip lzma bz2 pthread)


add_executable(mpicalibrator Frontends/mpicalibrator.cpp ${MPI_SOURCE_LIST})
target_link_libraries(mpicalibrator libsmoldock)
target_link_libraries(mpicalibrator tbb)
target_link_libraries(mpicalibrator rdkit_base avalon_clib AvalonLib freesasa_clib FreeSASALib maeparser )
target_link_libraries(mpicalibrator coordgen RingDecomposerLib URFLib RDGeneral RDStreams DataStructs )
target_link_libraries(mpicalibrator RDGeometryLib Alignment EigenSolvers Optimizer ForceField DistGeometry)
target_link_libraries(mpicalibrator Catalogs GraphMol Depictor SmilesParse FileParsers SubstructMatch ChemReactions ChemTransforms Subgraphs)
target_link_libraries(mpicalibrator FilterCatalog FragCatalog Descriptors Fingerprints PartialCharges MolTransforms ForceFieldHelpers)
target_link_libraries(mpicalibrator DistGeomHelpers MolAlign MolChemicalFeatures ShapeHelpers MolCatalog MolDraw2D FMCS MolHash MMPA)
target_link_libraries(mpicalibrator StructChecker ReducedGraphs Trajectory SubstructLibrary RGroupDecomposition MolInterchange SLNParse)
target_link_libraries(mpicalibrator MolStandardize SimDivPickers hc InfoTheory ChemicalFeatures ConformerParser)
target_link_libraries(mpicalibrator boost_log_setup boost_log boost_thread boost_filesystem boost_program_options boost_iostreams)
target_link_libraries(mpicalibrator unwind)
target_link_libraries(mpicalibrator Vc)
target_link_libraries(mpicalibrator lapack blas)
target_link_libraries(mpicalibrator boost_mpi mpi_cxx mpi)
target_link_libraries(mpicalibrator z zip lzma bz2 pthread)




add_library(PySmolDock MODULE Frontends/SmoldockPythonBindings.cpp ${PY_EXPORT_SOURCE_LIST})
set_target_properties(PySmolDock PROPERTIES COMPILE_FLAGS " -fPIC ")
set_target_properties(PySmolDock PROPERTIES LINK_FLAGS " -shared ")
set_target_properties(PySmolDock PROPERTIES PREFIX "")
set_target_properties(PySmolDock PROPERTIES OUTPUT_NAME "PySmolDock")
target_compile_definitions(PySmolDock PUBLIC BOOST_LOG_DYN_LINK)
target_compile_definitions(PySmolDock PUBLIC ARMA_DONT_USE_WRAPPER)
target_link_libraries(PySmolDock libsmoldock)

target_link_libraries(PySmolDock tbb)
target_link_libraries(PySmolDock rdkit_base avalon_clib AvalonLib freesasa_clib FreeSASALib maeparser )
target_link_libraries(PySmolDock coordgen RingDecomposerLib URFLib RDGeneral RDStreams DataStructs )
target_link_libraries(PySmolDock RDGeometryLib Alignment EigenSolvers Optimizer ForceField DistGeometry)
target_link_libraries(PySmolDock Catalogs GraphMol Depictor SmilesParse FileParsers SubstructMatch ChemReactions ChemTransforms Subgraphs)
target_link_libraries(PySmolDock FilterCatalog FragCatalog Descriptors Fingerprints PartialCharges MolTransforms ForceFieldHelpers)
target_link_libraries(PySmolDock DistGeomHelpers MolAlign MolChemicalFeatures ShapeHelpers MolCatalog MolDraw2D FMCS MolHash MMPA)
target_link_libraries(PySmolDock StructChecker ReducedGraphs Trajectory SubstructLibrary RGroupDecomposition MolInterchange SLNParse)
target_link_libraries(PySmolDock MolStandardize SimDivPickers hc InfoTheory ChemicalFeatures ConformerParser)
target_link_libraries(PySmolDock boost_log_setup boost_log boost_thread boost_filesystem boost_program_options boost_iostreams)
target_link_libraries(PySmolDock unwind)
target_link_libraries(PySmolDock Vc)
target_link_libraries(PySmolDock z zstd zip lzma bz2 lapack blas)
target_link_libraries(PySmolDock pthread)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(libsmoldock PUBLIC SMOLDOCK_VERBOSE_DEBUG)
    target_compile_definitions(smoldock PUBLIC SMOLDOCK_VERBOSE_DEBUG)
    target_compile_definitions(PySmolDock PUBLIC SMOLDOCK_VERBOSE_DEBUG)
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)




###############################################################################################################
############                                  UNIT TESTING
###############################################################################################################
enable_testing()

set(TEST_SOURCE_LIST
        UnitTests/main_test_suite.cpp
        UnitTests/engine_test_suite.cpp
        UnitTests/Engines/DockingBoxUtils_test_suite.cpp
        UnitTests/Fixtures/TripeptideFixture.cpp
        )


add_executable(complete_test_suite ${TEST_SOURCE_LIST})

set_target_properties(complete_test_suite PROPERTIES LINK_FLAGS "   ")
target_link_libraries(complete_test_suite libsmoldock)

target_link_libraries(complete_test_suite tbb)
target_link_libraries(complete_test_suite rdkit_base avalon_clib AvalonLib freesasa_clib FreeSASALib maeparser )
target_link_libraries(complete_test_suite coordgen RingDecomposerLib URFLib RDGeneral RDStreams DataStructs )
target_link_libraries(complete_test_suite RDGeometryLib Alignment EigenSolvers Optimizer ForceField DistGeometry)
target_link_libraries(complete_test_suite Catalogs GraphMol Depictor SmilesParse FileParsers SubstructMatch ChemReactions ChemTransforms Subgraphs)
target_link_libraries(complete_test_suite FilterCatalog FragCatalog Descriptors Fingerprints PartialCharges MolTransforms ForceFieldHelpers)
target_link_libraries(complete_test_suite DistGeomHelpers MolAlign MolChemicalFeatures ShapeHelpers MolCatalog MolDraw2D FMCS MolHash MMPA)
target_link_libraries(complete_test_suite StructChecker ReducedGraphs Trajectory SubstructLibrary RGroupDecomposition MolInterchange SLNParse)
target_link_libraries(complete_test_suite MolStandardize SimDivPickers hc InfoTheory ChemicalFeatures ConformerParser)
target_link_libraries(complete_test_suite boost_log_setup boost_log boost_thread boost_filesystem boost_program_options boost_iostreams)
target_link_libraries(complete_test_suite unwind)
target_link_libraries(complete_test_suite Vc)
target_link_libraries(complete_test_suite lapack blas)
target_link_libraries(complete_test_suite z zip zstd lzma bz2 pthread)
target_link_libraries(complete_test_suite ${Boost_LIBRARIES} boost_unit_test_framework )
#target_link_libraries(complete_test_suite Vc)


add_test(smoldock_unittest complete_test_suite --catch_system_error=yes --log_level=all )
