#pragma once


#include "GltfAnimation.h"
#include "GltfRendering.h"
#include "Logging.h"
#include "Polar.h"

#include <graphics/AppInterface.h>
#include <graphics/CameraUtilities.h>
#include <graphics/Timer.h>

#include <renderer/Uniforms.h>

#include <math/Box.h>

// Dirty include, to get the GLFW input definitions
#include <GLFW/glfw3.h>


namespace ad {
namespace gltfviewer {


struct MeshInstances
{
    Mesh mesh;
    std::vector<InstanceList::Instance> instances; 
};


using MeshRepository = std::map<arte::gltf::Index<arte::gltf::Mesh>,
                                MeshInstances>;
using AnimationRepository = std::vector<Animation>;


void clearInstances(MeshRepository & aRepository)
{
    for(auto & [index, mesh] : aRepository)
    {
        mesh.instances.clear();
    }
}


/// \brief Associate a gltf::mesh index to a viewer's Mesh instance.
template <class T_nodeRange>
void populateMeshRepository(MeshRepository & aRepository, const T_nodeRange & aNodes)
{
    for (arte::Owned<arte::gltf::Node> node : aNodes)
    {
        if(node->mesh && !aRepository.contains(*node->mesh))
        {
            auto [it, didInsert] = 
                aRepository.emplace(*node->mesh,
                                    MeshInstances{.mesh =prepare(node.get(&arte::gltf::Node::mesh))});
            ADLOG(gPrepareLogger, info)("Completed GPU loading for mesh '{}'.", it->second.mesh);
        }
        populateMeshRepository(aRepository, node.iterate(&arte::gltf::Node::children));
    }
}


/// \brief Create viewer's Animation instances for each animation in the provided range.
template <class T_animationRange>
void populateAnimationRepository(AnimationRepository & aRepository, const T_animationRange & aAnimations)
{
    for (arte::Owned<arte::gltf::Animation> animation : aAnimations)
    {
        aRepository.push_back(prepare(animation));
    }
}


struct Scene
{
    Scene(arte::Gltf aGltf,
          arte::gltf::Index<arte::gltf::Scene> aSceneIndex,
          std::shared_ptr<graphics::AppInterface> aAppInterface) :
        gltf{std::move(aGltf)},
        scene{gltf.get(aSceneIndex)},
        appInterface{std::move(aAppInterface)}
    {
        populateMeshRepository(indexToMeshes, scene.iterate(&arte::gltf::Scene::nodes));
        populateAnimationRepository(animations, gltf.getAnimations());
        if (!animations.empty())
        {
            activeAnimation = &animations.front();
        }

        const math::Box<GLfloat> aProjectedBox =
            graphics::getViewVolume(appInterface->getWindowSize(), 2.f, 0.f, gViewedDepth);
        renderer.setProjectionTransformation(math::trans3d::orthographicProjection(aProjectedBox));
        
        // Not enabled by default OpenGL context.
        glEnable(GL_DEPTH_TEST);

        using namespace std::placeholders;
        appInterface->registerKeyCallback(
            std::bind(&Scene::callbackKeyboard, this, _1, _2, _3, _4));
        appInterface->registerMouseButtonCallback(
            std::bind(&Scene::callbackMouseButton, this, _1, _2, _3, _4, _5));
        appInterface->registerCursorPositionCallback(
            std::bind(&Scene::callbackCursorPosition, this, _1, _2));
    }

    void update(const graphics::Timer & aTimer)
    {
        updateCamera();
        updateAnimation(aTimer);
        updatesInstances();
    }

    void updateCamera()
    {
        const math::Position<3, GLfloat> cameraCartesian = cameraPosition.toCartesian();
        ADLOG(gDrawLogger, trace)("Camera position {}.", cameraCartesian);

        math::Vec<3, GLfloat> gazeDirection = gGazePoint - cameraCartesian;
        renderer.setCameraTransformation(
            graphics::getCameraTransform(cameraCartesian,
                                         gazeDirection,
                                         cameraPosition.getUpTangent()));
    }

    void updateAnimation(const graphics::Timer & aTimer)
    {
        if(activeAnimation != nullptr)
        {
            Animation & animation = *activeAnimation;
            for (const auto & [nodeIndex, nodeChannels] : animation.nodeToChannels)
            {
                auto node = scene.get(nodeIndex);
                arte::gltf::Node::TRS * trs = 
                    std::get_if<arte::gltf::Node::TRS>(&node->transformation);

                // TODO Ad 2022/03/22: I suspect it is legal to animate a channel for
                // a node where it was not explicitly specified in the gltf.
                if(trs == nullptr)
                {
                    ADLOG(gDrawLogger, critical)
                         ("Unsupported: Node #{} animates a transformation channel, but did not specify any of TRS.",
                          node.id());
                    throw std::logic_error{"Animation on a node without any of TRS."};
                }

                for (auto nodeChannel : nodeChannels)
                {
                    switch(nodeChannel.path)
                    {
                    case arte::gltf::Target::Path::Translation:
                        nodeChannel.sampler->interpolate(aTimer.time(), trs->translation);
                        break;
                    case arte::gltf::Target::Path::Rotation:
                        nodeChannel.sampler->interpolate(aTimer.time(), trs->rotation);
                        break;
                    case arte::gltf::Target::Path::Scale:
                        nodeChannel.sampler->interpolate(aTimer.time(), trs->scale);
                        break;
                    }
                }
            }
        }
    }

    void updatesInstances()
    {
        clearInstances(indexToMeshes);
        for (auto node : scene.iterate(&arte::gltf::Scene::nodes))
        {
            updatesInstances(node);
        }

        for(auto & [index, pair] : indexToMeshes)
        {
            // Update the VBO containing instance data with the client vector of instance data
            pair.mesh.gpuInstances.update(pair.instances);
        }
    }

    // Recursive function to:
    // * queue the mesh instance
    // * traverse the node children
    void updatesInstances(arte::Owned<arte::gltf::Node> aNode,
                           math::AffineMatrix<4, float> aParentTransform = 
                               math::AffineMatrix<4, float>::Identity())
    {
        math::AffineMatrix<4, float> modelTransform = getLocalTransform(aNode);

        if(aNode->mesh)
        {
            indexToMeshes.at(*aNode->mesh).instances.push_back({modelTransform});
        }

        for (auto node : aNode.iterate(&arte::gltf::Node::children))
        {
            updatesInstances(node, modelTransform);
        }
    }


    void render() const
    {
        // TODO should it be optimized to only call when there is at least once instance?
        for(const auto & [index, mesh] : indexToMeshes)
        {
            renderer.render(mesh.mesh);
        }
    }


    void callbackKeyboard(int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            appInterface->requestCloseApplication();
        }

        else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        {
            switch(polygonMode)
            {
            case PolygonMode::Fill:
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                polygonMode = PolygonMode::Line;
                break;
            case PolygonMode::Line:
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                polygonMode = PolygonMode::Fill;
                break;
            }
        }
    }

    void callbackMouseButton(int button, int action, int mods, double xpos, double ypos)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        {
            previousDragPosition = math::Position<2, GLfloat>{(GLfloat)xpos, (GLfloat)ypos};
        }
        else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        {
            previousDragPosition = std::nullopt;
        }
    }

    void callbackCursorPosition(double xpos, double ypos)
    {
        using Radian = math::Radian<GLfloat>;
        // top-left corner origin
        if (previousDragPosition)
        {
            math::Position<2, GLfloat> cursorPosition{(GLfloat)xpos, (GLfloat)ypos};
            auto angularIncrements = (cursorPosition - *previousDragPosition).cwMul(gMouseControlFactor);

            // The viewed object should turn in the direction of the mouse,
            // so the camera angles are changed in the opposite direction (hence the substractions).
            cameraPosition.azimuthal -= Radian{angularIncrements.x()};
            cameraPosition.polar -= Radian{angularIncrements.y()};
            cameraPosition.polar = std::max(Radian{0}, std::min(Radian{math::pi<GLfloat>}, cameraPosition.polar));

            previousDragPosition = cursorPosition;
        }
    }

    enum class PolygonMode
    {
        Fill,
        Line,
    };

    arte::Gltf gltf;
    arte::Owned<arte::gltf::Scene> scene;
    MeshRepository indexToMeshes;
    AnimationRepository animations;
    Animation * activeAnimation{nullptr};
    Polar cameraPosition{2.f};
    Renderer renderer;
    PolygonMode polygonMode{PolygonMode::Fill};
    std::shared_ptr<graphics::AppInterface> appInterface;
    std::optional<math::Position<2, GLfloat>> previousDragPosition;

    static constexpr math::Position<3, GLfloat> gGazePoint{0.f, 0.f, 0.f};
    static constexpr math::Vec<2, GLfloat> gMouseControlFactor{1/700.f, 1/700.f};
    static constexpr GLfloat gViewedDepth = 10000;
};

} // namespace gltfviewer
} // namespace ad
