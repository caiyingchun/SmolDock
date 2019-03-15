//
// Created by eliane on 13/03/19.
//

#include "Calibrator.h"

#include <cmath>
#include <thread>

#include <boost/log/trivial.hpp>

#define USE_BOOST_KARMA

#include <bprinter/table_printer.h>

#include <armadillo>


#include <Engines/Internals/InternalsUtilityFunctions.h>


namespace SmolDock::Calibration {

    Calibrator::Calibrator(Score::ScoringFunctionType scFuncType,
                           Heuristics::GlobalHeuristicType heurType,
                           Optimizer::LocalOptimizerType localOptimizerType_,
                           unsigned int maxLearningSteps,
                           double learningRate,
                           unsigned int rngSeed,
                           Heuristics::HeuristicParameters hParams,
                           unsigned int conformerNumber,
                           unsigned int retryNumber) :
            scoringFunctionType(scFuncType),
            heuristicType(heurType),
            localOptimizerType(localOptimizerType_),
            maxLearningSteps(maxLearningSteps),
            learningRate(learningRate),
            hParams(std::move(hParams)),
            rndGenerator(rngSeed),
            conformerNumber(conformerNumber),
            retryNumber(retryNumber) {

        if (conformerNumber == 0 || retryNumber == 0) {
            BOOST_LOG_TRIVIAL(error) << "Cannot run calibration with number of conformer/number of try per conformer = "
                                     << conformerNumber << "," << retryNumber;
        }

        iConformer dummy_cf;
        dummy_cf.num_rotatable_bond = 0;
        iProtein dummy_prot;
        iTransform dummy_tr = iTransformIdentityInit(0);
        this->dummy_sf = scoringFunctionFactory(this->scoringFunctionType,
                                                dummy_cf,
                                                dummy_prot,
                                                dummy_tr,
                                                1e-3,
                                                true);

        this->currentCoeffs = this->dummy_sf->getCurrentCoefficients();
        this->nameOfAllCoeffs = this->dummy_sf->getCoefficientsNames();
    }


    bool Calibrator::addReferenceLigand_SMILES_Ki(ReceptorID recID, const std::string &smiles, double Ki, int seed) {
        auto mol_sptr = std::make_shared<Molecule>(true); // FIXME : no flexible rings
        mol_sptr->populateFromSMILES(smiles, seed);


        const double R = 8.3144598;
        const double T = 310.15; // 37 degree celsius
        double deltaG = R * T * std::log(Ki) / 1000; // kcal/mol aka same as other docking software

        this->referenceLigands[recID].emplace_back(std::make_tuple(mol_sptr, deltaG, std::vector<iConformer>()));

        return true;
    }

    bool Calibrator::addReferenceLigand_Mol_Ki(Calibrator::ReceptorID recID, const Molecule &mol, double Ki, int seed) {
        auto mol_sptr = std::make_shared<Molecule>(mol.deepcopy());

        const double R = 8.3144598;
        const double T = 310.15; // 37 degree celsius
        double deltaG = R * T * std::log(Ki) / 1000; // kcal/mol aka same as other docking software

        this->referenceLigands[recID].emplace_back(std::make_tuple(mol_sptr, deltaG, std::vector<iConformer>()));
        return true;
    }

    Calibrator::ReceptorID Calibrator::addReceptor(const Protein &prot,
                                                   Engine::AbstractDockingEngine::DockingBoxSetting dbsettings) {

        referenceReceptor.emplace_back(
                std::make_tuple(std::make_shared<Protein>(prot), dbsettings, iProtein(), iProtein()));
        current_max_ReceptorID++;
        return current_max_ReceptorID - 1;
    }

    bool Calibrator::setupCalibration() {

        BOOST_LOG_TRIVIAL(info) << "Setting up calibration...";

        std::uniform_int_distribution<> dis_int(0, std::numeric_limits<int>::max());
        std::uniform_real_distribution<double> dis_real_position(-100.0, 100.0);


        for (unsigned int i = 0; i < this->referenceReceptor.size(); i++) {
            auto &protein = std::get<std::shared_ptr<Protein>>(this->referenceReceptor[i]);
            auto &iProt = std::get<2>(this->referenceReceptor[i]);
            auto &fulliProt = std::get<3>(this->referenceReceptor[i]);
            auto &settings = std::get<Engine::AbstractDockingEngine::DockingBoxSetting>(this->referenceReceptor[i]);

            if (settings.type == Engine::AbstractDockingEngine::DockingBoxSetting::Type::centeredAround) {
                iProt = protein->getPartialiProtein_sphere(settings.center, settings.radius, 2.0);
            } else {
                iProt = protein->getiProtein();
            }

            fulliProt = protein->getiProtein();

            for (auto &j : this->referenceLigands[i]) {
                auto mol_sptr = std::get<std::shared_ptr<Molecule>>(j);
                auto &conformer_vector = std::get<std::vector<iConformer>>(j);
                mol_sptr->generateConformers(conformer_vector, this->conformerNumber, true,
                                             dis_int(this->rndGenerator));
            }
        }

        return true;
    }


    bool Calibrator::runCalibration() {

        BOOST_LOG_TRIVIAL(info) << "Running the calibration...";

        std::uniform_int_distribution<unsigned int> dis_uint(0, std::numeric_limits<unsigned int>::max());


        std::vector<double> averageLossHistory;

        for (unsigned int learningEpoch = 1; learningEpoch <= this->maxLearningSteps; learningEpoch++) {

            BOOST_LOG_TRIVIAL(info)
                << " ------------- CALIBRATION EPOCH " << learningEpoch << " of " << this->maxLearningSteps
                << " ----------------";

            auto workItemVector = std::make_shared<std::vector<CalibratorWorkItem>>();
            auto resultMutex = std::make_shared<std::mutex>();
            auto local_scores = std::make_shared<std::vector<double>>() ;
            auto local_referenceScores = std::make_shared<std::vector<double>>();
            auto local_scoreComponents = std::make_shared<std::vector<std::vector<std::tuple<std::string, double>>>>();


            for (unsigned int receptorIdx = 0; receptorIdx < this->referenceReceptor.size(); receptorIdx++) {
                auto &protein = std::get<std::shared_ptr<Protein>>(this->referenceReceptor[receptorIdx]);
                auto &iProt = std::get<2>(this->referenceReceptor[receptorIdx]);
                auto &fulliProt = std::get<3>(this->referenceReceptor[receptorIdx]);
                auto &settings = std::get<Engine::AbstractDockingEngine::DockingBoxSetting>(
                        this->referenceReceptor[receptorIdx]);

                if (this->hParams.index() == 0) // LackOfParameter
                {
                    BOOST_LOG_TRIVIAL(debug)
                        << "Received default heuristics parameters, setting up search domain if relevant";

                    double proteinMaxRadius = (settings.type ==
                                               Engine::AbstractDockingEngine::DockingBoxSetting::Type::centeredAround) ?
                                              settings.radius : protein->getMaxRadius();
                    this->hParams = setupSearchDomainIfRelevant(this->heuristicType, proteinMaxRadius);
                }

                for (auto &ligandRecord : this->referenceLigands[receptorIdx]) {
                    auto &conformer_vector = std::get<std::vector<iConformer>>(ligandRecord);
                    double referenceScore = std::get<double>(ligandRecord);

                    iTransform starting_pos_tr = iTransformIdentityInit(conformer_vector[0].num_rotatable_bond);
                    if (settings.type == Engine::AbstractDockingEngine::DockingBoxSetting::Type::centeredAround) {
                        starting_pos_tr.transl.x() += settings.center[0];
                        starting_pos_tr.transl.y() += settings.center[1];
                        starting_pos_tr.transl.z() += settings.center[2];
                    }


                    for (auto &conformer : conformer_vector) {


                        CalibratorWorkItem item = {.scFuncType_ = this->scoringFunctionType,
                                .heurType_ = this->heuristicType,
                                .localOptimizerType_ = this->localOptimizerType,
                                .transform_ = starting_pos_tr,
                                .conformer_ = &conformer,
                                .prot_ = &iProt,
                                .fullProt_ = &fulliProt,
                                .seed_ = dis_uint(this->rndGenerator),
                                .retryNumber_ = this->retryNumber,
                                .hParams_ = this->hParams,
                                .referenceScore_ = referenceScore,
                        };

                        workItemVector->emplace_back(std::move(item));


                    }

                }


            }


            // At this point all work items for this epoch have been added
            // We parellel run them
            tbb::parallel_for(tbb::blocked_range<size_t>(0,workItemVector->size()),
                    CalibratorLoopRunner(workItemVector,
                                        resultMutex,
                                        local_scores,
                                        local_referenceScores,
                                        local_scoreComponents,
                                         this->currentCoeffs)
                    );



            bprinter::TablePrinter tp(&std::cout);
            tp.AddColumn("iteration", 10);
            tp.AddColumn("Local scores", 25);
            tp.AddColumn("Local DeltaG", 25);


            tp.PrintHeader();
            for (unsigned int l = 0; l < local_referenceScores->size(); ++l) {
                tp << l << local_scores->at(l) << local_referenceScores->at(l);
            }
            tp.PrintFooter();

//            std::vector<double> local_scores;
//            std::vector<double> local_referenceScores;
//             std::vector<std::vector<std::tuple<std::string,double>>> local_scoreComponents;

            double averageOutputScore =
                    std::accumulate(local_scores->begin(), local_scores->end(), 0.0) / local_scores->size();
            double averageReferenceScore =
                    std::accumulate(local_referenceScores->begin(), local_referenceScores->end(), 0.0) /
                    local_referenceScores->size();
            double average_loss = averageReferenceScore - averageOutputScore;

            unsigned int numElementInBatch = local_scores->size();

            BOOST_LOG_TRIVIAL(info) << "\n\n";
            for (unsigned int idxCoeff : this->idxOfCoeffsToCalibrate) {
                std::string coeffName = this->nameOfAllCoeffs[idxCoeff];
                double nonUpdatedCoeff = this->currentCoeffs[idxCoeff];

                double meanValueInput = 0.0;
                for (auto &record : *local_scoreComponents) {
                    auto it = std::find_if(record.begin(), record.end(),
                                           [&coeffName](auto &e) { return std::get<std::string>(e) == coeffName; });
                    if (it == record.end())
                        BOOST_LOG_TRIVIAL(error) << "Coefficient \"" << coeffName
                                                 << "\" was not found among the score subcomponents, this is a bug.", std::terminate();

                    meanValueInput += std::get<double>(*it);
                }
                meanValueInput = meanValueInput / numElementInBatch;


                double deltaCoeff = (this->learningRate /* /learningEpoch */) * meanValueInput * average_loss;


                this->currentCoeffs[idxCoeff] = nonUpdatedCoeff + deltaCoeff;

                BOOST_LOG_TRIVIAL(info) << " ---- COEFFICIENT " << coeffName << " ---- ";
                BOOST_LOG_TRIVIAL(info) << "   Old coeff        : " << nonUpdatedCoeff;
                BOOST_LOG_TRIVIAL(info) << "   Mean input value : " << meanValueInput;
                BOOST_LOG_TRIVIAL(info) << "   Learning rate    : " << this->learningRate;
                BOOST_LOG_TRIVIAL(info) << "   Avg Loss         : " << average_loss;
                BOOST_LOG_TRIVIAL(info) << "   Delta coeff      : " << deltaCoeff;
                BOOST_LOG_TRIVIAL(info) << "   New coeff        : " << this->currentCoeffs[idxCoeff];
                BOOST_LOG_TRIVIAL(info) << " ----------------------------- \n";
            }

            BOOST_LOG_TRIVIAL(info) << "\n";
            BOOST_LOG_TRIVIAL(info) << " ---- LOSS HISTORY ---- \n";

            averageLossHistory.push_back(average_loss);

            bprinter::TablePrinter losstable(&std::cout);
            losstable.AddColumn("iteration", 10);
            losstable.AddColumn("Average loss", 25);

            losstable.PrintHeader();
            for (unsigned int l = 0; l < averageLossHistory.size(); ++l) {
                losstable << l << averageLossHistory[l];
            }
            losstable.PrintFooter();

            BOOST_LOG_TRIVIAL(info) << "\n";
            BOOST_LOG_TRIVIAL(info) << " ----------------------- ";


            BOOST_LOG_TRIVIAL(info) << "\n";
            BOOST_LOG_TRIVIAL(info) << " ---- CYCLE RESULTS ---- ";
            BOOST_LOG_TRIVIAL(info) << "  Cycle #           : " << learningEpoch << " of " << this->maxLearningSteps;
            BOOST_LOG_TRIVIAL(info) << "  Average loss      : " << average_loss;
            BOOST_LOG_TRIVIAL(info) << " ----------------------- \n";

            BOOST_LOG_TRIVIAL(info)
                << " -------------  END OF CALIBRATION CYCLE  ----------------\n\n";
        }


        return true;
    }

    bool Calibrator::coefficientsToCalibrate(std::vector<std::string> nameOfCoeffs) {


        std::vector<std::string> names = this->dummy_sf->getCoefficientsNames();
        bool hasNameNotFound = false;
        for (auto &nameGiven: nameOfCoeffs) {
            auto it = std::find(names.begin(), names.end(), nameGiven);
            if (it == names.end()) {
                BOOST_LOG_TRIVIAL(warning) << "Coefficient to calibrate \"" << nameGiven
                                           << "\" was not found among the optimizable coefficients.";
                hasNameNotFound = true;
            } else {
                this->coeffsToCalibrate.push_back(nameGiven);
                this->idxOfCoeffsToCalibrate.push_back(std::distance(names.begin(), it));
            }

        }
        if (hasNameNotFound) {
            BOOST_LOG_TRIVIAL(warning) << "List of optimizable coefficients for "
                                       << scoringFunctionTypeToString(this->scoringFunctionType) << " : ";
            for (auto &nameCoeff : names) {
                BOOST_LOG_TRIVIAL(warning) << "  - " << nameCoeff;
            }
        }
        BOOST_LOG_TRIVIAL(info) << "Calibrating the following coefficients :";
        for (auto &nameCoeff : this->coeffsToCalibrate) {
            BOOST_LOG_TRIVIAL(info) << "  - " << nameCoeff;
        }

        return true;
    }




    CalibratorLoopRunner::CalibratorLoopRunner(std::shared_ptr<std::vector<CalibratorWorkItem>> workItemList,
            std::shared_ptr<std::mutex> resultMutex_,
            std::shared_ptr<std::vector<double>> local_scores_,
            std::shared_ptr<std::vector<double>> local_referenceScores_,
            std::shared_ptr<std::vector<std::vector<std::tuple<std::string,double>>>> local_scoreComponents_,
            std::vector<double> currentCoeffs_,
            std::vector<unsigned int> indexShufflingArray_) :
                workItemList(workItemList),
                resultMutex(resultMutex_),
                local_scores(local_scores_),
                local_referenceScores(local_referenceScores_),
                local_scoreComponents(local_scoreComponents_),
                currentCoeffs(currentCoeffs_)
            {
        if(indexShufflingArray_.size() == 0)
        {
            for (unsigned int j = 0; j < this->workItemList->size(); ++j) {
                this->indexShufflingArray.push_back(j);
            }
        }else {
            assert(this->workItemList->size() == indexShufflingArray_.size());
            this->indexShufflingArray = indexShufflingArray_;
        }
    }

    void CalibratorLoopRunner::operator()(const tbb::blocked_range<size_t> &r) const {
        for (size_t i = r.begin(); i != r.end(); ++i) {
            CalibratorWorkItem &item = this->workItemList->at(this->indexShufflingArray[i]);

            item.transform_.transl.x() += item.conformer_->centroidNormalizingTransform.x();
            item.transform_.transl.y() += item.conformer_->centroidNormalizingTransform.y();
            item.transform_.transl.z() += item.conformer_->centroidNormalizingTransform.z();
            item.transform_.rota.normalize();

            arma::arma_rng::set_seed(item.seed_);

            std::shared_ptr<Score::ScoringFunction> scoringFunction = scoringFunctionFactory(
                    item.scFuncType_,
                    *item.conformer_,
                    *item.prot_,
                    item.transform_,
                    1e-3,
                    true);

            scoringFunction->setNonDefaultCoefficients(this->currentCoeffs);


            std::shared_ptr<Score::ScoringFunction> fullScoringFunction = scoringFunctionFactory(
                    item.scFuncType_,
                    *item.conformer_,
                    *item.fullProt_,
                    item.transform_,
                    1e-3);

            std::shared_ptr<Optimizer::Optimizer> localOptimizer = optimizerFactory(
                    item.localOptimizerType_,
                    scoringFunction.get(),
                    1e-3);

            double bestScoreAmongRetry = std::numeric_limits<double>::max();
            std::vector<std::tuple<std::string,double>> bestComponentsAmongRetry;
            for (unsigned int k = 0; k < item.retryNumber_; ++k) {

                {
                    std::lock_guard lock(*this->resultMutex);
                    BOOST_LOG_TRIVIAL(debug) << "Run " << k << " of " << item.retryNumber_;
                }

                std::shared_ptr<Heuristics::GlobalHeuristic> globalHeuristic = globalHeuristicFactory(
                        item.heurType_,
                        scoringFunction.get(),
                        localOptimizer.get(),
                        item.seed_ + k,
                        item.hParams_);

                globalHeuristic->search();

                auto rawResultMatrix = globalHeuristic->getResultMatrix();
                double score = scoringFunction->EvaluateOnlyIntermolecular(rawResultMatrix);
                double fullScore = fullScoringFunction->EvaluateOnlyIntermolecular(rawResultMatrix);
                double delta_full = fullScore / score;

                if (delta_full > 1.2 || delta_full < 0.80) {
                    std::lock_guard lock(*this->resultMutex);
                    BOOST_LOG_TRIVIAL(debug)
                        << "Discrepency between score for partial and full protein, ignoring this run ";
                    continue;
                }

                if(fullScore < bestScoreAmongRetry) {
                    bestScoreAmongRetry = fullScore;
                    bestComponentsAmongRetry = scoringFunction->EvaluateSubcomponents(rawResultMatrix);
                }

            }

            {
                std::lock_guard lock(*this->resultMutex);
                this->local_scores->push_back(bestScoreAmongRetry);
                this->local_referenceScores->push_back(item.referenceScore_);
                this->local_scoreComponents->push_back(bestComponentsAmongRetry);

                BOOST_LOG_TRIVIAL(debug) << " ---- Work item " << i << "  -------";
                BOOST_LOG_TRIVIAL(debug) << "Score       : " << bestScoreAmongRetry;
                BOOST_LOG_TRIVIAL(debug) << "Delta G     : " << item.referenceScore_;
                BOOST_LOG_TRIVIAL(debug) << " --------------------------";


            }

        }
    }


    CalibratorEnsmallenLayer::CalibratorEnsmallenLayer(std::shared_ptr<std::vector<CalibratorWorkItem>> workitemVector_,
                                                       unsigned int paramSize_,
                                                       unsigned int seed_,
                                                       double differentialEpsilon_) :
    paramSize(paramSize_),
    workitemVector(workitemVector_),
    differentialEpsilon(differentialEpsilon_),
    rndGenerator(seed_)
    {
        this->numWorkItem = workitemVector->size();
        for (unsigned int j = 0; j < this->numWorkItem; ++j) {
            this->shufflexIndexArray[j] = j;
        }

    }

    double CalibratorEnsmallenLayer::doRealEvaluate(const arma::mat &x, size_t i, size_t batchSize) {
        std::shared_ptr<std::mutex> resultMutex;
        std::shared_ptr<std::vector<double>> local_scores;
        std::shared_ptr<std::vector<double>> local_referenceScores;
        std::shared_ptr<std::vector<std::vector<std::tuple<std::string,double>>>> local_scoreComponents;

        std::vector<double> coeffs;
        assert(x.n_rows == this->paramSize);
        for (unsigned int j = 0; j < this->paramSize; ++j) {
            coeffs[j] = x[j];
        }

        tbb::parallel_for(tbb::blocked_range<size_t>(i,i+batchSize),
                          CalibratorLoopRunner(this->workitemVector,
                                               resultMutex,
                                               local_scores,
                                               local_referenceScores,
                                               local_scoreComponents,
                                               coeffs)
        );

        double totalOutputScore = std::accumulate(local_scores->begin(), local_scores->end(), 0.0);

        return totalOutputScore;
    }

    double CalibratorEnsmallenLayer::Evaluate(const arma::mat &x, const size_t i, const size_t batchSize) {
        return this->doRealEvaluate(x,i,batchSize);
    }

    void CalibratorEnsmallenLayer::Gradient(const arma::mat &x, const size_t i, arma::mat &g, const size_t batchSize) {
        assert(x.n_rows == g.n_rows);
        for (unsigned int j = 0; j < x.n_rows; ++j) {
            arma::mat gradientX = x;
            gradientX[j] = gradientX[j] + this->differentialEpsilon;
            double value = this->doRealEvaluate(gradientX,i,batchSize);
            g[j] = value;
        }
    }

    void CalibratorEnsmallenLayer::Shuffle() {
        std::shuffle(this->shufflexIndexArray.begin(), this->shufflexIndexArray.end(), this->rndGenerator);
    }

    size_t CalibratorEnsmallenLayer::NumFunctions() {
        return this->numWorkItem;
    }


} // namespace SmolDock