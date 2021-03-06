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

#include <memory>
#include <chrono>
#include <thread>
#include <algorithm>
#include <stdexcept>

#include "ConformerDockingEngine.h"
#include <Engines/Internals/InternalsUtilityFunctions.h>
#include <Engines/ScoringFunctions/VinaLikeRigid.h>
#include <Engines/LocalOptimizers/L_BFGS.h>
#include <Engines/Utils/ExtractProteinFromBox.hpp>
#include <Utilities/TimingsLog.h>

#undef BOOST_LOG

#include <boost/log/trivial.hpp>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/moment.hpp>


namespace SmolDock {
    namespace Engine {

        ConformerDockingEngine::ConformerDockingEngine(unsigned int conformer_num_,
                                                                 unsigned int retryPerConformer,
                                                                 Protein* protein,
                                                                 Molecule* ligand,
                                                                 Score::ScoringFunctionType scFuncType,
                                                                 Heuristics::GlobalHeuristicType heurType,
                                                                 Optimizer::LocalOptimizerType localOptimizerType_,
                                                                 unsigned int seed,
                                                                 Heuristics::HeuristicParameters hParams,
                                                                 bool rigidDocking) :
                conformer_num(conformer_num_),
                retryPerConformer(retryPerConformer),
                orig_protein(protein),
                orig_ligand(ligand),
                scoringFuncType(scFuncType),
                heuristicType(heurType),
                localOptimizerType(localOptimizerType_),
                rnd_generator(seed),
                heurParams(hParams),
                rigidDocking(rigidDocking){


            scores.reserve(this->conformer_num);
            allGeneratediConformer.reserve(this->conformer_num * this->retryPerConformer);
        }


        bool ConformerDockingEngine::setupDockingEngine() {

            record_timings(begin_setup);

            std::uniform_int_distribution<> distrib_random_int(0, std::numeric_limits<int>::max());
            std::uniform_real_distribution<double> distrib_real_position(-100.0, 100.0);

            record_timings(begin_conformersgen);

            this->orig_ligand->generateConformers(this->viConformers, this->conformer_num, true,
                                                  distrib_random_int(this->rnd_generator));

            record_timings(end_conformersgen);

            this->protein = extractIProteinFromBoxSetting(this->orig_protein,this->dockBoxSettings);
            this->fullProtein = this->orig_protein->getiProtein();


            record_timings(end_iprotgen);


            record_timings(end_setup);


#ifdef SMOLDOCK_VERBOSE_DEBUG
            auto total_setup_duration = static_cast< std::chrono::duration<double> >(end_setup - begin_setup).count();
            auto duration_conformergen = static_cast< std::chrono::duration<double> >(end_conformersgen -
                                                                                      begin_conformersgen).count();
            auto duration_iprot = static_cast< std::chrono::duration<double> >(end_iprotgen -
                                                                               end_conformersgen).count();

            BOOST_LOG_TRIVIAL(debug) << "Timings ConformerDockingEngine setup"
                                     << "\n      TOTAL: "
                                     << total_setup_duration << "s"
                                     << "\n      Conformer: "
                                     << duration_conformergen
                                     << "s [n=" << this->conformer_num << "->" << this->viConformers.size() << "] "
                                     << duration_conformergen / this->conformer_num << "s each"
                                     << "\n      iProt: " << duration_iprot << "s";
#endif
            BOOST_LOG_TRIVIAL(info) << "Conformer docking engine: ready";
            return true;
        }

        void ConformerDockingEngine::runDockingEngine() {

            using namespace boost::accumulators;
            accumulator_set<double, stats<tag::mean, tag::moment<2> > > acc_score;
            accumulator_set<double, stats<tag::mean, tag::moment<2> > > acc_duration;

            std::uniform_int_distribution<unsigned int> distrib_uint(0, std::numeric_limits<unsigned int>::max());


            record_timings(begin_docking);


            for (auto& conformer : this->viConformers) {


                record_timings(begin_docking_this_conformer);



                iTransform starting_pos_tr = this->rigidDocking ? iTransformIdentityInit(0) : iTransformIdentityInit(conformer.num_rotatable_bond);

                if(this->rigidDocking)
                {
                    conformer.num_rotatable_bond = 0;
                }

                starting_pos_tr.transl = conformer.centroidNormalizingTransform;


                DockingBoxSetting dbsettings = this->dockBoxSettings;
                unsigned int seed = distrib_uint(this->rnd_generator);

                arma::arma_rng::set_seed(seed);

                if (dbsettings.shape == DockingBoxSetting::Shape::sphere) {
                    starting_pos_tr.transl.x() += dbsettings.center[0];
                    starting_pos_tr.transl.y() += dbsettings.center[1];
                    starting_pos_tr.transl.z() += dbsettings.center[2];
                }

                starting_pos_tr.rota.normalize();

                std::shared_ptr<Score::ScoringFunction> scoringFunction = scoringFunctionFactory(this->scoringFuncType,
                                                                                                 conformer,
                                                                                                 this->protein,
                                                                                                 starting_pos_tr,
                                                                                                 1e-3);

                std::shared_ptr<Optimizer::Optimizer> localOptimizer = optimizerFactory(this->localOptimizerType,
                                                                                        scoringFunction.get(),
                                                                                        1e-3);

                if(this->heurParams.index() == 0 ) // LackOfParameter
                {
                    BOOST_LOG_TRIVIAL(debug) << "Received default heuristics parameters, setting up search domain if relevant";

                    double proteinMaxRadius = (dbsettings.shape == DockingBoxSetting::Shape::sphere) ? dbsettings.radius : this->orig_protein->getMaxRadius();
                    this->heurParams = setupSearchDomainIfRelevant(this->heuristicType, proteinMaxRadius);
                }



                for (unsigned int retryNum = 0; retryNum < this->retryPerConformer; retryNum++) {

                    std::shared_ptr<Heuristics::GlobalHeuristic> globalHeuristic = globalHeuristicFactory(heuristicType,
                                                                                                          scoringFunction.get(),
                                                                                                          localOptimizer.get(),
                                                                                                          seed +
                                                                                                          retryNum,
                                                                                                          this->heurParams);

                    globalHeuristic->search();

                    auto rawResultMatrix = globalHeuristic->getResultMatrix();
                    double score_withIntra = scoringFunction->Evaluate(rawResultMatrix);
                    double score = scoringFunction->EvaluateOnlyIntermolecular(rawResultMatrix);

                    std::vector<std::tuple<std::string, double>> subcomponents =  scoringFunction->EvaluateSubcomponents(rawResultMatrix);


                    iConformer result = scoringFunction->getConformerForParamMatrix(rawResultMatrix);

                    double starting_score = Score::vina_like_rigid_inter_scoring_func(conformer, starting_pos_tr,
                                                                                      this->protein);

                    iTransform tr = iTransformIdentityInit();
                    double real_score = Score::vina_like_rigid_inter_scoring_func(result, tr,
                                                                                  this->fullProtein);



                    BOOST_LOG_TRIVIAL(debug) << "VinaClassic Starting score : " << starting_score;
                    BOOST_LOG_TRIVIAL(debug) << "Score with intra   : " << score_withIntra;
                    BOOST_LOG_TRIVIAL(debug) << "Score after search : " << score;
                    BOOST_LOG_TRIVIAL(debug) << "VinaClassic end score : " << real_score;
                    BOOST_LOG_TRIVIAL(debug) << " --- Subcomponents --- ";
                    for(auto& subcomp: subcomponents)
                    {
                        BOOST_LOG_TRIVIAL(debug) << "   " << std::get<0>(subcomp) << " : " << std::get<1>(subcomp);
                    }

                    BOOST_LOG_TRIVIAL(debug) << " --------------------- ";

                    if (score != 0.0) {
                        this->startingScores.push_back(starting_score);
                        this->localScores.push_back(score);
                        this->scores.push_back(real_score);
                        this->allGeneratediConformer.push_back(result);
                        acc_score(real_score);
                    }

                }


                record_timings(end_docking_this_conformer);


                acc_duration(
                        static_cast< std::chrono::duration<double> >(end_docking_this_conformer -
                                                                     begin_docking_this_conformer).count()
                );

            }

            record_timings(end_docking);

            auto total_docking_duration = static_cast< std::chrono::duration<double> >(end_docking -
                                                                                       begin_docking).count();

            this->meanDuration = mean(acc_duration);
            this->stdDevDuration = std::sqrt(moment<2>(acc_duration));

#ifdef SMOLDOCK_VERBOSE_DEBUG

            BOOST_LOG_TRIVIAL(debug) << "Timings ConformerDockingEngine docking runs"
                                     << "\n      TOTAL: " << total_docking_duration << "s"
                                     << "\n      Mean per conformer: " << this->meanDuration << "s"
                                     << "\n      StdDev per conformer: " <<  this->stdDevDuration << "s";
#endif

            this->meanScore =  mean(acc_score);
            this->stdDevScore = std::sqrt(moment<2>(acc_score));

            BOOST_LOG_TRIVIAL(info) << "Results:";
            BOOST_LOG_TRIVIAL(info) << "   Score Mean: " << this->meanScore;
            BOOST_LOG_TRIVIAL(info) << "   Score StdDev: " << this->stdDevScore;
            BOOST_LOG_TRIVIAL(info) << "   Scores: ";
            for (unsigned int i = 0; i < this->scores.size(); i++) {
                BOOST_LOG_TRIVIAL(info) << "      " << this->startingScores[i] << " -> " << this->localScores[i]
                                        << " -> " << this->scores[i];
            }

            BOOST_LOG_TRIVIAL(info) << "   Best scores : ";

            std::vector<std::tuple<int, double>> scoreAndIndices;
            for (unsigned int i = 0; i < this->scores.size(); i++) {
                // scoreAndIndices.push_back(std::make_tuple(i, this->localScores[i]));
                scoreAndIndices.push_back(std::make_tuple(i, this->scores[i]));
            }

            std::sort(std::begin(scoreAndIndices),
                             std::end(scoreAndIndices),
                             [](const std::tuple<int, double> &a, const std::tuple<int, double> &b) {
                                 return std::get<1>(a) < std::get<1>(b);
                             });
            for (unsigned int i = 0; i < conformer_num; i++) {
                BOOST_LOG_TRIVIAL(info) << "      " << this->startingScores[std::get<0>(scoreAndIndices[i])]
                                        << " -> " << this->localScores[std::get<0>(scoreAndIndices[i])]
                                        << " -> " << this->scores[std::get<0>(scoreAndIndices[i])];
                this->bestiConformer.push_back(this->allGeneratediConformer[std::get<0>(scoreAndIndices[i])]);
            }
            this->bestScore = std::get<1>(scoreAndIndices[0]);

            BOOST_LOG_TRIVIAL(info) << "";
            BOOST_LOG_TRIVIAL(info) << "Best score : " << this->startingScores[std::get<0>(scoreAndIndices[0])]
                                                       << " -> " << this->localScores[std::get<0>(scoreAndIndices[0])]
                                                       << " -> " << this->scores[std::get<0>(scoreAndIndices[0])];

        }

        std::shared_ptr<DockingResult> ConformerDockingEngine::getDockingResult() {
            auto ret = std::make_shared<DockingResult>();
            for (const auto &confr : this->bestiConformer) {
                Molecule finalLigand = this->orig_ligand->deepcopy();
                finalLigand.updateAtomPositionsFromiConformer(confr);
                ret->ligandPoses.emplace_back(finalLigand);
            }
            return ret;
        }


        bool ConformerDockingEngine::setDockingBox(AbstractDockingEngine::DockingBoxSetting setting) {
            this->dockBoxSettings = setting;

            if (!(setting.shape == DockingBoxSetting::Shape::sphere)) {
                BOOST_LOG_TRIVIAL(error) << "The passed DockingBoxSetting is not yet implemented.";
                BOOST_LOG_TRIVIAL(error) << "Running as if DockingBoxSetting::whole_protein was passed";
                this->dockBoxSettings.shape = DockingBoxSetting::Shape::whole_protein;
                return false;
            }
            return true;
        }

        std::tuple<double, double> ConformerDockingEngine::getMeanStdDevDuration() const {
            return std::make_tuple(this->meanDuration,this->stdDevDuration);
        }

        std::tuple<double, double> ConformerDockingEngine::getMeanStdDevScore() const {
            return std::make_tuple(this->meanScore,this->stdDevScore);
        }

        double ConformerDockingEngine::getBestScore() {
            return this->bestScore;
        }


    }

}