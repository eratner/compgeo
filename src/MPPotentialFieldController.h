//
//  MPPotentialFieldPlanner.h
//
//  Created by Ellis Ratner on 5/1/14.
//

#ifndef __MPPotentialFieldPlanner__
#define __MPPotentialFieldPlanner__

#include <iostream>
#include <map>
#include "MPModel.h"

namespace MP
{

class PotentialFieldController
{
public:
    PotentialFieldController(const std::vector<Model *> &obstacles, Model *activeObject, float voxelSize);
    
    PotentialFieldController();
    
    void setGoal(const Transform3D &goal);
    
    void setGradStep(float gradStep) { gradStep_ = gradStep; }
    
    void setAttractiveMultiplier(float multiplier) { attractiveMultiplier_ = multiplier; }
    
    void setRepulsiveMultiplier(float multiplier) { repulsiveMultiplier_ = multiplier; }
    
    void move() const;
    
private:
    /* Potential functions inspired by http://www.cs.cmu.edu/~motionplanning/lecture/Chap4-Potential-Field_howie.pdf */
    MPVec3 potentialGrad(const MPVec3 &p) const;
    
    MPVec3 attractivePotentialGrad(const MPVec3 &p) const;
    
    /* pObs is the position of the obstacle
     * p is the position of the moving object
     * P is the cutoff beyond which there is no repuslive effect */
    MPVec3 repulsivePotentialGrad(const MPVec3 &pObs, const MPVec3 &p, float r, float P) const;
    
    float voxelSize_;
    
    /* all default to 1.0 */
    float gradStep_;
    float attractiveMultiplier_;
    float repulsiveMultiplier_;
    
    std::map<Model *, std::vector<MPVec3>> obstacles_;
    
    Model *activeObject_;
    Transform3D goal_;
};
    
}

#endif
