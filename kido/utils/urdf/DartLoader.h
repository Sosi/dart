/**
 * @file DartLoader.h
 */

#ifndef KIDO_UTILS_URDF_LOADER_H
#define KIDO_UTILS_URDF_LOADER_H

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <map>
#include <string>

#include "kido/common/Deprecated.h"
#include "kido/common/LocalResourceRetriever.h"
#include "kido/common/ResourceRetriever.h"
#include "kido/common/Uri.h"
#include "kido/dynamics/BodyNode.h"
#include "kido/dynamics/Joint.h"
#include "kido/dynamics/Skeleton.h"
#include "kido/simulation/World.h"
#include "kido/utils/CompositeResourceRetriever.h"
#include "kido/utils/PackageResourceRetriever.h"

namespace urdf
{
  class ModelInterface;
  class Link;
  class Joint;
  class Pose;
  class Vector3;
}

namespace kido {

namespace dynamics
{
  class Skeleton;
  class BodyNode;
  class Joint;
  class Shape;
}
namespace simulation
{
  class World;
}

namespace utils {

/**
 * @class DartLoader
 */
class DartLoader {
  public:
    /// Constructor with the default ResourceRetriever.
    DartLoader();

    /// Specify the directory of a ROS package. In your URDF files, you may see
    /// strings with a package URI pattern such as:
    ///
    /// @code
    /// "package://my_robot/meshes/mesh_for_my_robot.stl"
    ///  \______/  \______/\___________________________/
    ///      |        |                 |
    ///   package  package   file path with respect to
    ///   keyword   name       the package directory
    /// @endcode
    ///
    /// For us to successfully parse a URDF, we need to be told what the path
    /// to the package directory is, using addPackageDirectory(). In this case,
    /// suppose the path to the my_robot package is /path/to/my_robot. Then you
    /// should use addPackageDirectory("my_robot", "/path/to/my_robot").
    /// Altogether, this implies that a file named
    /// "/path/to/my_robot/meshes/mesh_for_my_robot.stl" exists. Whatever you
    /// specify as the package directory will end up replacing the 'package
    /// keyword' and 'package name' components of the URI string.
    void addPackageDirectory(const std::string& _packageName,
                             const std::string& _packageDirectory);

    /// Parse a file to produce a Skeleton
    dynamics::SkeletonPtr parseSkeleton(
      const common::Uri& _uri,
      const common::ResourceRetrieverPtr& _resourceRetriever = nullptr);

    /// Parse a text string to produce a Skeleton
    dynamics::SkeletonPtr parseSkeletonString(
      const std::string& _urdfString, const common::Uri& _baseUri,
      const common::ResourceRetrieverPtr& _resourceRetriever = nullptr);

    /// Parse a file to produce a World
    kido::simulation::WorldPtr parseWorld(const common::Uri& _uri,
      const common::ResourceRetrieverPtr& _resourceRetriever = nullptr);

    /// Parse a text string to produce a World
    kido::simulation::WorldPtr parseWorldString(
      const std::string& _urdfString, const common::Uri& _baseUri,
      const common::ResourceRetrieverPtr& _resourceRetriever = nullptr);

private:
    typedef std::shared_ptr<dynamics::BodyNode::Properties> BodyPropPtr;
    typedef std::shared_ptr<dynamics::Joint::Properties> JointPropPtr;

    static kido::dynamics::SkeletonPtr modelInterfaceToSkeleton(
      const urdf::ModelInterface* _model,
      const common::Uri& _baseUri,
      const common::ResourceRetrieverPtr& _resourceRetriever);

    static bool createSkeletonRecursive(
      dynamics::SkeletonPtr _skel,
      const urdf::Link* _lk,
      dynamics::BodyNode* _parent,
      const common::Uri& _baseUri,
      const common::ResourceRetrieverPtr& _resourceRetriever);

    template <class VisualOrCollision>
    static dynamics::ShapePtr createShape(const VisualOrCollision* _vizOrCol,
      const common::Uri& _baseUri,
      const common::ResourceRetrieverPtr& _resourceRetriever);

    static dynamics::BodyNode* createDartJointAndNode(
      const urdf::Joint* _jt,
      const dynamics::BodyNode::Properties& _body,
      dynamics::BodyNode* _parent,
      dynamics::SkeletonPtr _skeleton,
      const common::Uri& _baseUri,
      const common::ResourceRetrieverPtr& _resourceRetriever);

    static bool createDartNodeProperties(
      const urdf::Link* _lk,
      dynamics::BodyNode::Properties &properties,
      const common::Uri& _baseUri,
      const common::ResourceRetrieverPtr& _resourceRetriever);

    common::ResourceRetrieverPtr getResourceRetriever(
      const common::ResourceRetrieverPtr& _resourceRetriever);

    static Eigen::Isometry3d toEigen(const urdf::Pose& _pose);
    static Eigen::Vector3d toEigen(const urdf::Vector3& _vector);

    static bool readFileToString(
      const common::ResourceRetrieverPtr& _resourceRetriever,
      const common::Uri& _uri,
      std::string &_output);

    common::LocalResourceRetrieverPtr mLocalRetriever;
    utils::PackageResourceRetrieverPtr mPackageRetriever;
    utils::CompositeResourceRetrieverPtr mRetriever;
};

}
}

#endif /** KIDO_UTILS_URDF_LOADER_H */