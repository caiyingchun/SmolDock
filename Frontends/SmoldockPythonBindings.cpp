/*
 * Copyright (c) 2018 Eliane Briand
 *
 * This file is part of SmolDock.
 *
 * SmolDock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SmolDock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SmolDock.  If not, see <https://www.gnu.org/licenses/>.
 *
 */


#include <boost/python.hpp>
#include <boost/numpy.hpp>


#include "Python/PySTLWrapper.h"
#include "Python/PyStructures.h"
#include "Python/PyUtilities.h"
#include "Python/PyScoringFunctions.h"

namespace p = boost::python;
namespace np = boost::numpy;

BOOST_PYTHON_MODULE(PySmolDock)
{
    Py_Initialize();
    np::initialize();

    export_STLWrapper();
    export_Structures();
    export_Utilities();
    export_ScoringFunctions();


}