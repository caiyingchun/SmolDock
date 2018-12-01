//
// Created by eliane on 25/11/18.
//

#include "MDStyleDockingEngine.h"


namespace SmolDock::Engine {


    bool MDStyleDockingEngine::setProtein(Protein *p) {
        this->protein = p;
        return true;
    }

    bool MDStyleDockingEngine::setLigand(Molecule *m) {
        this->ligand = m;
        return true;
    }

    bool MDStyleDockingEngine::setupDockingEngine() {
        return true;
    }

    void MDStyleDockingEngine::runDockingEngine() {

    }

    std::shared_ptr<DockingResult> MDStyleDockingEngine::getDockingResult() {
        return std::make_shared<DockingResult>();
    }


    bool MDStyleDockingEngine::setDockingBox(AbstractDockingEngine::DockingBoxSetting setting) {
        if (setting != DockingBoxSetting::everything) {
            std::cout << "[!] DockingBoxSetting (that is not DockingBoxSetting::everything) is not yet implemented."
                      << std::endl;
            std::cout << "[ ] Running as if DockingBoxSetting::everything was passed" << std::endl;
            return false;
        }
        return true;
    }
}