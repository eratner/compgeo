//
//  MPEnvironment3D.cpp
//
//  Created by Ellis Ratner on 4/9/14.
//  Copyright (c) 2014 Ellis Ratner. All rights reserved.
//

#include "MPEnvironment3D.h"
#include "MPTimer.h"
#include "MPUtils.h"

namespace MP
{

double distanceHeuristic(const Transform3D &start, const Transform3D &goal)
{
    return MPVec3EuclideanDistance(start.getPosition(), goal.getPosition());
}

double manhattanHeuristic(const Transform3D &start, const Transform3D &goal)
{
    MPQuaternion sRot = start.getRotation();
    MPQuaternion tRot = goal.getRotation();
    
    MPVec3 difference = MPVec3Subtract(start.getPosition(), goal.getPosition());
    
    float pitchDiff = sRot.x - tRot.x;
    float yawDiff = sRot.y - tRot.y;
    float rollDiff = sRot.z - tRot.z;
    
    return std::abs(difference.x) + std::abs(difference.y) + std::abs(difference.z) + std::abs(pitchDiff) + std::abs(yawDiff) + std::abs(rollDiff);
}

bool operator==(const Transform3D &lhs, const Transform3D &rhs)
{
    MPVec3 leftPos = lhs.getPosition();
    MPVec3 rightPos = rhs.getPosition();
    
    MPQuaternion leftRot = lhs.getRotation();
    MPQuaternion rightRot = rhs.getRotation();
    
    // Only consider the (x, y, z) projection of the transform
    // Also, assume that the coordinates are integer-valued
    return (((int)leftPos.x == (int)rightPos.x) &&
            ((int)leftPos.y == (int)rightPos.y) &&
            ((int)leftPos.z == (int)rightPos.z) &&
            // We store the pitch/yaw/roll as rotation xyz
            ((int)leftRot.x == (int)rightRot.x) &&
            ((int)leftRot.y == (int)rightRot.y) &&
            ((int)leftRot.z == (int)rightRot.z));
}

int transform3DHash(const Transform3D &t)
{
    const int p1 = 73856093;
    const int p2 = 19349663;
    const int p3 = 83492791;
    const int p4 = 3331333;
    const int p5 = 393919;
    const int p6 = 39916801;
    
    MPVec3 pos = t.getPosition();
    MPQuaternion rot = t.getRotation();
    
    // Assume that the coordinates are integer-valued
    int x = (int)pos.x;
    int y = (int)pos.y;
    int z = (int)pos.z;
    
    int pitch = (int)rot.x;
    int yaw = (int)rot.y;
    int roll = (int)rot.z;
    
    return ((x*p1) ^ (y*p2) ^ (z*p3) ^ (pitch*p4) ^ (yaw*p5) ^ (roll*p6));
}

Environment3D::Environment3D()
: Environment<Transform3D>(transform3DHash), origin_(MPVec3Zero), size_(MPVec3Make(1.0f, 1.0f, 1.0f)), activeObject_(nullptr), dynamic_(false)
{
}

Environment3D::Environment3D(const MPVec3 &size)
: Environment<Transform3D>(transform3DHash), origin_(MPVec3Zero), size_(size), activeObject_(nullptr), dynamic_(false)
{
}

Environment3D::Environment3D(const MPVec3 &origin, const MPVec3 &size)
: Environment<Transform3D>(transform3DHash), origin_(origin), size_(size), activeObject_(nullptr), dynamic_(false)
{
}

Environment3D::~Environment3D()
{
}
    
void Environment3D::setOrigin(const MPVec3 &origin)
{
    this->origin_ = origin;
    this->updateBoundingBox();
}
    
void Environment3D::setSize(const MPVec3 &size)
{
    this->size_ = size;
    this->updateBoundingBox();
}
    
void Environment3D::setRotationStepSize(double s)
{
    this->rotationStepSize_ = s;
    this->numRotations_ = 2.0f * M_PI / s;
}
    
void Environment3D::setActiveObject(MP::Model *activeObject)
{
    activeObject_ = activeObject;

    actionSet_.clear();
}

void Environment3D::getSuccessors(SearchState3D *s,
                                  std::vector<SearchState3D *> &successors,
                                  std::vector<double> &costs)
{
    if(actionSet_.empty())
        generateActionSet();
    
    Transform3D sT = s->getValue();
    
    if(states_.get(sT) == nullptr)
        states_.insert(s);
    
//    Timer timer;
//    timer.start();
    
    for(auto action : actionSet_)
    {
        // Generate the successor transform by applying an action to the current state
        Transform3D T = sT;
        applyAction(action, T);
        
        SearchState3D *neighbor = states_.get(T);
        if(neighbor == nullptr)
        {
            // Has not seen this state yet
            neighbor = new SearchState3D();
            neighbor->setValue(T);
            states_.insert(neighbor);
        }
        
        successors.push_back(neighbor);
        costs.push_back(action.getCost());
    }
    
//    std::cout << "Successor generation took "
//    << GET_ELAPSED_MICRO(timer) << " microseconds" << std::endl;
}

bool Environment3D::getCost(SearchState3D *s, SearchState3D *t, double &cost)
{
    MPQuaternion sRot = s->getValue().getRotation();
    MPQuaternion tRot = t->getValue().getRotation();
    
    MPVec3 difference = MPVec3Subtract(s->getValue().getPosition(), t->getValue().getPosition());
    
    float pitchDiff = sRot.x - tRot.x;
    float yawDiff = sRot.y - tRot.y;
    float rollDiff = sRot.z - tRot.z;
    
    cost = std::abs(difference.x) + std::abs(difference.y) + std::abs(difference.z) + (std::abs(pitchDiff) + std::abs(yawDiff) + std::abs(rollDiff));
    
    return true;
}

bool Environment3D::stateValid(const Transform3D &T)
{
    if(!Environment<Transform3D>::stateValid(T)) return false;
    
    Transform3D worldT = this->plannerToWorld(T);
    
    if(!this->isValid(worldT))
    {
        SearchState3D *s = new SearchState3D();
        s->setValue(T);
        invalidStates_.insert(s);
        return false;
    }
    
    return true;
}

bool Environment3D::isValid(Transform3D &T) const
{
    return this->isValidForModel(T, this->activeObject_);
}

bool Environment3D::isValidForModel(Transform3D &T, Model *model) const
{
//    Timer timer;
//    timer.start();
    
    bool valid = this->inBoundsForModel(T, model);
    
    for(auto it = obstacles_.begin(); valid && it != obstacles_.end(); ++it)
    {
        if(model->wouldCollideWithModel(T, **it))
        {
            valid = false;
        }
    }
    
//    std::cout << "Collision detection took "
//    << GET_ELAPSED_MICRO(timer) << " microseconds" << std::endl;
    
    return valid;
}
    
bool Environment3D::inBounds(Transform3D &T) const
{
    return this->inBoundsForModel(T, this->activeObject_);
}

bool Environment3D::inBoundsForModel(MP::Transform3D &T, MP::Model *model) const
{
    const MPVec3 *extremePoints = MPMeshGetExtremePoints(model->getMesh());
    
    bool inBounds = true;
    
    for (int i = 0; i < 6 && inBounds; ++i)
    {
        inBounds = MPAABoxContainsPoint(this->boundingBox_, T.transformVec3(extremePoints[i]));
    }
    
    return inBounds;
}
    
#pragma mark - world/planner conversions
    
void Environment3D::plannerToWorld(Transform3D &state) const
{
    MPVec3 wPos = state.getPosition();
    this->plannerToWorld(wPos);
    
    MPQuaternion wRot = state.getRotation();
    this->plannerToWorld(wRot);
    
    state.setPosition(wPos);
    state.setRotation(wRot);
}

Transform3D Environment3D::plannerToWorld(const Transform3D &state) const
{
    Transform3D worldState = state;
    
    plannerToWorld(worldState);
    
    return worldState;
}

void Environment3D::worldToPlanner(Transform3D &state) const
{
    MPVec3 pPos = state.getPosition();
    this->worldToPlanner(pPos);
    
    MPQuaternion pRot = state.getRotation();
    this->worldToPlanner(pRot);
    
    state.setPosition(pPos);
    state.setRotation(pRot);
}

Transform3D Environment3D::worldToPlanner(const Transform3D &state) const
{
    Transform3D plannerState = state;
    
    worldToPlanner(plannerState);
    
    return plannerState;
}
    
void Environment3D::plannerToWorld(MPVec3 &vec) const
{
    vec.x *= this->stepSize_;
    vec.y *= this->stepSize_;
    vec.z *= this->stepSize_;
}

void Environment3D::plannerToWorld(MPQuaternion &q) const
{
    q = MPRPYToQuaternion(q.z * this->rotationStepSize_, q.x * this->rotationStepSize_, q.y * this->rotationStepSize_);
}
    
void Environment3D::worldToPlanner(MPVec3 &vec) const
{
    vec.x = int(std::round(vec.x / this->stepSize_));
    vec.y = int(std::round(vec.y / this->stepSize_));
    vec.z = int(std::round(vec.z / this->stepSize_));
}

void Environment3D::worldToPlanner(MPQuaternion &q) const
{
    float r, p, y;
    MPQuaternionToRPY(q, &r, &p, &y);
    q.x = (int(std::round(p / this->rotationStepSize_)) + this->numRotations_) % this->numRotations_;
    q.y = (int(std::round(y / this->rotationStepSize_)) + this->numRotations_) % this->numRotations_;
    q.z = (int(std::round(r / this->rotationStepSize_)) + this->numRotations_) % this->numRotations_;
    q.w = 0.0f;
}
    
#pragma mark - protected methods
    
void Environment3D::updateBoundingBox()
{
    MPVec3 halfSize = MPVec3MultiplyScalar(this->size_, 0.5f);
    
    MPVec3 min = MPVec3Subtract(this->origin_, halfSize);
    MPVec3 max = MPVec3Add(this->origin_, halfSize);
    
    this->boundingBox_ = MPAABoxMake(min, max);    
}
    
void Environment3D::generateActionSet()
{
    actionSet_.clear();
    
    // Get the model-specific actions that are specified in world coordinates, and
    // convert them to planner coordinate actions
    for(auto action : activeObject_->getActionSet())
    {
        MPVec3 translation = action.getTranslation();
        this->worldToPlanner(translation);
        
        MPQuaternion rotation = action.getRotation();
        this->worldToPlanner(rotation);
       
        actionSet_.push_back(Action6D(action.getCost(), translation, rotation));
    }
}
    
void Environment3D::applyAction(const MP::Action6D &action, MP::Transform3D &stateTransform)
{
    MPQuaternion rot = action.getRotation();
    MPVec3 trans = action.getTranslation();
    
    MPVec3 p = stateTransform.getPosition();
    MPQuaternion q = stateTransform.getRotation();
    
    MPQuaternion worldQ = q;
    this->plannerToWorld(worldQ);
    
    this->plannerToWorld(trans);
    
    trans = MPQuaternionRotateVec3(worldQ, trans);
    
    this->worldToPlanner(trans);
    
    p.x += trans.x;
    p.y += trans.y;
    p.z += trans.z;
    
    q.x = (int(q.x + rot.x + this->numRotations_) % this->numRotations_);
    q.y = (int(q.y + rot.y + this->numRotations_) % this->numRotations_);
    q.z = (int(q.z + rot.z + this->numRotations_) % this->numRotations_);
    
    stateTransform.setPosition(p);
    stateTransform.setRotation(q);
}
    
}
