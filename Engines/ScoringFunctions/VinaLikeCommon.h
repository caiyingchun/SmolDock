//
// Created by eliane on 04/03/19.
//

#ifndef SMOLDOCK_VINALIKECOMMON_H
#define SMOLDOCK_VINALIKECOMMON_H

#include <boost/log/trivial.hpp>

#include <cmath>
#include <boost/assert.hpp>

#include <Vc/Vc>


#include <Structures/Atom.h>


namespace SmolDock::Score {


    // ////////////////// VINA CODE //////////////////////////////////////////////////
    // Until the closing "VINA CODE END" comment, the following code is adapted from Autodock Vina
    // Copyright (c) 2006-2010, The Scripps Research Institute
    // Released under the Apache 2.0 licence   http://www.apache.org/licenses/LICENSE-2.0
    // See COPYING for more details on licence information


    __attribute__((const)) inline bool
    isHydrophobic(const unsigned int atomicNumber, const unsigned int variantFlags) noexcept {
        return (atomicNumber == 6 && (variantFlags & ((const unsigned int) Atom::AtomVariant::apolar))) ||
               // C && apolar
               atomicNumber == 9 || // F
               atomicNumber == 17 || // Cl
               atomicNumber == 35 || // Br
               atomicNumber == 53; // I
    }

    __attribute__((const)) inline double
    isHydrophobic_prepareMask(const unsigned int atomicNumber, const unsigned int variantFlags) noexcept {
        if((atomicNumber == 6 && (variantFlags & ((const unsigned int) Atom::AtomVariant::apolar))) ||
           // C && apolar
           atomicNumber == 9 || // F
           atomicNumber == 17 || // Cl
           atomicNumber == 35 || // Br
           atomicNumber == 53) {
            return 1.0;
        }else {
            return 0.0;
        }

    }


    __attribute__((const)) inline bool
    isHydrogenAcceptor(const unsigned int atomicNumber, const unsigned int atomVariantFlags) noexcept {
        return (atomicNumber == 7 || //N
                atomicNumber == 8) // O
               && (atomVariantFlags & ((const unsigned int) Atom::AtomVariant::hydrogenAcceptor));
    }

    __attribute__((const)) inline bool
    isHydrogenDonor(const unsigned int atomicNumber, const unsigned int atomVariantFlags) noexcept {
        return (atomicNumber == 7 ||
                atomicNumber == 8)
               && (atomVariantFlags & ((const unsigned int) Atom::AtomVariant::hydrogenDonor));
    }

    __attribute__((const)) inline bool
    hydrogenDonorAcceptorPair(const unsigned int atomicNumber1, const unsigned int atom1VariantFlags,
                              const unsigned int atomicNumber2, const unsigned int atom2VariantFlags) noexcept {
        return isHydrogenDonor(atomicNumber1, atom1VariantFlags) &&
               isHydrogenAcceptor(atomicNumber2, atom2VariantFlags);
    }

    __attribute__((const)) inline bool
    hydrogenBondingPossible(const unsigned int atomicNumber1, const unsigned int atom1VariantFlags,
                            unsigned char atomicNumber2, unsigned int atom2VariantFlags) noexcept {
        return hydrogenDonorAcceptorPair(atomicNumber1, atom1VariantFlags, atomicNumber2, atom2VariantFlags) ||
               hydrogenDonorAcceptorPair(atomicNumber2, atom2VariantFlags, atomicNumber1, atom1VariantFlags);
    }

    __attribute__((const)) inline double
    hydrogenBondingPossible_prepareMask(const unsigned int atomicNumber1, const unsigned int atom1VariantFlags,
                            unsigned char atomicNumber2, unsigned int atom2VariantFlags) noexcept {
        if(hydrogenDonorAcceptorPair(atomicNumber1, atom1VariantFlags, atomicNumber2, atom2VariantFlags) ||
           hydrogenDonorAcceptorPair(atomicNumber2, atom2VariantFlags, atomicNumber1, atom1VariantFlags)) {
            return 1.0;
        }else {
            return 0.0;
        }

    }

    // ////////////////// VINA CODE END //////////////////////////////////////////////////

    __attribute__((const)) inline bool
    isCovalentReversibleAcceptor(const unsigned int atomicNumber, const unsigned int atomVariantFlags) noexcept {
        return static_cast<bool>((atomVariantFlags &
                                  ((const unsigned int) Atom::AtomVariant::covalentReversibleAcceptor)));
    }

    __attribute__((const)) inline bool
    isCovalentReversibleDonor(const unsigned int atomicNumber, const unsigned int atomVariantFlags) noexcept {
        return static_cast<bool>((atomVariantFlags &
                                  ((const unsigned int) Atom::AtomVariant::covalentReversibleDonor)));
    }

    __attribute__((const)) inline bool
    covalentReversiblePair(const unsigned int atomicNumber1, const unsigned int atom1VariantFlags,
                           const unsigned int atomicNumber2, const unsigned int atom2VariantFlags) noexcept {
        return isCovalentReversibleDonor(atomicNumber1, atom1VariantFlags) &&
               isCovalentReversibleAcceptor(atomicNumber2, atom2VariantFlags);
    }


    __attribute__((const)) inline bool
    covalentReversibleBondingPossible(const unsigned int atomicNumber1, const unsigned int atom1VariantFlags,
                                      unsigned char atomicNumber2, unsigned int atom2VariantFlags) noexcept {
        return covalentReversiblePair(atomicNumber1, atom1VariantFlags, atomicNumber2, atom2VariantFlags) ||
               covalentReversiblePair(atomicNumber2, atom2VariantFlags, atomicNumber1, atom1VariantFlags);
    }


    // The following scoring function was implemented without knowledge of the Vina code proper, using papers such as
    // DOI:10.1371/journal.pone.0155183 or the original Vina paper, that describe the scoring function but do not
    // show code. The functions/constants above this comment (VINA CODE) were used after testing revealed small numerical
    // differences between this software and Vina.
    // The following function is thus thought to not be derived work under copyright law. However, if it actually is,
    // it could be distributed under the GPLv 3 as the original Vina code is released under Apache 2.0.


    namespace VinaClassic {
        constexpr const double coeff_gauss1 = -0.035579;
        constexpr const double coeff_gauss2 = -0.005156;
        constexpr const double coeff_repulsion = 0.840245;
        constexpr const double coeff_hydrophobic = -0.035069;
        constexpr const double coeff_hydrogen = -0.587439;
        constexpr const double coeff_entropic = 0.058459999999999998;

        constexpr const double interaction_cutoff = 8.0;
    }


    __attribute__((const)) inline double distanceFromRawDistance(const double rawDistance,
                                                                 const double atomicRadiusLig,
                                                                 const double atomicRadiusProt) noexcept {
        return rawDistance - (atomicRadiusLig + atomicRadiusProt);
    }

    __attribute__((const)) inline Vc::Vector<double> distanceFromRawDistance(const Vc::Vector<double> rawDistance,
                                                                             const Vc::Vector<double> atomicRadiusLig,
                                                                             const Vc::Vector<double> atomicRadiusProt) noexcept {
        return rawDistance - (atomicRadiusLig + atomicRadiusProt);
    }


    __attribute__((const)) inline double
    vinaGaussComponent(const double distance,
                       const double offset,
                       const double multiplier) noexcept {
        return std::exp(-1 * std::pow((distance - offset) / multiplier, 2));
    }

    __attribute__((const)) inline Vc::Vector<double>
    vinaGaussComponent(const Vc::Vector<double> distance,
                       const double offset,
                       const double multiplier) noexcept {
        // x^y = pow(x,y) = exp(y*log(x))
        const Vc::Vector<double> centeredDist = (distance - offset) / multiplier;
        const Vc::Vector<double> squaredDist = centeredDist * centeredDist;
        return Vc::exp(-1 * squaredDist);
    }

    __attribute__((const)) inline double vinaRepulsionComponent(const double distance, const double cutoff) noexcept {
        if (distance < cutoff) {
            return std::pow(distance, 2);
        } else {
            return 0.0;
        }
    }

    __attribute__((const)) inline Vc::Vector<double> vinaRepulsionComponent(const Vc::Vector<double> distance,
                                                                            const double cutoff) noexcept {
        return  Vc::iif ((0.0 < distance) &&  (distance < cutoff), distance * distance, Vc::Vector<double>(Vc::Zero)); // iff -> cond ? x : y
    }

    __attribute__((const)) inline double
    vinaHydrophobicComponent(const double distance, const unsigned int atomicNumberAtom1,
                             const unsigned int variantFlagsAtom1,
                             const unsigned int atomicNumberAtom2, const unsigned int variantFlagsAtom2) noexcept {
        if (isHydrophobic(atomicNumberAtom1, variantFlagsAtom1) &&
            isHydrophobic(atomicNumberAtom2, variantFlagsAtom2)) // "Hydrophobic" atoms
        {
            if (distance >= 1.5)
                return 0.0;
            if (distance <= 0.5)
                return 1.0;
            //then  (0.5 < distance && distance < 1.5) holds
            return (1.5 - distance);
        }
        return 0.0;
    }

    __attribute__((const)) inline Vc::Vector<double>
    vinaHydrophobicComponent(const Vc::Vector<double> distances, Vc::Mask<double> hydrophobicMask) noexcept {

        Vc::Vector<double> retVect(Vc::Vector<double>::Zero());

        Vc::Mask<double> oneFiveDistMask = distances < 1.5;
        Vc::Mask<double> zeroFiveDistMask = distances <= 0.5;

        Vc::Mask<double> hydrophobicAndzeroFive = (hydrophobicMask && zeroFiveDistMask);
        Vc::Mask<double> hydrophobicAndoneFive = (hydrophobicMask && oneFiveDistMask) && (!zeroFiveDistMask);

        Vc::where(hydrophobicAndzeroFive)(retVect) = 1.0;
        Vc::where(hydrophobicAndoneFive)(retVect) = (-distances / 0.7);

        return retVect;
    }

    __attribute__((const)) inline double
    vinaHydrogenComponent(const double distance, const unsigned int atomicNumberAtom1,
                          const unsigned int variantFlagsAtom1,
                          const unsigned int atomicNumberAtom2, const unsigned int variantFlagsAtom2) noexcept {
        if (hydrogenBondingPossible(atomicNumberAtom1, variantFlagsAtom1, atomicNumberAtom2,
                                    variantFlagsAtom2)) // Hydrogen donor and acceptor
        {
            if (distance < -0.7) {
                return 1.0;
            } else if (distance < 0) //  // ==> distance between -0.7 and 0
            {
                return -distance / 0.7;
            }
        }
        return 0.0;
    }

    __attribute__((const)) inline Vc::Vector<double>
    vinaHydrogenComponent(const Vc::Vector<double> distances, Vc::Mask<double> hydrogenMask) noexcept {
        Vc::Vector<double> retVect(Vc::Vector<double>::Zero());

        Vc::Mask<double> zeroDistMask = distances < 0;
        Vc::Mask<double> zeroSevenDistMask = distances < -0.7;

        Vc::Mask<double> hydrogenAndZeroSeven = (hydrogenMask && zeroSevenDistMask);
        Vc::Mask<double> hydrogenAndZero = (hydrogenMask && zeroDistMask) && (!zeroSevenDistMask);

        Vc::where(hydrogenAndZeroSeven)(retVect) = 1.0;
        Vc::where(hydrogenAndZero)(retVect) = (-distances / 0.7);

        return retVect;
    }


    __attribute__((const)) inline double
    scoreForAtomCouple(const double distance, const unsigned int atom1AtomicNumber,
                       const unsigned int atom1AtomVariant,
                       const unsigned int atom2AtomicNumber, const unsigned int atom2AtomVariant) noexcept {

        double score_intermol = 0.0;

        // exp −(d/0.5Å)^2
        const double gauss1 = vinaGaussComponent(distance, 0.0, 0.5);
        score_intermol += VinaClassic::coeff_gauss1 * gauss1;

        // exp −((d−3Å)/2Å)^2
        const double gauss2 = vinaGaussComponent(distance, 3.0, 2.0);
        score_intermol += VinaClassic::coeff_gauss2 * gauss2;

        const double repuls = vinaRepulsionComponent(distance, 0.0);
        score_intermol += VinaClassic::coeff_repulsion * repuls;

        const double hydrophobic = vinaHydrophobicComponent(distance,
                                                            atom1AtomicNumber, atom1AtomVariant,
                                                            atom2AtomicNumber, atom2AtomVariant);
        score_intermol += VinaClassic::coeff_hydrophobic * hydrophobic;


        const double hydrogen = vinaHydrogenComponent(distance,
                                                      atom1AtomicNumber, atom1AtomVariant,
                                                      atom2AtomicNumber, atom2AtomVariant);
        score_intermol += VinaClassic::coeff_hydrogen * hydrogen;

        return score_intermol;
    }


}


#endif //SMOLDOCK_VINALIKECOMMON_H
