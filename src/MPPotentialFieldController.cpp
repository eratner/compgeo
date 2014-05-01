//
//  MPPotentialFieldPlanner.cpp
//
//  Created by Ellis Ratner on 5/1/14.
//

#include "MPPotentialFieldController.h"

namespace MP
{
PotentialFieldController::PotentialFieldController(const std::vector<Model *> &obstacles, Model *activeObject)
: obstacles_(obstacles), activeObject_(activeObject)
{
    
}

PotentialFieldController::PotentialFieldController()
{
}

void PotentialFieldController::setGoal(const Transform3D &goal)
{
    goal_ = goal;
}

void PotentialFieldController::move()
{
    // Perform a single step of gradient descent
    MPVec3 currentPosition = activeObject_->getPosition();
    MPVec3 gradient = potentialGrad(currentPosition);
    const float alpha = 0.6f;
    
    // TODO: Check for convergence (i.e. when gradient of potential function is close to zero
    // or, || gradient || < epsilon for some epsilon)
    MPVec3 nextPosition = MPVec3Subtract(currentPosition, MPVec3MultiplyScalar(gradient, alpha));
    
    activeObject_->setPosition(nextPosition);
}

MPVec3 PotentialFieldController::potentialGrad(const MPVec3 &p)
{
    // By superposition and linearity, the gradient of the potential at p is simply the
    // sum of the individual contributions from all the "particles"
    MPVec3 potentialGrad = attractivePotentialGrad(p);
    for(auto obstacle : obstacles_)
    {
        const MPMat4 matrix = obstacle->getTransform().getMatrix();
        MPSphere boundingSphere = MPMeshGetBoundingSphere(obstacle->getMesh(), &matrix);
        MPVec3Add(potentialGrad, repulsivePotentialGrad(obstacle->getPosition(), p, boundingSphere.radius));
    }
    
    return potentialGrad;
}

MPVec3 PotentialFieldController::attractivePotentialGrad(const MPVec3 &p)
{
    // TODO: Move these (arbitrary-defined) constants somewhere better
    const float c = 4.0f;
    const float threshold = 1.0f;
    
    float distance = MPVec3EuclideanDistance(p, goal_.getPosition());
    
    MPVec3 potentialGrad = MPVec3Zero;
    
    if(distance <= threshold)
    {
        potentialGrad = MPVec3Subtract(p, goal_.getPosition());
        potentialGrad = MPVec3MultiplyScalar(potentialGrad, c);
    }
    else
    {
        potentialGrad = MPVec3Subtract(p, goal_.getPosition());
        potentialGrad = MPVec3MultiplyScalar(potentialGrad, (threshold * c) / (distance));
    }
    
    return potentialGrad;
}

MPVec3 PotentialFieldController::repulsivePotentialGrad(const MPVec3 &pObs, const MPVec3 &p, float P)
{
    // TODO: Move this (arbitrary-defined) constant somewhere better
    const float a = 6.0f;
    
    float distance = MPVec3EuclideanDistance(pObs, p);
    
    // The gradient of the Euclidean distance function
    MPVec3 distanceGrad = MPVec3Make(p.x - pObs.x, p.y - pObs.y, p.z - pObs.z);
    distanceGrad = MPVec3MultiplyScalar(distanceGrad, sqrtf(powf(p.x - pObs.x, 2.0f) + powf(p.y - pObs.y, 2.0f) + powf(p.z - pObs.z, 2.0f)));

    if(distance > P)
        return MPVec3Zero;
    
    float multiplier = a * (1.0f / P - 1.0f / distance) * (1.0f / powf(distance, 2.0f));
    
    // Compute the gradient of the repulsive potential function
    return MPVec3MultiplyScalar(distanceGrad, multiplier);
}
}