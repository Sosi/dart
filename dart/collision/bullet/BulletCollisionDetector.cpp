/*
 * Copyright (c) 2014-2015, Georgia Tech Research Corporation
 * All rights reserved.
 *
 * Author(s): Jeongseok Lee <jslee02@gmail.com>
 *
 * Georgia Tech Graphics Lab and Humanoid Robotics Lab
 *
 * Directed by Prof. C. Karen Liu and Prof. Mike Stilman
 * <karenliu@cc.gatech.edu> <mstilman@cc.gatech.edu>
 *
 * This file is provided under the following "BSD-style" License:
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted provided that the following
 *   conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *   USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 */

#include "dart/collision/bullet/BulletCollisionDetector.h"

#include <vector>

#include "dart/collision/bullet/BulletCollisionNode.h"
#include "dart/dynamics/BodyNode.h"
#include "dart/dynamics/Skeleton.h"

namespace dart {
namespace collision {

//==============================================================================
struct CollisionFilter : public btOverlapFilterCallback
{
  // return true when pairs need collision
  virtual bool needBroadphaseCollision(btBroadphaseProxy* _proxy0,
                                       btBroadphaseProxy* _proxy1) const
  {
    assert((_proxy0 != nullptr && _proxy1 != nullptr) &&
           "Bullet broadphase overlapping pair proxies are nullptr");

    bool collide = (_proxy0->m_collisionFilterGroup &
                    _proxy1->m_collisionFilterMask) != 0;
    collide = collide && (_proxy1->m_collisionFilterGroup &
                          _proxy0->m_collisionFilterMask);

    if (collide)
    {
      btCollisionObject* collObj0 =
          static_cast<btCollisionObject*>(_proxy0->m_clientObject);
      btCollisionObject* collObj1 =
          static_cast<btCollisionObject*>(_proxy1->m_clientObject);

      BulletUserData* userData0 =
          static_cast<BulletUserData*>(collObj0->getUserPointer());
      BulletUserData* userData1 =
          static_cast<BulletUserData*>(collObj1->getUserPointer());

      // Assume single collision detector
      assert(userData0->btCollDet == userData1->btCollDet);

      CollisionDetector* cd = userData0->btCollDet;

      CollisionNode* cn1 = userData0->btCollNode;
      CollisionNode* cn2 = userData1->btCollNode;

      collide = cd->isCollidable(cn1, cn2);
    }

    return collide;
  }
};

//==============================================================================
BulletCollisionDetector::BulletCollisionDetector() : CollisionDetector()
{
  btCollisionConfiguration* collisionConfiguration
      = new btDefaultCollisionConfiguration();
  btDispatcher* dispatcher
      = new btCollisionDispatcher(collisionConfiguration);
  btBroadphaseInterface* broadphase
      = new btDbvtBroadphase();

  mBulletCollisionWorld
      = new btCollisionWorld(dispatcher, broadphase, collisionConfiguration);

  // Setting up broadphase collision detection options
  btDispatcherInfo& dispatchInfo = mBulletCollisionWorld->getDispatchInfo();
  dispatchInfo.m_timeStep  = 0.001;
  dispatchInfo.m_stepCount = 0;

  btOverlapFilterCallback* filterCB = new CollisionFilter();
  mBulletCollisionWorld->getPairCache()->setOverlapFilterCallback(filterCB);
}

//==============================================================================
BulletCollisionDetector::~BulletCollisionDetector()
{
}

//==============================================================================
CollisionNode* BulletCollisionDetector::createCollisionNode(
    dynamics::BodyNode* _bodyNode)
{
  BulletCollisionNode* collNode = new BulletCollisionNode(_bodyNode);

  for (int i = 0; i < collNode->getNumBulletCollisionObjects(); ++i)
  {
    btCollisionObject* collObj = collNode->getBulletCollisionObject(i);
    BulletUserData* userData
        = static_cast<BulletUserData*>(collObj->getUserPointer());
    userData->btCollDet = this;
    mBulletCollisionWorld->addCollisionObject(collObj);
  }

  return collNode;
}

//==============================================================================
void BulletCollisionDetector::destroyCollisionNode(
    const dynamics::BodyNode* bodyNode)
{
  if (nullptr == bodyNode)
    return;

  BulletCollisionNode* collNode
      = static_cast<BulletCollisionNode*>(getCollisionNode(bodyNode));

  if (nullptr == collNode)
    return;

  for (int i = 0; i < collNode->getNumBulletCollisionObjects(); ++i)
  {
    btCollisionObject* collObj = collNode->getBulletCollisionObject(i);
    mBulletCollisionWorld->removeCollisionObject(collObj);
  }
}

//==============================================================================
bool BulletCollisionDetector::detectCollision(bool /*checkAllCollisions*/,
                                              bool /*calculateContactPoints*/)
{
  // Clear mContacts which is the list of old contacts
  clearAllContacts();

  for (std::size_t i = 0; i < mCollisionNodes.size(); i++)
  for (auto collNode : mCollisionNodes)
  {
    // Set all the body nodes are not in colliding
    collNode->getBodyNode()->setColliding(false);

    // Update all the transformations of the collision nodes
    assert(nullptr != dynamic_cast<BulletCollisionNode*>(collNode));
    static_cast<BulletCollisionNode*>(collNode)->updateBulletCollisionObjects();
  }

  // Collision detection
  mBulletCollisionWorld->performDiscreteCollisionDetection();

  // Add all the contacts to mContacts
  int numManifolds = mBulletCollisionWorld->getDispatcher()->getNumManifolds();
  btDispatcher* dispatcher = mBulletCollisionWorld->getDispatcher();
  for (int i = 0; i < numManifolds; ++i)
  {
    btPersistentManifold* contactManifold
        = dispatcher->getManifoldByIndexInternal(i);
    const btCollisionObject* obA = contactManifold->getBody0();
    const btCollisionObject* obB = contactManifold->getBody1();

    BulletUserData* userDataA
        = static_cast<BulletUserData*>(obA->getUserPointer());
    BulletUserData* userDataB
        = static_cast<BulletUserData*>(obB->getUserPointer());

    int numContacts = contactManifold->getNumContacts();
    for (int j = 0; j < numContacts; j++)
    {
      btManifoldPoint& cp = contactManifold->getContactPoint(j);

      Contact contactPair;
      contactPair.point            = convertVector3(cp.getPositionWorldOnA());
      contactPair.normal           = convertVector3(cp.m_normalWorldOnB);
      contactPair.penetrationDepth = -cp.getDistance();
      contactPair.bodyNode1   = userDataA->btCollNode->getBodyNode();
      contactPair.bodyNode2   = userDataB->btCollNode->getBodyNode();

      mContacts.push_back(contactPair);

      // Set these two bodies are in colliding
      contactPair.bodyNode1.lock()->setColliding(true);
      contactPair.bodyNode2.lock()->setColliding(true);
    }
  }

  // Return true if there are contacts
  return !mContacts.empty();
}

//==============================================================================
bool BulletCollisionDetector::detectCollision(CollisionNode* _node1,
                                              CollisionNode* _node2,
                                              bool _calculateContactPoints)
{
  assert(false);
  return false;
}

}  // namespace collision
}  // namespace dart
