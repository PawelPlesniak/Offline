//
// Geometry and identifier info about the Calorimeter.
//
// Original author B. Echenard
//


#include "CalorimeterGeom/inc/CaloGeomUtil.hh"
#include "CalorimeterGeom/inc/Disk.hh"
#include "CalorimeterGeom/inc/Crystal.hh"

#include "CLHEP/Vector/Rotation.h"
#include "CLHEP/Vector/ThreeVector.h"


namespace mu2e {


    CaloGeomUtil::CaloGeomUtil(const std::vector<std::shared_ptr<Disk>>& disks, 
                               const CaloInfo& caloInfo,
                               const CaloGeomInfo& geomInfo,
                               const std::vector<Crystal const*>& fullCrystalList) : 
      disks_(disks),
      caloInfo_(caloInfo),
      geomInfo_(geomInfo),
      fullCrystalList_(fullCrystalList)
    {}


    CLHEP::Hep3Vector CaloGeomUtil::mu2eToCrystal(int crystalId, const CLHEP::Hep3Vector &pos) const 
    {   
        const Disk& thisDisk = disk( fullCrystalList_.at(crystalId)->diskId());
        CLHEP::Hep3Vector crysLocalPos = fullCrystalList_.at(crystalId)->localPosition();
        return thisDisk.geomInfo().rotation()*(pos-thisDisk.geomInfo().origin())-crysLocalPos;  
    }

    CLHEP::Hep3Vector CaloGeomUtil::mu2eToDisk(int diskId, const CLHEP::Hep3Vector &pos) const 
    {   
        const Disk& thisDisk = disk(diskId);
        return (thisDisk.geomInfo().rotation())*(pos-thisDisk.geomInfo().origin());
    }

    CLHEP::Hep3Vector CaloGeomUtil::mu2eToDiskFF(int diskId, const CLHEP::Hep3Vector &pos) const 
    {   
        const Disk& thisDisk = disk(diskId);
        return (thisDisk.geomInfo().rotation())*(pos-thisDisk.geomInfo().origin()) - thisDisk.geomInfo().originToCrystalOrigin();
    }

    CLHEP::Hep3Vector CaloGeomUtil::mu2eToTracker(const CLHEP::Hep3Vector &pos) const 
    {   
        return pos - geomInfo_.trackerCenter();
    }


    CLHEP::Hep3Vector CaloGeomUtil::crystalToMu2e(int crystalId, const CLHEP::Hep3Vector &pos) const 
    {   
        const Disk& thisDisk = disk( fullCrystalList_.at(crystalId)->diskId() );
        CLHEP::Hep3Vector crysLocalPos = fullCrystalList_.at(crystalId)->localPosition();
        return thisDisk.geomInfo().inverseRotation()*(pos+crysLocalPos) + thisDisk.geomInfo().origin();  
    }

    CLHEP::Hep3Vector CaloGeomUtil::diskToMu2e(int diskId, const CLHEP::Hep3Vector &pos) const 
    {   
        const Disk& thisDisk = disk(diskId);
        return thisDisk.geomInfo().inverseRotation()*pos + thisDisk.geomInfo().origin();
    }

    CLHEP::Hep3Vector CaloGeomUtil::diskFFToMu2e(int diskId, const CLHEP::Hep3Vector &pos) const 
    {   
        const Disk& thisDisk = disk(diskId);
        return thisDisk.geomInfo().inverseRotation()*(pos + thisDisk.geomInfo().originToCrystalOrigin()) 
               + thisDisk.geomInfo().origin();
    }

    CLHEP::Hep3Vector CaloGeomUtil::trackerToMu2e(const CLHEP::Hep3Vector &pos) const 
    {   
        return pos + geomInfo_.trackerCenter();
    }


   bool CaloGeomUtil::isInsideCalorimeter(const CLHEP::Hep3Vector &pos) const 
   {   
       for (unsigned int idisk=0;idisk<disks_.size();++idisk) if (isInsideSection(idisk, pos)) return true;	
       return false;    
   }

   bool CaloGeomUtil::isInsideSection(int idisk, const CLHEP::Hep3Vector &pos) const 
   {   
	CLHEP::Hep3Vector posInSection = mu2eToDiskFF(idisk, pos);
	double posZ = posInSection.z();
	double zlim = 2*caloInfo_.crystalHalfLength()+1e-6;

	if ( posZ < -1e-6 || posZ > zlim )                                          return false;
	if ( disk(idisk).idxFromPosition(posInSection.x(),posInSection.y()) == -1)  return false;	

	return true;
    }
            
    bool CaloGeomUtil::isContainedSection(const CLHEP::Hep3Vector &front, const CLHEP::Hep3Vector &back) const 
    {   
	//for (unsigned int idisk=0;idisk<disks_.size();++idisk) 
	//   if (isInsideSection(idisk,start) && isInsideSection(idisk,stop)) return true;
	//return false;

	//this is a more efficient way of doing this in the case of two disks
	for (unsigned int idisk=0;idisk<disks_.size();++idisk) 
	{
	  if ( abs(front.z()-back.z()) < (*disks_.at(idisk)).geomInfo().size().z() ) return true;
	}
	return false;   
    }
	 


}

