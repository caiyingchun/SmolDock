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

#ifndef SMOLDOCK_BASICSCORINGFUNCTION_H
#define SMOLDOCK_BASICSCORINGFUNCTION_H


#include <Engines/Internals/iConformer.h>
#include <Engines/Internals/iProtein.h>
#include <Engines/Internals/iTransform.h>
#include "ScoringFunctionInterface.h"

#include <Structures/Molecule.h>
#include <Structures/Protein.h>

namespace SmolDock {

    /*! \namespace SmolDock::Score Free-standing docking score functions */
    namespace Score {

        const constexpr unsigned int VinaLikeRigid_numCoefficients = 5;

        //! \fn Score a protein-ligand configuration. Variant : Vina-like, rigid, inter-only
        /*!
         * Compute inter-molecular part of the docking score, 
         * according to the Vina scoring function. The transform provided will
         * be applied to the entire, rigid ligand.
         *
         * Nb : can't be templatized to support non-default coeffs because it is used out of this file
         * But it would be possible to make a "normal" overload.
         *
         * \param conformer Ligand conformation & position to evaluate
         * \param transform Transformation to apply to the ligand
         * \param protein Protein conformation to evaluate
         * \return The docking score
         * \sa
        */
        double vina_like_rigid_inter_scoring_func(const iConformer &conformer, iTransform &transform,
                                                  const iProtein &protein);

        class VinaLikeRigid : public ScoringFunction {
        public:


            VinaLikeRigid(const iConformer &startingConformation_,
                                         const iProtein &p,
                                         const iTransform &initialTransform_,
                                         double differential_epsilon_ = 1e-3);


            double Evaluate(const arma::mat &x) final;
            double EvaluateWithGradient(const arma::mat &x, arma::mat &gradient) final;

            std::vector<std::tuple<std::string,double>> EvaluateSubcomponents(const arma::mat &x) final;
            double EvaluateOnlyIntermolecular(const arma::mat &x) final;

            arma::mat getStartingConditions() const final;


            double getDifferentialEpsilon() const final;


            iConformer getConformerForParamMatrix(const arma::mat &x) final;

            unsigned int getParamVectorDimension() const final;


            ~VinaLikeRigid() final = default;


            unsigned int getCoefficientsVectorWidth() final;
            std::vector<std::string> getCoefficientsNames() final;
            std::vector<double> getCurrentCoefficients() final;
            bool setNonDefaultCoefficients(std::vector<double> coeffs) final;

        private:

            static const constexpr unsigned int numCoefficients = VinaLikeRigid_numCoefficients;
            static const std::array<std::string, numCoefficients> coefficientsNames;

            std::array<double, numCoefficients> nonDefaultCoefficients;

            inline iTransform internalToExternalRepr(const arma::mat &x_) const {
                BOOST_ASSERT(x_.n_rows == 7);

                iTransform tr_;

                tr_.transl.x() = x_[0];
                tr_.transl.y() = x_[1];
                tr_.transl.z() = x_[2];

                tr_.rota.w() = x_[3];
                tr_.rota.x() = x_[4];
                tr_.rota.y() = x_[5];
                tr_.rota.z() = x_[6];

                return tr_;
            }

            inline arma::mat externalToInternalRepr(const iTransform &tr_) const {
                arma::mat ret(7, 1);

                ret[0] = tr_.transl.x();
                ret[1] = tr_.transl.y();
                ret[2] = tr_.transl.z();

                ret[3] = tr_.rota.w();
                ret[4] = tr_.rota.x();
                ret[5] = tr_.rota.y();
                ret[6] = tr_.rota.z();

                return ret;
            }

            iConformer startingConformation;
            const iProtein &prot;
            const iTransform initialTransform;
            double differential_epsilon;


        };


    }

}


#endif //SMOLDOCK_BASICSCORINGFUNCTION_H
