////////////////////////////////////////////////////////////////////////
/// \file   DrawSkewHits_tool.cc
/// \author T. Usher
////////////////////////////////////////////////////////////////////////

#include <cmath>
#include "lareventdisplay/EventDisplay/wfHitDrawers/IWFHitDrawer.h"
#include "lareventdisplay/EventDisplay/RawDrawingOptions.h"
#include "lareventdisplay/EventDisplay/RecoDrawingOptions.h"
#include "larcore/Geometry/Geometry.h"
#include "art/Utilities/ToolMacros.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "nutools/EventDisplayBase/EventHolder.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib_except/exception.h"

#include "lardataobj/RecoBase/Hit.h"
#include "lardata/ArtDataHelper/MVAReader.h"

#include "TPolyLine.h"

#include <fstream>

namespace evdb_tool
{

class DrawSkewHits : public IWFHitDrawer
{
public:
    explicit DrawSkewHits(const fhicl::ParameterSet& pset);
    
    ~DrawSkewHits();
    
    void configure(const fhicl::ParameterSet& pset)                                       override;
    void Draw(evdb::View2D&, std::vector<std::unique_ptr<TF1>>&, raw::ChannelID_t&) const override;
    
private:
    
    mutable std::vector<TPolyLine*> fPolyLineVec;
};
    
//----------------------------------------------------------------------
// Constructor.
DrawSkewHits::DrawSkewHits(const fhicl::ParameterSet& pset)
{
    configure(pset);
}
    
DrawSkewHits::~DrawSkewHits()
{
}
    
void DrawSkewHits::configure(const fhicl::ParameterSet& pset)
{
    return;
}

    
void DrawSkewHits::Draw(evdb::View2D&                      view2D,
                        std::vector<std::unique_ptr<TF1>>& hitFuncVec,
                        raw::ChannelID_t&                  channel) const
{
    art::ServiceHandle<evd::RawDrawingOptions>  rawOpt;
    art::ServiceHandle<evd::RecoDrawingOptions> recoOpt;
    art::ServiceHandle<geo::Geometry>            geo;
    
    //grab the singleton with the event
    const art::Event* event = evdb::EventHolder::Instance()->GetEvent();
    if(!event) return;

    fPolyLineVec.clear();

    for (size_t imod = 0; imod < recoOpt->fHitLabels.size(); ++imod)
    {
        art::InputTag const which = recoOpt->fHitLabels[imod];
        
        art::Handle< std::vector<recob::Hit> > hitVecHandle;
        event->getByLabel(which, hitVecHandle);
        
        // Get a container for the subset of hits we are working with and setup to fill it
        art::PtrVector<recob::Hit> hitPtrVec;
        bool                       stillSearching(true);
        int                        fitParamsOffset(0);
        
        // Loop through all hits and find those on the channel in question, also count up to the start of these hits
        for(size_t hitIdx = 0; hitIdx < hitVecHandle->size(); hitIdx++)
        {
            art::Ptr<recob::Hit> hit(hitVecHandle, hitIdx);
            
            if (hit->Channel() == channel)
            {
                hitPtrVec.push_back(hit);
                stillSearching = false;
            }
            else if (!stillSearching) break;
            
            if (stillSearching) fitParamsOffset++;
        }
        
        // No hits no work
        if (hitPtrVec.empty()) continue;

        // The fit parameters will be returned in an auxiliary object
        auto        hitResults   = anab::FVectorReader<recob::Hit, 4>::create(*event, "dprawhit");
        const auto& fitParamVecs = hitResults->vectors();
        
        // Containers for the piecess...
        std::vector<double> hitPeakTimeVec;
        std::vector<double> hitTau1Vec;
        std::vector<double> hitTau2Vec;
        std::vector<double> hitPeakAmpVec;
        std::vector<int>    hitStartTVec;
        std::vector<int>    hitEndTVec;
        std::vector<int>    hitNMultiHitVec;
        std::vector<int>    hitLocalIdxVec;
        
        // Ok, loop through the hits for this channnel and recover the parameters
        for (size_t idx = 0; idx < hitPtrVec.size(); ++idx)
        {
            const auto& fitParams = fitParamVecs[fitParamsOffset + idx];
            const auto& hit       = hitPtrVec[idx];
            
            hitPeakTimeVec.push_back(fitParams[0]);
            hitTau1Vec.push_back(fitParams[1]);
            hitTau2Vec.push_back(fitParams[2]);
            hitPeakAmpVec.push_back(fitParams[3]);
            hitStartTVec.push_back(hit->StartTick());
            hitEndTVec.push_back(hit->EndTick());
            hitNMultiHitVec.push_back(hit->Multiplicity());
            hitLocalIdxVec.push_back(hit->LocalIndex());
        }
        
        // Now we can go through these and start filling the polylines
        for(size_t idx = 0; idx < hitPeakTimeVec.size(); idx++)
        {
            if (hitNMultiHitVec[idx] > 1 && hitLocalIdxVec[idx] == 0)
            {
                TPolyLine& p2 = view2D.AddPolyLine(1001,kRed,3,1);
                
                // set coordinates of TPolyLine based fitted function
                for(int j = 0; j<1001; ++j)
                {
                    double x = hitStartTVec[idx] + j * (hitEndTVec[idx+hitNMultiHitVec[idx]-1]-hitStartTVec[idx])/1000;
                    double y = 0.; //RecoBaseDraw()->EvalMultiExpoFit(x,i,hNMultiHit[i],htau1,htau2,hamplitudes,hpeaktimes);
                    p2.SetPoint(j, x, y);
                }
                
                fPolyLineVec.push_back(&p2);
            }
            
            // Always draw the single peaks in addition to the sum of all peaks
            // create TPolyLine that actually gets drawn
            TPolyLine& p1 = view2D.AddPolyLine(1001,kOrange+7,3,1);
            
            // set coordinates of TPolyLine based fitted function
            for(int j = 0; j<1001; ++j){
                double x = hitStartTVec[idx - hitLocalIdxVec[idx]] + j * (hitEndTVec[idx + hitNMultiHitVec[idx] - hitLocalIdxVec[idx] - 1] - hitStartTVec[idx - hitLocalIdxVec[idx]]) / 1000;
                double y = 0.; //RecoBaseDraw()->EvalExpoFit(x,htau1[i],htau2[i],hamplitudes[i],hpeaktimes[i]);
                p1.SetPoint(j, x, y);
            }
            
            fPolyLineVec.push_back(&p1);
            
//            p1.Draw("same");
        }
    }

    return;
}

DEFINE_ART_CLASS_TOOL(DrawSkewHits)
}
