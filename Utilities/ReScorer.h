//
// Created by eliane on 26/12/18.
//

#ifndef SMOLDOCK_RESCORER_H
#define SMOLDOCK_RESCORER_H

#include <Structures/Protein.h>
#include <Structures/Molecule.h>
#include <Engines/Internals/iTransform.h>

namespace SmolDock {

    class ReScorer {
        ReScorer(Protein &prot, Molecule &mol, std::function<double(const iConformer &, const iTransform &, const iProtein &)>& scorFunc);

        bool prepare();
        double getScore();
    private:
        Protein &protein;
        Molecule &molecule;
        std::function<double(const iConformer &, const iTransform &, const iProtein &)>& scoringFunction;

        iProtein iprotein;
        iConformer iconformer;
    };

}

#endif //SMOLDOCK_RESCORER_H