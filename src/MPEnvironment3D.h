//
//  MPEnvironment3D.h
//
//  Created by Ellis Ratner on 4/9/14.
//  Copyright (c) 2014 Ellis Ratner. All rights reserved.
//
//  This environment considers only (x, y, z) and no rotations. Furthermore, we assume 
//  integer coordinates, which may be scaled by the actual step size determined by the 
//  discretization of the space. For example, if the actual step size is 0.01 m, then 
//  we assume that 0.01 maps to 1, 0.02 maps to 2, and so on.

#ifndef _MPEnvironment3D_h
#define _MPEnvironment3D_h

#include "MPEnvironment.h"
#include "MPModel.h"

namespace MP
{
    
typedef enum
{
    EnvironmentPresetDefault = 0
} EnvironmentPreset;

typedef SearchState<Transform3D> SearchState3D;

class Environment3D : public Environment<Transform3D>
{
public:
  Environment3D(const MPVec3 &size);
  Environment3D(const MPVec3 &origin, const MPVec3 &size);
  Environment3D(EnvironmentPreset preset);
    
  ~Environment3D();

  void getSuccessors(SearchState3D *s, std::vector<SearchState3D *> &successors, std::vector<double> &costs);

  bool getCost(SearchState3D *s, SearchState3D *t, double &cost);

  void setOrigin(const MPVec3 &origin) { origin_ = origin; }

  MPVec3 getOrigin() const { return origin_; }

  void setSize(const MPVec3 &size) { size_ = size; }

  MPVec3 getSize() const { return size_; }
    
  void setActiveObject(Model *activeObject) { activeObject_ = activeObject; }
    
  Model* getActiveObject() const { return activeObject_; }
    
  void addObstacle(Model *obstacle) { obstacles_.push_back(obstacle); }
    
  const std::vector<Model *>& getObstacles() const { return obstacles_; }

private:
  MPVec3 origin_;
  MPVec3 size_;

  Model *activeObject_;
  std::vector<Model *> obstacles_;

};

}

#endif 
