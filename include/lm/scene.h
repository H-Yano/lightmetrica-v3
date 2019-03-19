/*
    Lightmetrica - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#include "component.h"
#include "math.h"
#include "surface.h"
#include <variant>

LM_NAMESPACE_BEGIN(LM_NAMESPACE)

// ----------------------------------------------------------------------------

/*!
    \addtogroup scene
    @{
*/

/*!
    \brief Result of ray sampling.

    \rst
    This structure represents the result of ray sampling
    used by the functions of :cpp:class:`lm::Scene` class.
    \endrst
*/
struct RaySample {
    SurfacePoint sp;   //!< Surface point information.
    Vec3 wo;           //!< Sampled direction.
    Vec3 weight;       //!< Contribution divided by probability.

    /*!
        \brief Get a ray from the sample.
        
        \rst
        This function constructs a :cpp:class:`lm::Ray`
        structure from the ray sample.
        \endrst
    */
    Ray ray() const {
        assert(!sp.geom.infinite);
        return { sp.geom.p, wo };
    }
};

// ----------------------------------------------------------------------------

/*!
    \brief Scene primitive.

    \rst
    This structure represents a scene primitive. The scene is described by a set of primitives
    and a primitive describes an object in the scene,
    which associates various scene components like mesh or material.
    A primitive can represent three types of scene objects.

    (1) *Scene geometry*.
        If ``mesh != nullptr`` and ``material != nullptr``,
        the structure describes a geometry in the scene,
        represented by an association of a mesh and a material.
        A transformation is applied to the mesh.

    (2) *Light*.
        If ``light != nullptr``, the structure describes a light in the scene.
        Note that a light can also be a scene geometry, such as area lights.

    (3) *Camera*.
        If ``camera != nullptr``, the structure describes a camera in the scene.
        Note that a camera and a light cannot be the same primitive,
        that is, ``light`` and ``camera`` cannot be non-``nullptr`` in the same time.

    A set of primitives is managed internally by the implementation of :cpp:class:`lm::Scene`
    and the class exposes a facade for the sampling and evaluation functions
    for the underlying component interfaces.
    Thus the users usually do not need to explicitly access the underlying
    component interfaces of a primitive.
    \endrst
*/
struct Primitive {
    int index;                      //!< Primitive index.
    Transform transform;            //!< Transformation associated to the primitive.
    Mesh* mesh = nullptr;           //!< Underlying mesh.
    Material* material = nullptr;   //!< Underlying material.
    Light* light = nullptr;         //!< Underlying light.
    Camera* camera = nullptr;       //!< Underlying camera.

    template <typename Archive>
    void serialize(Archive& ar) {
        ar(index, transform, mesh, material, light, camera);
    }
};

// ----------------------------------------------------------------------------

/*!
    \brief Scene.

    \rst
    This class represent a component interface for a scene.
    A scene is responsible for sampling of a ray emitted from a point inside a scene,
    evaluation of directional terms given a point in the scene,
    ray-scene intersection, visibility query, etc.
    The class is a basic building block to construct your own renderer.
    For detail, see TODO.
    \endrst
*/
class Scene : public Component {
public:
    /*!
        \brief Check if the scene is renderable.

        \rst
        This function returns true if the scene is renderable.
        If not, the function returns false with error messages.
        \endrst
    */
    virtual bool renderable() const = 0;

    /*!
        \brief Load scene primitive(s).
        \param transform Transformation associated to the primitive.
        \param prop Property containing references to the scene components.

        \rst
        This function construct a primitive and add it to the scene
        given the transformation and the references specified in ``prop``.
        The type of the primitive created by this function changes according to
        the properties in ``prop``.
        The function returns true if the loading is a success.
        \endrst
    */
    virtual bool loadPrimitive(Mat4 transform, const Json& prop) = 0;

    /*!
        \brief Callback function to process a triangle.
        \param primitive Primitive index.
        \param face Face index.
        \param p1 First position.
        \param p2 Second position.
        \param p3 Third position.
        
        \rst
        The function of this type is used as an argument of
        :cpp:func:`lm::Scene::foreachTriangle` function.
        \endrst
    */
    using ProcessTriangleFunc = std::function<void(int primitive, int face, Vec3 p1, Vec3 p2, Vec3 p3)>;

    /*!
        \brief Enumerate triangles in the scene.
        \param processTriangle Callback function to process a triangle.

        \rst
        This function enumerates triangles of all the transformed meshes in the scene.
        The specified callback function is called per a triangle.
        The primitive and face indices associated to the triangle is also given.
        \endrst
    */
    virtual void foreachTriangle(const ProcessTriangleFunc& processTriangle) const = 0;

    /*!
        \brief Iterate primitives in the scene.
    */
    using ProcessPrimitiveFunc = std::function<void(const Primitive& primitive)>;
    virtual void foreachPrimitive(const ProcessPrimitiveFunc& processPrimitive) const = 0;

    // ------------------------------------------------------------------------

    /*!
        \brief Build acceleration structure.
        \param name Name of the acceleration structure.
        \param prop Property for configuration.
    */
    virtual void build(const std::string& name, const Json& prop) = 0;

    /*!
        \brief Compute closest intersection point.
    */
    virtual std::optional<SurfacePoint> intersect(
        Ray ray, Float tmin = Eps, Float tmax = Inf) const = 0;

    /*!
        \brief Check if two surface points are mutually visible.
    */
    bool visible(const SurfacePoint& sp1, const SurfacePoint& sp2) const {
        const auto visible_ = [this](const SurfacePoint& sp1, const SurfacePoint& sp2) -> bool {
            assert(!sp1.geom.infinite);
            const auto wo = sp2.geom.infinite
                ? -sp2.geom.wo
                : glm::normalize(sp2.geom.p - sp1.geom.p);
            const auto tmax = sp2.geom.infinite
                ? Inf - 1_f
                : [&]() {
                    const auto d = glm::distance(sp1.geom.p, sp2.geom.p);
                    return d * (1_f - Eps);
                }();
            // Exclude environent light from intersection test with tmax < Inf
            return !intersect(Ray{sp1.geom.p, wo}, Eps, tmax);
        };
        if (sp1.geom.infinite) {
            return visible_(sp2, sp1);
        }
        else {
            return visible_(sp1, sp2);
        }
    }

    // ------------------------------------------------------------------------

    /*!
        \brief Check if given surface point is light.
    */
    virtual bool isLight(const SurfacePoint& sp) const = 0;

    /*!
        \brief Check if given surface point is specular.
    */
    virtual bool isSpecular(const SurfacePoint& sp) const = 0;

    // ------------------------------------------------------------------------

    /*!
        \brief Generate a primary ray.
    */
    virtual Ray primaryRay(Vec2 rp, Float aspectRatio) const = 0;

    /*!
        \brief Sample a ray given surface point and incident direction.
        \rst
        (x,wo) ~ p(x,wo|sp,wi)
        \endrst
    */
    virtual std::optional<RaySample> sampleRay(Rng& rng, const SurfacePoint& sp, Vec3 wi) const = 0;

    /*!
        \brief Sample a ray given pixel position.
        \rst
        (x,wo) ~ p(x,wo|raster window)
        \endrst
    */
    virtual std::optional<RaySample> samplePrimaryRay(Rng& rng, Vec4 window, Float aspectRatio) const = 0;

    /*!
        \brief Sample a position on a light.
    */
    virtual std::optional<RaySample> sampleLight(Rng& rng, const SurfacePoint& sp) const = 0;

    /*!
        \brief Evaluate pdf for direction sampling.
    */
    virtual Float pdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const = 0;

    /*!
        \brief Evaluate pdf for light sampling.
    */
    virtual Float pdfLight(const SurfacePoint& sp, const SurfacePoint& spL, Vec3 wo) const = 0;

    // ------------------------------------------------------------------------

    /*!
        \brief Evaluate extended BSDF.
    */
    virtual Vec3 evalBsdf(const SurfacePoint& sp, Vec3 wi, Vec3 wo) const = 0;

    /*!
        \brief Evaluate endpoint contribution.
        \rst
        f(x,wo) where x is endpoint
        \endrst
    */
    virtual Vec3 evalContrbEndpoint(const SurfacePoint& sp, Vec3 wo) const = 0;

    /*!
        \brief Evaluate reflectance (if available).
        \rst
        \rho(x)
        \endrst
    */
    virtual std::optional<Vec3> reflectance(const SurfacePoint& sp) const = 0;
};

/*!
    @}
*/

// ----------------------------------------------------------------------------

LM_NAMESPACE_END(LM_NAMESPACE)
