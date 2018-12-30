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

#ifndef SMOLDOCK_ICONFORMER_H
#define SMOLDOCK_ICONFORMER_H

#include <vector>
#include <memory>


namespace SmolDock {

    //! Internal, fast representation of a given Molecule conformer for efficiency
    /*!
     * \sa iAtom, iProtein
    */
    struct iConformer {

        unsigned int num_rotatable_bond;

        std::vector<double> x,y,z;
        std::vector<double> atomicRadius;
        std::vector<unsigned char> type;
        std::vector<unsigned int> variant;
    };

}


#endif //SMOLDOCK_ICONFORMER_H
