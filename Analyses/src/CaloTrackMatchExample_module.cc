 //
// An EDAnalyzer module that reads back the hits created by the calorimeter and produces an ntuple
//
// $Id: CaloTrackMatchExample_module.cc,v 1.4 2014/08/01 20:57:44 echenard Exp $
// $Author: echenard $
// $Date: 2014/08/01 20:57:44 $
//
// Original author Bertrand Echenard
//

#include "CLHEP/Units/SystemOfUnits.h"
#include "ConditionsService/inc/GlobalConstantsHandle.hh"
#include "ConditionsService/inc/ParticleDataTable.hh"
#include "ConditionsService/inc/unknownPDGIdName.hh"

#include "CalorimeterGeom/inc/Calorimeter.hh"

#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/GeometryService.hh"

#include "KalmanTests/inc/KalRepCollection.hh"
#include "KalmanTests/inc/TrkFitDirection.hh"
#include "KalmanTests/inc/KalFitMC.hh"
#include "KalmanTrack/KalRep.hh"
#include "KalmanTests/inc/KalRepPtrCollection.hh"


#include "RecoDataProducts/inc/CaloCrystalHitCollection.hh"
#include "RecoDataProducts/inc/CaloHitCollection.hh"
#include "RecoDataProducts/inc/CaloHit.hh"
#include "RecoDataProducts/inc/CaloCluster.hh"
#include "RecoDataProducts/inc/CaloClusterCollection.hh"
#include "RecoDataProducts/inc/CaloClusterCollection.hh"
#include "RecoDataProducts/inc/TrkCaloIntersectCollection.hh"
#include "RecoDataProducts/inc/TrkCaloMatchCollection.hh"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Selector.h"
#include "art/Framework/Principal/Provenance.h"
#include "cetlib/exception.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art/Utilities/InputTag.h"

#include "TDirectory.h"
#include "TNtuple.h"
#include "TTree.h"

#include <cmath>
#include <iostream>
#include <string>
#include <map>
#include <vector>




using namespace std;
using CLHEP::Hep3Vector;
using CLHEP::keV;



namespace mu2e {


  class CaloTrackMatchExample : public art::EDAnalyzer {
     
     public:


       explicit CaloTrackMatchExample(fhicl::ParameterSet const& pset);
       virtual ~CaloTrackMatchExample() { }

       virtual void beginJob();
       virtual void endJob();

       // This is called for each event.
       virtual void analyze(const art::Event& e);

       



     private:
       
       int findBestCluster(TrkCaloMatchCollection const& trkCaloMatches, int trkId, double maxChi2);
       int findBestTrack(  TrkCaloMatchCollection const& trkCaloMatches, int cluId, double maxChi2);

       int _diagLevel;
       int _nProcess;

       std::string      _caloCrystalModuleLabel;
       std::string      _caloClusterModuleLabel;
       std::string      _trkCaloMatchModuleLabel;
       std::string      _trkFitterModuleLabel;
       std::string      _trkfitInstanceName;
       TrkParticle      _tpart;
       TrkFitDirection  _fdir;
       double           _maxChi2Match;
       




       TTree* _Ntup;

       int   _evt,_run;

       int   _nHits,_cryId[16384],_crySectionId[16384],_crySimIdx[16384],_crySimLen[16384];
       float _cryEtot,_cryTime[16384],_cryEdep[16384],_cryDose[16384],_cryPosX[16384],_cryPosY[16384],_cryPosZ[16384],_cryLeak[16384];
              
       int   _nCluster,_cluNcrys[16384];
       float _cluEnergy[16384],_cluTime[16384],_cluCogX[16384],_cluCogY[16384],_cluCogZ[16384];
       std::vector<std::vector<int> > _cluList;	 

       int   _nTrk,_trkCluIdx[8192];


  };
 

  CaloTrackMatchExample::CaloTrackMatchExample(fhicl::ParameterSet const& pset) :
    art::EDAnalyzer(pset),
    _diagLevel(pset.get<int>("diagLevel",0)),
    _nProcess(0),
    _caloCrystalModuleLabel(pset.get<std::string>("caloCrystalModuleLabel")),
    _caloClusterModuleLabel(pset.get<std::string>("caloClusterModuleLabel")),
    _trkCaloMatchModuleLabel(pset.get<std::string>("trkCaloMatchModuleLabel")),
    _trkFitterModuleLabel(pset.get<std::string>("fitterModuleLabel")),
    _tpart((TrkParticle::type)(pset.get<int>("fitparticle"))),
    _fdir((TrkFitDirection::FitDirection)(pset.get<int>("fitdirection"))),
    _maxChi2Match(pset.get<double>("maxChi2Match")),
    _Ntup(0)

  {
    _trkfitInstanceName = _fdir.name() + _tpart.name();
  }

  void CaloTrackMatchExample::beginJob(){

    art::ServiceHandle<art::TFileService> tfs;

    _Ntup  = tfs->make<TTree>("CaloTrackMatch", "CaloTrackMatch");



    _Ntup->Branch("evt",          &_evt ,        "evt/I");
    _Ntup->Branch("run",          &_run ,        "run/I");

    _Ntup->Branch("nCry",         &_nHits ,       "nCry/I");
    _Ntup->Branch("cryId",        &_cryId ,       "cryId[nCry]/I");
    _Ntup->Branch("crySectionId", &_crySectionId, "crySectionId[nCry]/I");
    _Ntup->Branch("cryPosX",      &_cryPosX ,     "cryPosX[nCry]/F");
    _Ntup->Branch("cryPosY",      &_cryPosY ,     "cryPosY[nCry]/F");
    _Ntup->Branch("cryPosZ",      &_cryPosZ ,     "cryPosZ[nCry]/F");
    _Ntup->Branch("cryEdep",      &_cryEdep ,     "cryEdep[nCry]/F");
    _Ntup->Branch("cryTime",      &_cryTime ,     "cryTime[nCry]/F");

    _Ntup->Branch("nCluster",     &_nCluster ,    "nCluster/I");
    _Ntup->Branch("cluEnergy",    &_cluEnergy ,   "cluEnergy[nCluster]/F");
    _Ntup->Branch("cluTime",      &_cluTime ,     "cluTime[nCluster]/F");
    _Ntup->Branch("cluCogX",      &_cluCogX ,     "cluCogX[nCluster]/F");	
    _Ntup->Branch("cluCogY",      &_cluCogY ,     "cluCogY[nCluster]/F");	
    _Ntup->Branch("cluCogZ",      &_cluCogZ ,     "cluCogZ[nCluster]/F");	
    _Ntup->Branch("cluNcrys",     &_cluNcrys ,    "cluNcrys[nCluster]/I");	
    _Ntup->Branch("cluList",      &_cluList);

    _Ntup->Branch("nTrk",         &_nTrk ,        "nTrk/I");
    _Ntup->Branch("trkCluIdx",    &_trkCluIdx,    "trkCluIdx[nTrk]/I");
   

  }

	

  void CaloTrackMatchExample::endJob(){
  }




  void CaloTrackMatchExample::analyze(const art::Event& event) {

      ++_nProcess;
      if (_diagLevel && _nProcess%10==0) std::cout<<"Processing event from CaloTrackMatchExample =  "<<_nProcess <<std::endl;
      
 
      // Get handle to the calorimeter
      art::ServiceHandle<GeometryService> geom;
      if( ! geom->hasElement<Calorimeter>() ) return;
      Calorimeter const & cal = *(GeomHandle<Calorimeter>());
  

      // Get crystal hits
      art::Handle<CaloCrystalHitCollection> caloCrystalHitsHandle;
      event.getByLabel(_caloCrystalModuleLabel, caloCrystalHitsHandle);
      CaloCrystalHitCollection const& caloCrystalHits(*caloCrystalHitsHandle);

      // Get tracks
      art::Handle<KalRepPtrCollection> trksHandle;
      event.getByLabel(_trkFitterModuleLabel,_trkfitInstanceName,trksHandle);
      KalRepPtrCollection const& trksPtrColl(*trksHandle);

      // Get clusters
      art::Handle<CaloClusterCollection> caloClustersHandle;
      event.getByLabel(_caloClusterModuleLabel, caloClustersHandle);
      CaloClusterCollection const& caloClusters(*caloClustersHandle);

      // Get track calorimeter matches
      art::Handle<TrkCaloMatchCollection>  trkCaloMatchHandle;
      event.getByLabel(_trkCaloMatchModuleLabel, trkCaloMatchHandle);
      TrkCaloMatchCollection const& trkCaloMatches(*trkCaloMatchHandle);

 







       //--------------------------  Do generated particles --------------------------------
       _evt = event.id().event();
       _run = event.run();
              
     
       //--------------------------  Do calorimeter hits --------------------------------
      
       _nHits = 0;
       for (unsigned int ic=0; ic<caloCrystalHits.size();++ic) 
       {	   
	   CaloCrystalHit const& hit    = caloCrystalHits.at(ic);
	   CLHEP::Hep3Vector crystalPos = cal.crystalOrigin(hit.id());

	   _cryTime[_nHits]      = hit.time();
	   _cryEdep[_nHits]      = hit.energyDep();
	   _cryPosX[_nHits]      = crystalPos.x();
	   _cryPosY[_nHits]      = crystalPos.y();
	   _cryPosZ[_nHits]      = crystalPos.z();
	   _cryId[_nHits]        = hit.id();
	   _crySectionId[_nHits] = cal.crystal(hit.id()).sectionId();
           ++_nHits;
       }
      
       //--------------------------  Dump cluster info --------------------------------
       _nCluster = 0;
       _cluList.clear();
       for (CaloClusterCollection::const_iterator clusterIt = caloClusters.begin(); clusterIt != caloClusters.end(); ++clusterIt) 
       {
	  std::vector<int> _list;
	  for (int i=0;i<clusterIt->size();++i) 
	     _list.push_back( int(clusterIt->caloCrystalHitsPtrVector().at(i).get()- &caloCrystalHits.at(0)) );	    

	  _cluEnergy[_nCluster] = clusterIt->energyDep();
	  _cluTime[_nCluster]   = clusterIt->time();
	  _cluNcrys[_nCluster]  = clusterIt->size();
          _cluCogX[_nCluster]   = clusterIt->cog3Vector().x();
          _cluCogY[_nCluster]   = clusterIt->cog3Vector().y();
          _cluCogZ[_nCluster]   = clusterIt->cog3Vector().z();
	  _cluList.push_back(_list);

	  ++_nCluster;
       }
       

       //--------------------------  Do tracks  --------------------------------
       // we just want the cluster that matches the track. 
       // For information about the track at the entrance of the calorimeter, look at the 
       // Analysis/src/ReadTrackCaloMatching_module.cc mdoule

       _nTrk = 0;
       for (unsigned int i=0;i< trksPtrColl.size(); ++i)
       {         
	 //KalRepPtr const& trkPtr = trksPtrColl.at(i);
	 _trkCluIdx[_nTrk] = findBestCluster(trkCaloMatches,i,_maxChi2Match);
	 ++_nTrk;

        }

  	_Ntup->Fill();
  




  }

   int CaloTrackMatchExample::findBestCluster(TrkCaloMatchCollection const& trkCaloMatches, int trkId, double maxChi2)
   {
      double chi2Best(maxChi2), cluBest(-1);
      for (auto const& trkCaloMatch: trkCaloMatches)
      {
	 if (trkCaloMatch.trkId()==trkId && trkCaloMatch.chi2() < chi2Best)
	 {
           chi2Best= trkCaloMatch.chi2(); 
	   cluBest = trkCaloMatch.cluId();
	 } 
      }
      return cluBest;
   }

   int CaloTrackMatchExample::findBestTrack(TrkCaloMatchCollection const& trkCaloMatches, int cluId, double maxChi2)
   {
      double chi2Best(maxChi2), trkBest(-1);
      for (auto const& trkCaloMatch: trkCaloMatches)
      {
	 if (trkCaloMatch.cluId()==cluId && trkCaloMatch.chi2() < chi2Best)
	 {
           chi2Best= trkCaloMatch.chi2(); 
	   trkBest = trkCaloMatch.trkId();
	 } 
      }
      return trkBest;
   }


}  

DEFINE_ART_MODULE(mu2e::CaloTrackMatchExample);

