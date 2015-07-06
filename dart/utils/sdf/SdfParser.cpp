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

#include <map>
#include <iostream>
#include <fstream>

#include "dart/common/Console.h"
#include "dart/dynamics/BodyNode.h"
#include "dart/dynamics/SoftBodyNode.h"
#include "dart/dynamics/BoxShape.h"
#include "dart/dynamics/CylinderShape.h"
#include "dart/dynamics/EllipsoidShape.h"
#include "dart/dynamics/MeshShape.h"
#include "dart/dynamics/SoftMeshShape.h"
#include "dart/dynamics/WeldJoint.h"
#include "dart/dynamics/PrismaticJoint.h"
#include "dart/dynamics/RevoluteJoint.h"
#include "dart/dynamics/ScrewJoint.h"
#include "dart/dynamics/TranslationalJoint.h"
#include "dart/dynamics/BallJoint.h"
#include "dart/dynamics/FreeJoint.h"
#include "dart/dynamics/EulerJoint.h"
#include "dart/dynamics/UniversalJoint.h"
#include "dart/dynamics/Skeleton.h"
#include "dart/simulation/World.h"
#include "dart/utils/SkelParser.h"
#include "dart/utils/sdf/SdfParser.h"

namespace dart {
namespace utils {

//==============================================================================
simulation::WorldPtr SdfParser::readSdfFile(const std::string& _filename)
{
  //--------------------------------------------------------------------------
  // Load xml and create Document
  tinyxml2::XMLDocument _dartFile;
  try
  {
    openXMLFile(_dartFile, _filename.c_str());
  }
  catch(std::exception const& e)
  {
    std::cout << "LoadFile Fails: " << e.what() << std::endl;
    return nullptr;
  }

  //--------------------------------------------------------------------------
  // Load DART
  tinyxml2::XMLElement* sdfElement = nullptr;
  sdfElement = _dartFile.FirstChildElement("sdf");
  if (sdfElement == nullptr)
    return nullptr;

  //--------------------------------------------------------------------------
  // version attribute
  std::string version = getAttribute(sdfElement, "version");
  // TODO: We need version aware SDF parser (see #264)
  // We support 1.4 only for now.
  if (version != "1.4" && version != "1.5")
  {
    dterr << "The file format of ["
          << _filename
          << "] is not sdf 1.4 or 1.5."
          << std::endl;
    return nullptr;
  }

  //--------------------------------------------------------------------------
  // Load World
  tinyxml2::XMLElement* worldElement = nullptr;
  worldElement = sdfElement->FirstChildElement("world");
  if (worldElement == nullptr)
      return nullptr;

  // Change path to a Unix-style path if given a Windows one
  // Windows can handle Unix-style paths (apparently)
  std::string unixFileName = _filename;
  std::replace(unixFileName.begin(), unixFileName.end(), '\\' , '/' );
  std::string skelPath = unixFileName.substr(0, unixFileName.rfind("/") + 1);

  return readWorld(worldElement, skelPath);
}

//==============================================================================
simulation::WorldPtr SdfParser::readSdfFile(const std::string& _filename,
    std::function<simulation::WorldPtr (tinyxml2::XMLElement*,
                                        const std::string&)>)
{
  return readSdfFile(_filename);
}

//==============================================================================
dynamics::SkeletonPtr SdfParser::readSkeleton(const std::string& _filename)
{
  //--------------------------------------------------------------------------
  // Load xml and create Document
  tinyxml2::XMLDocument _dartFile;
  try
  {
    openXMLFile(_dartFile, _filename.c_str());
  }
  catch(std::exception const& e)
  {
    std::cout << "LoadFile Fails: " << e.what() << std::endl;
    return nullptr;
  }

  //--------------------------------------------------------------------------
  // Load sdf
  tinyxml2::XMLElement* sdfElement = nullptr;
  sdfElement = _dartFile.FirstChildElement("sdf");
  if (sdfElement == nullptr)
      return nullptr;

  //--------------------------------------------------------------------------
  // version attribute
  std::string version = getAttribute(sdfElement, "version");
  // TODO: We need version aware SDF parser (see #264)
  // We support 1.4 only for now.
  if (version != "1.4" && version != "1.5")
  {
    dterr << "The file format of ["
          << _filename
          << "] is not sdf 1.4 or 1.5."
          << std::endl;
    return nullptr;
  }

  //--------------------------------------------------------------------------
  // Load skeleton
  tinyxml2::XMLElement* skelElement = nullptr;
  skelElement = sdfElement->FirstChildElement("model");
  if (skelElement == nullptr)
      return nullptr;

  // Change path to a Unix-style path if given a Windows one
  // Windows can handle Unix-style paths (apparently)
  std::string unixFileName = _filename;
  std::replace(unixFileName.begin(), unixFileName.end(), '\\' , '/' );
  std::string skelPath = unixFileName.substr(0, unixFileName.rfind("/") + 1);

  dynamics::SkeletonPtr newSkeleton = readSkeleton(skelElement, skelPath);

  return newSkeleton;
}

//==============================================================================
dynamics::SkeletonPtr SdfParser::readSkeleton(
    const std::string& _filename,
    std::function<dynamics::SkeletonPtr(tinyxml2::XMLElement*,
                                        const std::string&)> xmlReader)
{
  return readSkeleton(_filename);
}

//==============================================================================
simulation::WorldPtr SdfParser::readWorld(
    tinyxml2::XMLElement* _worldElement, const std::string& _skelPath)
{
  assert(_worldElement != nullptr);

  // Create a world
  simulation::WorldPtr newWorld(new simulation::World);

  //--------------------------------------------------------------------------
  // Name attribute
  std::string name = getAttribute(_worldElement, "name");
  newWorld->setName(name);

  //--------------------------------------------------------------------------
  // Load physics
  if (hasElement(_worldElement, "physics"))
  {
    tinyxml2::XMLElement* physicsElement = _worldElement->FirstChildElement("physics");
    readPhysics(physicsElement, newWorld);
  }

  //--------------------------------------------------------------------------
  // Load skeletons
  ElementEnumerator skeletonElements(_worldElement, "model");
  while (skeletonElements.next())
  {
    dynamics::SkeletonPtr newSkeleton
        = readSkeleton(skeletonElements.get(), _skelPath);

    newWorld->addSkeleton(newSkeleton);
  }

  return newWorld;
}

//==============================================================================
simulation::WorldPtr SdfParser::readWorld(
    tinyxml2::XMLElement* _worldElement,
    const std::string& _skelPath,
    std::function<dynamics::SkeletonPtr(tinyxml2::XMLElement*,
                                        const std::string&)>)
{
  return readWorld(_worldElement, _skelPath);
}

//==============================================================================
void SdfParser::readPhysics(tinyxml2::XMLElement* _physicsElement,
                 simulation::WorldPtr _world)
{
    // Type attribute
//    std::string physicsEngineName = getAttribute(_physicsElement, "type");

    // Time step
    if (hasElement(_physicsElement, "max_step_size"))
    {
        double timeStep = getValueDouble(_physicsElement, "max_step_size");
        _world->setTimeStep(timeStep);
    }

    // Number of max contacts
//    if (hasElement(_physicsElement, "max_contacts"))
//    {
//        int timeStep = getValueInt(_physicsElement, "max_contacts");
//        _world->setMaxNumContacts(timeStep);
//    }

    // Gravity
    if (hasElement(_physicsElement, "gravity"))
    {
        Eigen::Vector3d gravity = getValueVector3d(_physicsElement, "gravity");
        _world->setGravity(gravity);
    }
}

//==============================================================================
dynamics::SkeletonPtr SdfParser::readSkeleton(
    tinyxml2::XMLElement* _skeletonElement, const std::string& _skelPath)
{
  assert(_skeletonElement != nullptr);

  Eigen::Isometry3d skeletonFrame = Eigen::Isometry3d::Identity();
  dynamics::SkeletonPtr newSkeleton = makeSkeleton(
        _skeletonElement, skeletonFrame);

  //--------------------------------------------------------------------------
  // Bodies
  BodyMap sdfBodyNodes = readAllBodyNodes(_skeletonElement, _skelPath,
                                          skeletonFrame, &readSoftBodyNode);

  //--------------------------------------------------------------------------
  // Joints
  JointMap sdfJoints = readAllJoints(_skeletonElement, skeletonFrame,
                                     sdfBodyNodes);

  // Iterate through the collected properties and construct the Skeleton from
  // the root nodes downward
  JointMap::iterator it = sdfJoints.begin();
  BodyMap::const_iterator child;
  dynamics::BodyNode* parent;
  while(it != sdfJoints.end())
  {
    NextResult result = getNextJointAndNodePair(
          it, child, parent, newSkeleton, sdfJoints, sdfBodyNodes);

    if(BREAK == result)
      break;
    else if(CONTINUE == result)
      continue;
    else if(CREATE_FREEJOINT_ROOT == result)
    {
      // If a root FreeJoint is needed for the parent of the current joint, then
      // create it
      BodyMap::const_iterator rootNode = sdfBodyNodes.find(it->second.parentName);
      SDFJoint rootJoint;
      rootJoint.properties =
          Eigen::make_aligned_shared<dynamics::FreeJoint::Properties>(
            dynamics::Joint::Properties("root", rootNode->second.initTransform));
      rootJoint.type = "free";

      if(!createPair(newSkeleton, nullptr, rootJoint, rootNode->second))
        break;

      continue;
    }

    if(!createPair(newSkeleton, parent, it->second, child->second))
      break;

    sdfJoints.erase(it);
    it = sdfJoints.begin();
  }

  return newSkeleton;
}

//==============================================================================
dynamics::SkeletonPtr SdfParser::readSkeleton(
    tinyxml2::XMLElement* _skeletonElement,
    const std::string& _skelPath,
    std::function<SDFBodyNode (tinyxml2::XMLElement*,
                   const Eigen::Isometry3d&,
                   const std::string&)>,
    std::function<bool(dynamics::SkeletonPtr,
                       dynamics::BodyNode*,
                       const SDFJoint&,
                       const SDFBodyNode&)>)
{
  return readSkeleton(_skeletonElement, _skelPath);
}

//==============================================================================
bool SdfParser::createPair(
    dynamics::SkeletonPtr skeleton,
    dynamics::BodyNode* parent,
    const SDFJoint& newJoint,
    const SDFBodyNode& newBody)
{
  std::pair<dynamics::Joint*, dynamics::BodyNode*> pair;
  if(newBody.type.empty())
    pair = createJointAndNodePair<dynamics::BodyNode>(
          skeleton, parent, newJoint, newBody);
  else if(std::string("soft") == newBody.type)
    pair = createJointAndNodePair<dynamics::SoftBodyNode>(
          skeleton, parent, newJoint, newBody);
  else
  {
    dterr << "[SoftSdfParser::createPair] Unsupported Link type: "
          << newBody.type << "\n";
    return false;
  }

  if(!pair.first || !pair.second)
    return false;

  return true;
}

//==============================================================================
SdfParser::NextResult SdfParser::getNextJointAndNodePair(
    JointMap::iterator& it,
    BodyMap::const_iterator& child,
    dynamics::BodyNode*& parent,
    const dynamics::SkeletonPtr skeleton,
    JointMap& sdfJoints,
    const BodyMap& sdfBodyNodes)
{
  NextResult result = VALID;
  const SDFJoint& joint = it->second;
  parent = skeleton->getBodyNode(joint.parentName);
  if(nullptr == parent
     && joint.parentName != "world" && !joint.parentName.empty())
  {
    // Find the properties of the parent Joint of the current Joint, because it
    // does not seem to be created yet.
    JointMap::iterator check_parent_joint = sdfJoints.find(joint.parentName);
    if(check_parent_joint == sdfJoints.end())
    {
      BodyMap::const_iterator check_parent_node = sdfBodyNodes.find(joint.parentName);
      if(check_parent_node == sdfBodyNodes.end())
      {
        dterr << "[SdfParser::getNextJointAndNodePair] Could not find Link "
              << "named [" << joint.parentName << "] requested as parent of "
              << "Joint [" << joint.properties->mName << "]. We will now quit "
              << "parsing.\n";
        return BREAK;
      }

      // If the current Joint has a parent BodyNode but does not have a parent
      // Joint, then we need to create a FreeJoint for the parent BodyNode.
      result = CREATE_FREEJOINT_ROOT;
    }
    else
    {
      it = check_parent_joint;
      return CONTINUE; // Create the parent before creating the current Joint
    }
  }

  // Find the child node of this Joint, so we can create them together
  child = sdfBodyNodes.find(joint.childName);
  if(child == sdfBodyNodes.end())
  {
    dterr << "[SdfParser::getNextJointAndNodePair] Could not find Link named ["
          << joint.childName << "] requested as child of Joint ["
          << joint.properties->mName << "]. This should not be possible! "
          << "We will now quit parsing. Please report this bug!\n";
    return BREAK;
  }

  return result;
}

dynamics::SkeletonPtr SdfParser::makeSkeleton(
    tinyxml2::XMLElement* _skeletonElement,
    Eigen::Isometry3d& skeletonFrame)
{
  assert(_skeletonElement != nullptr);

  dynamics::SkeletonPtr newSkeleton = dynamics::Skeleton::create();

  //--------------------------------------------------------------------------
  // Name attribute
  std::string name = getAttribute(_skeletonElement, "name");
  newSkeleton->setName(name);

  //--------------------------------------------------------------------------
  // immobile attribute
  if (hasElement(_skeletonElement, "static"))
  {
      bool isStatic= getValueBool(_skeletonElement, "static");
      newSkeleton->setMobile(!isStatic);
  }

  //--------------------------------------------------------------------------
  // transformation
  if (hasElement(_skeletonElement, "pose"))
  {
      Eigen::Isometry3d W = getValueIsometry3dWithExtrinsicRotation(_skeletonElement, "pose");
      skeletonFrame = W;
  }

  return newSkeleton;
}

SdfParser::BodyMap SdfParser::readAllBodyNodes(
    tinyxml2::XMLElement* _skeletonElement,
    const std::string& _skelPath,
    const Eigen::Isometry3d& skeletonFrame)
{
  return readAllBodyNodes(_skeletonElement, _skelPath, skeletonFrame,
                          &readBodyNode);
}

SdfParser::BodyMap SdfParser::readAllBodyNodes(
    tinyxml2::XMLElement* _skeletonElement,
    const std::string& _skelPath,
    const Eigen::Isometry3d& skeletonFrame,
    std::function<SDFBodyNode (
      tinyxml2::XMLElement*,
      const Eigen::Isometry3d&,
      const std::string&)> bodyReader)
{
  ElementEnumerator bodies(_skeletonElement, "link");
  BodyMap sdfBodyNodes;
  while (bodies.next())
  {
    SDFBodyNode body = bodyReader(
          bodies.get(), skeletonFrame, _skelPath);

    BodyMap::iterator it = sdfBodyNodes.find(body.properties->mName);
    if(it != sdfBodyNodes.end())
    {
      dtwarn << "[SdfParser::readAllBodyNodes] Duplicate name in file: "
             << body.properties->mName << "\n"
             << "Every Link must have a unique name!\n";
      continue;
    }

    sdfBodyNodes[body.properties->mName] = body;
  }

  return sdfBodyNodes;
}

SdfParser::SDFBodyNode SdfParser::readBodyNode(
        tinyxml2::XMLElement* _bodyNodeElement,
        const Eigen::Isometry3d& _skeletonFrame,
        const std::string& _skelPath)
{
  assert(_bodyNodeElement != nullptr);

  dynamics::BodyNode::Properties properties;
  Eigen::Isometry3d initTransform = Eigen::Isometry3d::Identity();

  // Name attribute
  std::string name = getAttribute(_bodyNodeElement, "name");
  properties.mName = name;

  //--------------------------------------------------------------------------
  // gravity
  if (hasElement(_bodyNodeElement, "gravity"))
  {
    bool gravityMode = getValueBool(_bodyNodeElement, "gravity");
    properties.mGravityMode = gravityMode;
  }

  //--------------------------------------------------------------------------
  // self_collide
//    if (hasElement(_bodyElement, "self_collide"))
//    {
//        bool gravityMode = getValueBool(_bodyElement, "self_collide");
//    }

  //--------------------------------------------------------------------------
  // transformation
  if (hasElement(_bodyNodeElement, "pose"))
  {
    Eigen::Isometry3d W = getValueIsometry3dWithExtrinsicRotation(_bodyNodeElement, "pose");
    initTransform = _skeletonFrame * W;
  }
  else
  {
    initTransform = _skeletonFrame;
  }

  //--------------------------------------------------------------------------
  // visual
  ElementEnumerator vizShapes(_bodyNodeElement, "visual");
  while (vizShapes.next())
  {
    dynamics::ShapePtr newShape(readShape(vizShapes.get(), _skelPath));
    if (newShape)
      properties.mVizShapes.push_back(newShape);
  }

  //--------------------------------------------------------------------------
  // collision
  ElementEnumerator collShapes(_bodyNodeElement, "collision");
  while (collShapes.next())
  {
    dynamics::ShapePtr newShape(readShape(collShapes.get(), _skelPath));

    if (newShape)
      properties.mColShapes.push_back(newShape);
  }

  //--------------------------------------------------------------------------
  // inertia
  if (hasElement(_bodyNodeElement, "inertial"))
  {
    tinyxml2::XMLElement* inertiaElement = getElement(_bodyNodeElement, "inertial");

    // mass
    if (hasElement(inertiaElement, "mass"))
    {
      double mass = getValueDouble(inertiaElement, "mass");
      properties.mInertia.setMass(mass);
    }

    // offset
    if (hasElement(inertiaElement, "pose"))
    {
      Eigen::Isometry3d T = getValueIsometry3dWithExtrinsicRotation(inertiaElement, "pose");
      properties.mInertia.setLocalCOM(T.translation());
    }

    // inertia
    if (hasElement(inertiaElement, "inertia"))
    {
      tinyxml2::XMLElement* moiElement
              = getElement(inertiaElement, "inertia");

      double ixx = getValueDouble(moiElement, "ixx");
      double iyy = getValueDouble(moiElement, "iyy");
      double izz = getValueDouble(moiElement, "izz");

      double ixy = getValueDouble(moiElement, "ixy");
      double ixz = getValueDouble(moiElement, "ixz");
      double iyz = getValueDouble(moiElement, "iyz");

      properties.mInertia.setMoment(ixx, iyy, izz, ixy, ixz, iyz);
    }
    else if (properties.mVizShapes.size() > 0
             && properties.mVizShapes[0] != nullptr)
    {
      Eigen::Matrix3d Ic =
          properties.mVizShapes[0]->computeInertia(
            properties.mInertia.getMass());

      properties.mInertia.setMoment(Ic(0,0), Ic(1,1), Ic(2,2),
                                    Ic(0,1), Ic(0,2), Ic(1,2));
    }
  }

  SDFBodyNode sdfBodyNode;
  sdfBodyNode.properties =
      Eigen::make_aligned_shared<dynamics::BodyNode::Properties>(properties);
  sdfBodyNode.initTransform = initTransform;

  return sdfBodyNode;
}

SdfParser::SDFBodyNode SdfParser::readSoftBodyNode(
    tinyxml2::XMLElement* _softBodyNodeElement,
    const Eigen::Isometry3d& _skeletonFrame,
    const std::string& _skelPath)
{
  //---------------------------------- Note ------------------------------------
  // SoftBodyNode is created if _softBodyNodeElement has <soft_shape>.
  // Otherwise, BodyNode is created.

  //----------------------------------------------------------------------------
  assert(_softBodyNodeElement != nullptr);

  // If _softBodyNodeElement has no <soft_shape>, return rigid body node
  if (!hasElement(_softBodyNodeElement, "soft_shape"))
  {
    return SdfParser::readBodyNode(
          _softBodyNodeElement, _skeletonFrame, _skelPath);
  }

  SDFBodyNode standardSDF =
      SdfParser::readBodyNode(_softBodyNodeElement, _skeletonFrame, _skelPath);

  BodyPropPtr standardProperties = standardSDF.properties;

  dynamics::SoftBodyNode::UniqueProperties softProperties;

  //----------------------------------------------------------------------------
  // Soft properties
  if (hasElement(_softBodyNodeElement, "soft_shape"))
  {
    tinyxml2::XMLElement* softShapeEle
        = getElement(_softBodyNodeElement, "soft_shape");

    // mass
    double totalMass = getValueDouble(softShapeEle, "total_mass");

    // pose
    Eigen::Isometry3d T = Eigen::Isometry3d::Identity();
    if (hasElement(softShapeEle, "pose"))
      T = getValueIsometry3dWithExtrinsicRotation(softShapeEle, "pose");

    // geometry
    tinyxml2::XMLElement* geometryEle = getElement(softShapeEle, "geometry");
    if (hasElement(geometryEle, "box"))
    {
      tinyxml2::XMLElement* boxEle = getElement(geometryEle, "box");
      Eigen::Vector3d size  = getValueVector3d(boxEle, "size");
      Eigen::Vector3i frags = getValueVector3i(boxEle, "frags");
      softProperties = dynamics::SoftBodyNodeHelper::makeBoxProperties(
            size, T, frags, totalMass);
    }
    else if (hasElement(geometryEle, "ellipsoid"))
    {
      tinyxml2::XMLElement* ellipsoidEle = getElement(geometryEle, "ellipsoid");
      Eigen::Vector3d size = getValueVector3d(ellipsoidEle, "size");
      double nSlices       = getValueDouble(ellipsoidEle, "num_slices");
      double nStacks       = getValueDouble(ellipsoidEle, "num_stacks");
      softProperties = dynamics::SoftBodyNodeHelper::makeEllipsoidProperties(
            size, nSlices, nStacks, totalMass);
    }
    else if (hasElement(geometryEle, "cylinder"))
    {
      tinyxml2::XMLElement* ellipsoidEle = getElement(geometryEle, "cylinder");
      double radius  = getValueDouble(ellipsoidEle, "radius");
      double height  = getValueDouble(ellipsoidEle, "height");
      double nSlices = getValueDouble(ellipsoidEle, "num_slices");
      double nStacks = getValueDouble(ellipsoidEle, "num_stacks");
      double nRings = getValueDouble(ellipsoidEle, "num_rings");
      softProperties = dynamics::SoftBodyNodeHelper::makeCylinderProperties(
            radius, height, nSlices, nStacks, nRings, totalMass);
    }
    else
    {
      dterr << "Unknown soft shape.\n";
    }

    // kv
    if (hasElement(softShapeEle, "kv"))
    {
      softProperties.mKv = getValueDouble(softShapeEle, "kv");
    }

    // ke
    if (hasElement(softShapeEle, "ke"))
    {
      softProperties.mKe = getValueDouble(softShapeEle, "ke");
    }

    // damp
    if (hasElement(softShapeEle, "damp"))
    {
      softProperties.mDampCoeff = getValueDouble(softShapeEle, "damp");
    }
  }

  SDFBodyNode sdfBodyNode;
  sdfBodyNode.properties =
      Eigen::make_aligned_shared<dynamics::SoftBodyNode::Properties>(
        *standardProperties, softProperties);
  sdfBodyNode.initTransform = standardSDF.initTransform;
  sdfBodyNode.type = "soft";

  return sdfBodyNode;
}

dynamics::ShapePtr SdfParser::readShape(tinyxml2::XMLElement* _shapelement,
                                      const std::string& _skelPath)
{
  dynamics::ShapePtr newShape;

  // type
  assert(hasElement(_shapelement, "geometry"));
  tinyxml2::XMLElement* geometryElement = getElement(_shapelement, "geometry");

  if (hasElement(geometryElement, "box"))
  {
    tinyxml2::XMLElement* boxElement = getElement(geometryElement, "box");

    Eigen::Vector3d size = getValueVector3d(boxElement, "size");

    newShape = dynamics::ShapePtr(new dynamics::BoxShape(size));
  }
  else if (hasElement(geometryElement, "sphere"))
  {
    tinyxml2::XMLElement* ellipsoidElement = getElement(geometryElement, "sphere");

    double radius = getValueDouble(ellipsoidElement, "radius");
    Eigen::Vector3d size(radius * 2, radius * 2, radius * 2);

    newShape = dynamics::ShapePtr(new dynamics::EllipsoidShape(size));
  }
  else if (hasElement(geometryElement, "cylinder"))
  {
    tinyxml2::XMLElement* cylinderElement = getElement(geometryElement, "cylinder");

    double radius = getValueDouble(cylinderElement, "radius");
    double height = getValueDouble(cylinderElement, "length");

    newShape = dynamics::ShapePtr(new dynamics::CylinderShape(radius, height));
  }
  else if (hasElement(geometryElement, "plane"))
  {
    // TODO: Don't support plane shape yet.
    tinyxml2::XMLElement* planeElement = getElement(geometryElement, "plane");

    Eigen::Vector2d visSize = getValueVector2d(planeElement, "size");
    // TODO: Need to use normal for correct orientation of the plane
    //Eigen::Vector3d normal = getValueVector3d(planeElement, "normal");

    Eigen::Vector3d size(visSize(0), visSize(1), 0.001);

    newShape = dynamics::ShapePtr(new dynamics::BoxShape(size));
  }
  else if (hasElement(geometryElement, "mesh"))
  {
    tinyxml2::XMLElement* meshEle = getElement(geometryElement, "mesh");
    // TODO(JS): We assume that uri is just file name for the mesh
    std::string           uri     = getValueString(meshEle, "uri");
    Eigen::Vector3d       scale   = getValueVector3d(meshEle, "scale");
    const aiScene* model = dynamics::MeshShape::loadMesh(_skelPath + uri);
    if (model)
      newShape = dynamics::ShapePtr(new dynamics::MeshShape(scale, model,
                                                            _skelPath));
    else
      dterr << "Fail to load model[" << uri << "]." << std::endl;
  }
  else
  {
    std::cout << "Invalid shape type." << std::endl;
    return nullptr;
  }

  // pose
  if (hasElement(_shapelement, "pose"))
  {
    Eigen::Isometry3d W = getValueIsometry3dWithExtrinsicRotation(_shapelement, "pose");
    newShape->setLocalTransform(W);
  }

  return newShape;
}

SdfParser::JointMap SdfParser::readAllJoints(
    tinyxml2::XMLElement* _skeletonElement,
    const Eigen::Isometry3d& skeletonFrame,
    const BodyMap& sdfBodyNodes)
{
  JointMap sdfJoints;
  ElementEnumerator joints(_skeletonElement, "joint");
  while (joints.next())
  {
    SDFJoint joint = readJoint(joints.get(), sdfBodyNodes, skeletonFrame);

    if(joint.childName.empty())
    {
      dterr << "[SdfParser::readAllJoints] Joint named ["
            << joint.properties->mName << "] does not have a valid child "
            << "Link, so it will not be added to the Skeleton\n";
      continue;
    }

    JointMap::iterator it = sdfJoints.find(joint.childName);
    if(it != sdfJoints.end())
    {
      dterr << "[SdfParser::readAllJoints] Joint named ["
            << joint.properties->mName << "] is claiming Link ["
            << joint.childName << "] as its child, but that is already "
            << "claimed by Joint [" << it->second.properties->mName
            << "]. Joint [" << joint.properties->mName
            << "] will be discarded\n";
      continue;
    }

    sdfJoints[joint.childName] = joint;
  }

  return sdfJoints;
}

SdfParser::SDFJoint SdfParser::readJoint(tinyxml2::XMLElement* _jointElement,
    const BodyMap& _sdfBodyNodes,
    const Eigen::Isometry3d& _skeletonFrame)
{
  assert(_jointElement != nullptr);

  //--------------------------------------------------------------------------
  // Type attribute
  std::string type = getAttribute(_jointElement, "type");
  assert(!type.empty());

  //--------------------------------------------------------------------------
  // Name attribute
  std::string name = getAttribute(_jointElement, "name");

  //--------------------------------------------------------------------------
  // parent
  BodyMap::const_iterator parent_it = _sdfBodyNodes.end();

  if (hasElement(_jointElement, "parent"))
  {
    std::string strParent = getValueString(_jointElement, "parent");

    if(strParent != std::string("world"))
    {
      parent_it = _sdfBodyNodes.find(strParent);

      if( parent_it == _sdfBodyNodes.end() )
      {
        dterr << "[SdfParser::readJoint] Cannot find a Link named ["
              << strParent << "] requested as the parent of the Joint named ["
              << name << "]\n";
        assert(0);
      }
    }
  }
  else
  {
    dterr << "[SdfParser::readJoint] You must set parent link for "
          << "the Joint [" << name << "]!\n";
    assert(0);
  }

  //--------------------------------------------------------------------------
  // child
  BodyMap::const_iterator child_it = _sdfBodyNodes.end();

  if (hasElement(_jointElement, "child"))
  {
    std::string strChild = getValueString(_jointElement, "child");

    child_it = _sdfBodyNodes.find(strChild);

    if ( child_it == _sdfBodyNodes.end() )
    {
      dterr << "[SdfParser::readJoint] Cannot find a Link named [" << strChild
            << "] requested as the child of the Joint named [" << name << "]\n";
      assert(0);
    }
  }
  else
  {
    dterr << "[SdfParser::readJoint] You must set the child link for the Joint "
          << "[" << name << "]!\n";
    assert(0);
  }

  SDFJoint newJoint;
  newJoint.parentName = (parent_it == _sdfBodyNodes.end())?
        "" : parent_it->first;
  newJoint.childName = (parent_it == _sdfBodyNodes.end())?
        "" : child_it->first;

  //--------------------------------------------------------------------------
  // transformation
  Eigen::Isometry3d parentWorld = Eigen::Isometry3d::Identity();
  Eigen::Isometry3d childToJoint = Eigen::Isometry3d::Identity();
  Eigen::Isometry3d childWorld =  Eigen::Isometry3d::Identity();

  if (parent_it != _sdfBodyNodes.end())
    parentWorld = parent_it->second.initTransform;
  if (child_it != _sdfBodyNodes.end())
    childWorld = child_it->second.initTransform;
  if (hasElement(_jointElement, "pose"))
    childToJoint = getValueIsometry3dWithExtrinsicRotation(_jointElement, "pose");

  Eigen::Isometry3d parentToJoint = parentWorld.inverse()*childWorld*childToJoint;

  // TODO: Workaround!!
  Eigen::Isometry3d parentModelFrame =
      (childWorld * childToJoint).inverse() * _skeletonFrame;

  if (type == std::string("prismatic"))
    newJoint.properties =
        Eigen::make_aligned_shared<dynamics::PrismaticJoint::Properties>(
          readPrismaticJoint(_jointElement, parentModelFrame, name));
  if (type == std::string("revolute"))
    newJoint.properties =
        Eigen::make_aligned_shared<dynamics::RevoluteJoint::Properties>(
          readRevoluteJoint(_jointElement, parentModelFrame, name));
  if (type == std::string("screw"))
    newJoint.properties =
        Eigen::make_aligned_shared<dynamics::ScrewJoint::Properties>(
          readScrewJoint(_jointElement, parentModelFrame, name));
  if (type == std::string("revolute2"))
    newJoint.properties =
        Eigen::make_aligned_shared<dynamics::UniversalJoint::Properties>(
          readUniversalJoint(_jointElement, parentModelFrame, name));
  if (type == std::string("ball"))
    newJoint.properties =
        Eigen::make_aligned_shared<dynamics::BallJoint::Properties>(
          readBallJoint(_jointElement, parentModelFrame, name));

  newJoint.type = type;

  newJoint.properties->mName = name;

  newJoint.properties->mT_ChildBodyToJoint = childToJoint;
  newJoint.properties->mT_ParentBodyToJoint = parentToJoint;

  return newJoint;
}

static void reportMissingElement(const std::string& functionName,
                                 const std::string& elementName,
                                 const std::string& objectType,
                                 const std::string& objectName)
{
  dterr << "[SdfParser::" << functionName << "] Missing element " << elementName
        << " for " << objectType << " named " << objectName << "\n";
  assert(0);
}

static void readAxisElement(
    tinyxml2::XMLElement* axisElement,
    const Eigen::Isometry3d& _parentModelFrame,
    Eigen::Vector3d& axis, double& lower, double& upper, double& damping)
{
  // use_parent_model_frame
  bool useParentModelFrame = false;
  if (hasElement(axisElement, "use_parent_model_frame"))
    useParentModelFrame = getValueBool(axisElement, "use_parent_model_frame");

  // xyz
  Eigen::Vector3d xyz = getValueVector3d(axisElement, "xyz");
  if (useParentModelFrame)
  {
    xyz = _parentModelFrame * xyz;
  }
  axis = xyz;

  // dynamics
  if (hasElement(axisElement, "dynamics"))
  {
    tinyxml2::XMLElement* dynamicsElement
            = getElement(axisElement, "dynamics");

    // damping
    if (hasElement(dynamicsElement, "damping"))
    {
      damping = getValueDouble(dynamicsElement, "damping");
    }
  }

  // limit
  if (hasElement(axisElement, "limit"))
  {
    tinyxml2::XMLElement* limitElement
            = getElement(axisElement, "limit");

    // lower
    if (hasElement(limitElement, "lower"))
    {
      lower = getValueDouble(limitElement, "lower");
    }

    // upper
    if (hasElement(limitElement, "upper"))
    {
      upper = getValueDouble(limitElement, "upper");
    }
  }
}

dart::dynamics::WeldJoint::Properties SdfParser::readWeldJoint(
    tinyxml2::XMLElement* _jointElement,
    const Eigen::Isometry3d&,
    const std::string&)
{
    assert(_jointElement != nullptr);

    return dynamics::WeldJoint::Properties();
}

dynamics::RevoluteJoint::Properties SdfParser::readRevoluteJoint(
    tinyxml2::XMLElement* _revoluteJointElement,
    const Eigen::Isometry3d& _parentModelFrame,
    const std::string& _name)
{
  assert(_revoluteJointElement != nullptr);

  dynamics::RevoluteJoint::Properties newRevoluteJoint;

  //--------------------------------------------------------------------------
  // axis
  if (hasElement(_revoluteJointElement, "axis"))
  {
    tinyxml2::XMLElement* axisElement
            = getElement(_revoluteJointElement, "axis");

    readAxisElement(axisElement, _parentModelFrame,
                    newRevoluteJoint.mAxis,
                    newRevoluteJoint.mPositionLowerLimit,
                    newRevoluteJoint.mPositionUpperLimit,
                    newRevoluteJoint.mDampingCoefficient);
  }
  else
  {
    reportMissingElement("readRevoluteJoint", "axis", "joint", _name);
  }

  return newRevoluteJoint;
}

dynamics::PrismaticJoint::Properties SdfParser::readPrismaticJoint(
    tinyxml2::XMLElement* _jointElement,
    const Eigen::Isometry3d& _parentModelFrame,
    const std::string& _name)
{
  assert(_jointElement != nullptr);

  dynamics::PrismaticJoint::Properties newPrismaticJoint;

  //--------------------------------------------------------------------------
  // axis
  if (hasElement(_jointElement, "axis"))
  {
    tinyxml2::XMLElement* axisElement
            = getElement(_jointElement, "axis");

    readAxisElement(axisElement, _parentModelFrame,
                    newPrismaticJoint.mAxis,
                    newPrismaticJoint.mPositionLowerLimit,
                    newPrismaticJoint.mPositionUpperLimit,
                    newPrismaticJoint.mDampingCoefficient);
  }
  else
  {
    reportMissingElement("readPrismaticJoint", "axis", "joint", _name);
  }

  return newPrismaticJoint;
}

dynamics::ScrewJoint::Properties SdfParser::readScrewJoint(
    tinyxml2::XMLElement* _jointElement,
    const Eigen::Isometry3d& _parentModelFrame,
    const std::string& _name)
{
  assert(_jointElement != nullptr);

  dynamics::ScrewJoint::Properties newScrewJoint;

  //--------------------------------------------------------------------------
  // axis
  if (hasElement(_jointElement, "axis"))
  {
    tinyxml2::XMLElement* axisElement
            = getElement(_jointElement, "axis");

    readAxisElement(axisElement, _parentModelFrame,
                    newScrewJoint.mAxis,
                    newScrewJoint.mPositionLowerLimit,
                    newScrewJoint.mPositionUpperLimit,
                    newScrewJoint.mDampingCoefficient);
  }
  else
  {
    reportMissingElement("readScrewJoint", "axis", "joint", _name);
  }

  // pitch
  if (hasElement(_jointElement, "thread_pitch"))
  {
      double pitch = getValueDouble(_jointElement, "thread_pitch");
      newScrewJoint.mPitch = pitch;
  }

  return newScrewJoint;
}

dynamics::UniversalJoint::Properties SdfParser::readUniversalJoint(
    tinyxml2::XMLElement* _jointElement,
    const Eigen::Isometry3d& _parentModelFrame,
    const std::string& _name)
{
  assert(_jointElement != nullptr);

  dynamics::UniversalJoint::Properties newUniversalJoint;

  //--------------------------------------------------------------------------
  // axis
  if (hasElement(_jointElement, "axis"))
  {
    tinyxml2::XMLElement* axisElement
            = getElement(_jointElement, "axis");

    readAxisElement(axisElement, _parentModelFrame,
                    newUniversalJoint.mAxis[0],
                    newUniversalJoint.mPositionLowerLimits[0],
                    newUniversalJoint.mPositionUpperLimits[0],
                    newUniversalJoint.mDampingCoefficients[0]);
  }
  else
  {
    reportMissingElement("readUniversalJoint", "axis", "joint", _name);
  }

  //--------------------------------------------------------------------------
  // axis2
  if (hasElement(_jointElement, "axis2"))
  {
    tinyxml2::XMLElement* axis2Element
            = getElement(_jointElement, "axis2");

    readAxisElement(axis2Element, _parentModelFrame,
                    newUniversalJoint.mAxis[1],
                    newUniversalJoint.mPositionLowerLimits[1],
                    newUniversalJoint.mPositionUpperLimits[1],
                    newUniversalJoint.mDampingCoefficients[1]);
  }
  else
  {
    reportMissingElement("readUniversalJoint", "axis2", "joint", _name);
  }

  return newUniversalJoint;
}

dynamics::BallJoint::Properties SdfParser::readBallJoint(
    tinyxml2::XMLElement* _jointElement,
    const Eigen::Isometry3d&,
    const std::string&)
{
  assert(_jointElement != nullptr);

  return dynamics::BallJoint::Properties();
}

dynamics::TranslationalJoint::Properties SdfParser::readTranslationalJoint(
    tinyxml2::XMLElement* _jointElement,
    const Eigen::Isometry3d&,
    const std::string&)
{
  assert(_jointElement != nullptr);

  return dynamics::TranslationalJoint::Properties();
}

dynamics::FreeJoint::Properties SdfParser::readFreeJoint(
    tinyxml2::XMLElement* _jointElement,
    const Eigen::Isometry3d&,
    const std::string&)
{
  assert(_jointElement != nullptr);

  return dynamics::FreeJoint::Properties();
}

} // namespace utils
} // namespace dart
