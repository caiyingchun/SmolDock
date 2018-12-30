//
// Created by eliane on 29/12/18.
//

#include "PDBResidueVariantTable.h"


namespace SmolDock {


    // PDB has specific, IUPAC compliant atom name for atoms in proteinogenic amino acid
    // eg : ATOM      5  CB  LEU A 353      12.418  28.143  21.455  1.00 27.35           C
    //                   ^^
    // Here, we use these name to assign the relevant AtomVariant flags
    // It is basically how far the atom is to the backbone (A : alpha, B : beta, D,G,E,Z,H)
    // Search IUPAC amino acid nomenclature on the web to get nice drawings of which is which

    std::set<std::tuple<AminoAcid::AAType, std::vector<std::tuple<std::string, Atom::AtomVariant> > > > ResidueAtomPropertiesLookupTable_General = {
            {AminoAcid::AAType::alanine,        {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar}}},
            {AminoAcid::AAType::arginine,       {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG",  Atom::AtomVariant::apolar},
                                                        {"CD",  Atom::AtomVariant{0}},
                                                        {"NE",  Atom::AtomVariant{0}},
                                                        {"CZ",  Atom::AtomVariant{0}},
                                                        {"NH1", Atom::AtomVariant{0}},
                                                        {"NH2", Atom::AtomVariant{0}}}},
            {AminoAcid::AAType::asparagine,     {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG",  Atom::AtomVariant{0}},
                                                        {"OD1", Atom::AtomVariant{0}},
                                                        {"ND2", Atom::AtomVariant{0}}}},
            {AminoAcid::AAType::aspartate,      {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG",  Atom::AtomVariant{0}},
                                                        {"OD1", Atom::AtomVariant{0}},
                                                        {"OD2", Atom::AtomVariant{0}}}},
            {AminoAcid::AAType::cysteine,       {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant{0}},
                                                        {"SG",  Atom::AtomVariant{0}}}},
            {AminoAcid::AAType::glutamate,      {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG",  Atom::AtomVariant::apolar},
                                                        {"CD",  Atom::AtomVariant{0}},
                                                        {"OE1", Atom::AtomVariant{0}},
                                                        {"OE2", Atom::AtomVariant{0}}}},
            {AminoAcid::AAType::glutamine,      {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG",  Atom::AtomVariant::apolar},
                                                        {"CD",  Atom::AtomVariant{0}},
                                                        {"OE1", Atom::AtomVariant{0}},
                                                        {"NE2", Atom::AtomVariant{0}}}},
            {AminoAcid::AAType::glycine,        {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}}}},
            {AminoAcid::AAType::histidine,      {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG",  Atom::AtomVariant{0}},
                                                        {"ND2", Atom::AtomVariant{0}},
                                                        {"CD2", Atom::AtomVariant{0}},
                                                        {"CE2", Atom::AtomVariant{0}},
                                                        {"NE2", Atom::AtomVariant{0}},}},
            {AminoAcid::AAType::isoleucine,     {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG1", Atom::AtomVariant::apolar},
                                                        {"CG2", Atom::AtomVariant::apolar},
                                                        {"CD1", Atom::AtomVariant::apolar}}},
            {AminoAcid::AAType::leucine,        {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG",  Atom::AtomVariant::apolar},
                                                        {"CD1", Atom::AtomVariant::apolar},
                                                        {"CD2", Atom::AtomVariant::apolar}}},
            {AminoAcid::AAType::lysine,         {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG",  Atom::AtomVariant::apolar},
                                                        {"CD",  Atom::AtomVariant::apolar},
                                                        {"CE",  Atom::AtomVariant{0}},
                                                        {"NZ",  Atom::AtomVariant{0}}}},
            {AminoAcid::AAType::methionine,     {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG",  Atom::AtomVariant{0}},
                                                        {"SD",  Atom::AtomVariant{0}},
                                                        {"CE",  Atom::AtomVariant{0}}}},
            {AminoAcid::AAType::phenylalanine,  {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG",  Atom::AtomVariant::apolar},
                                                        {"CD1", Atom::AtomVariant::apolar},
                                                        {"CD2", Atom::AtomVariant::apolar},
                                                        {"CE1", Atom::AtomVariant::apolar},
                                                        {"CE2", Atom::AtomVariant::apolar},
                                                        {"CZ",  Atom::AtomVariant::apolar}}},
            {AminoAcid::AAType::proline,        {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG",  Atom::AtomVariant::apolar}}},
            {AminoAcid::AAType::serine,         {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant{0}},
                                                        {"OG",  Atom::AtomVariant{0}}}},
            {AminoAcid::AAType::threonine,      {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant{0}},
                                                        {"OG1", Atom::AtomVariant{0}},
                                                        {"CG2", Atom::AtomVariant{0}}}},
            {AminoAcid::AAType::tryptophan,     {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG",  Atom::AtomVariant::apolar},
                                                        {"CD1", Atom::AtomVariant{0}},
                                                        {"CD2", Atom::AtomVariant::apolar},
                                                        {"NE1", Atom::AtomVariant::apolar},
                                                        {"CE2", Atom::AtomVariant{0}},
                                                        {"CE3", Atom::AtomVariant::apolar},
                                                        {"CZ2", Atom::AtomVariant::apolar},
                                                        {"CZ3", Atom::AtomVariant::apolar},
                                                        {"CH2", Atom::AtomVariant::apolar}}},
            {AminoAcid::AAType::tyrosine,       {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG",  Atom::AtomVariant::apolar},
                                                        {"CD1", Atom::AtomVariant::apolar},
                                                        {"CD2", Atom::AtomVariant::apolar},
                                                        {"CE1", Atom::AtomVariant::apolar},
                                                        {"CE2", Atom::AtomVariant::apolar},
                                                        {"CZ",  Atom::AtomVariant{0}},
                                                        {"OH",  Atom::AtomVariant{0}}}},
            {AminoAcid::AAType::valine,         {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant::apolar},
                                                        {"CG1", Atom::AtomVariant::apolar},
                                                        {"CG2", Atom::AtomVariant::apolar},}},
            {AminoAcid::AAType::selenocysteine, {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}},
                                                        {"CB", Atom::AtomVariant{0}},
                                                        {"SEG", Atom::AtomVariant{0}}}},
            {AminoAcid::AAType::pyrrolysine,    {{"CA", Atom::AtomVariant{0}},
                                                        {"C", Atom::AtomVariant{0}},
                                                        {"N", Atom::AtomVariant{0}},
                                                        {"O", Atom::AtomVariant{0}}}}
    };


}