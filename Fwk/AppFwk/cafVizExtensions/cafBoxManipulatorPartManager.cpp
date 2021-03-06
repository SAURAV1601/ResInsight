
#include "cafBoxManipulatorPartManager.h"

#include "cafBoxManipulatorGeometryGenerator.h"
#include "cafEffectGenerator.h"

#include "cvfBoxGenerator.h"
#include "cvfDrawableGeo.h"
#include "cvfGeometryBuilderFaceList.h"
#include "cvfModelBasicList.h"
#include "cvfPart.h"
#include "cvfPrimitiveSetIndexedUInt.h"
#include "cvfPrimitiveSetIndexedUShort.h"
#include "cvfRay.h"
#include "cvfPrimitiveSetDirect.h"
#include "cvfHitItem.h"
#include <QDebug>


using namespace cvf;

namespace caf {


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
BoxManipulatorPartManager::BoxManipulatorPartManager() 
    : m_sizeOnStartManipulation(cvf::Vec3d::UNDEFINED),
      m_originOnStartManipulation(cvf::Vec3d::UNDEFINED),
      m_currentHandleIndex(cvf::UNDEFINED_SIZE_T)
{
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
BoxManipulatorPartManager::~BoxManipulatorPartManager()
{
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void BoxManipulatorPartManager::setOrigin(const cvf::Vec3d& origin)
{
    if (isManipulatorActive()) return;

    m_origin = origin;
    if (m_originOnStartManipulation.isUndefined()) m_originOnStartManipulation = origin;

    clearAllGeometryAndParts();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void BoxManipulatorPartManager::setSize(const cvf::Vec3d& size)
{
    if(isManipulatorActive()) return;

    m_size = size;
    if (m_sizeOnStartManipulation.isUndefined()) m_sizeOnStartManipulation = m_size;
    
    clearAllGeometryAndParts();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void BoxManipulatorPartManager::originAndSize(cvf::Vec3d* origin, cvf::Vec3d* size)
{
    *origin = m_origin;
    *size = m_size;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool BoxManipulatorPartManager::isManipulatorActive() const
{
    return m_currentHandleIndex != cvf::UNDEFINED_SIZE_T;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void BoxManipulatorPartManager::appendPartsToModel(cvf::ModelBasicList* model)
{
    if (m_boundingBoxPart.isNull())
    {
        recreateAllGeometryAndParts();
    }

    CVF_ASSERT(m_boundingBoxPart.notNull());
    model->addPart(m_boundingBoxPart.p());

    for (size_t i = 0; i < m_handleParts.size(); i++)
    {
        model->addPart(m_handleParts.at(i));
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void BoxManipulatorPartManager::tryToActivateManipulator(const cvf::HitItem* hitItem)
{
    endManipulator();

    if (!hitItem) return;

    const cvf::Part* pickedPart = hitItem->part();
    const cvf::Vec3d intersectionPoint = hitItem->intersectionPoint();

    if (!pickedPart) return;

    const cvf::Object* siConstObj = pickedPart->sourceInfo();
    cvf::Object* siObj = const_cast<cvf::Object*>(siConstObj);

    BoxManipulatorSourceInfo* candidateSourceInfo = dynamic_cast<BoxManipulatorSourceInfo*>(siObj);
    if (!candidateSourceInfo) return;

    for (size_t i = 0; i < m_handleParts.size(); i++)
    {
        cvf::Part* part = m_handleParts.at(i);
        BoxManipulatorSourceInfo* si = static_cast<BoxManipulatorSourceInfo*>(part->sourceInfo());

        if (si->m_cubeFace == candidateSourceInfo->m_cubeFace &&
            si->m_cubeHandle == candidateSourceInfo->m_cubeHandle)
        {
            m_initialPickPoint = intersectionPoint;
            m_sizeOnStartManipulation = m_size;
            m_originOnStartManipulation = m_origin;
            m_currentHandleIndex = i;
        }
    }
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void BoxManipulatorPartManager::updateManipulatorFromRay(const cvf::Ray* ray)
{
    if (!isManipulatorActive()) return;
    if (m_boundingBoxPart.isNull()) return;
    

    BoxFace face = m_handleIds[m_currentHandleIndex].first;
    cvf::Vec3d faceDir = normalFromFace(face);

    cvf::Vec3d closestPointOnMouseRay;
    cvf::Vec3d closestPointOnHandleRay;
    BoxManipulatorPartManager::closestPointOfTwoLines(ray->origin(), ray->origin() + ray->direction(), 
                                                      m_initialPickPoint, m_initialPickPoint + faceDir, 
                                                      &closestPointOnMouseRay, &closestPointOnHandleRay);

    cvf::Vec3d newOrigin = m_origin;
    cvf::Vec3d newSize = m_size;

    int axis = face/2;
    cvf::Vec3d axisDir;
    axisDir[axis] = 1.0;

    cvf::Vec3d motion3d = closestPointOnHandleRay - m_initialPickPoint;

    if (face == BCF_X_POS || face == BCF_Y_POS || face == BCF_Z_POS)
    {
        newSize = m_sizeOnStartManipulation + motion3d;

        for (int i = 0; i < 3; ++i) if (newSize[i] < 0.0) { newOrigin[i] = m_originOnStartManipulation[i] + newSize[i]; newSize[i] = 0.0; }
    }
    else
    {
        newOrigin = m_originOnStartManipulation + motion3d;
        newSize   = m_sizeOnStartManipulation - motion3d;

        for (int i = 0; i < 3; ++i) if (newSize[i] < 0.0) {  newSize[i] = 0.0; }
    }

    m_origin = newOrigin;
    m_size = newSize;

    clearAllGeometryAndParts();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void BoxManipulatorPartManager::endManipulator()
{
    m_currentHandleIndex = cvf::UNDEFINED_SIZE_T;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
cvf::Vec3d BoxManipulatorPartManager::normalFromFace(BoxFace face)
{
    switch (face)
    {
    case caf::BoxManipulatorPartManager::BCF_X_POS:
        return cvf::Vec3d::X_AXIS;
        break;
    case caf::BoxManipulatorPartManager::BCF_X_NEG:
        return -cvf::Vec3d::X_AXIS;
        break;
    case caf::BoxManipulatorPartManager::BCF_Y_POS:
        return cvf::Vec3d::Y_AXIS;
        break;
    case caf::BoxManipulatorPartManager::BCF_Y_NEG:
        return -cvf::Vec3d::Y_AXIS;
        break;
    case caf::BoxManipulatorPartManager::BCF_Z_POS:
        return cvf::Vec3d::Z_AXIS;
        break;
    case caf::BoxManipulatorPartManager::BCF_Z_NEG:
        return -cvf::Vec3d::Z_AXIS;
        break;
    default:
        CVF_ASSERT(false);
        break;
    }

    return cvf::Vec3d::UNDEFINED;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void BoxManipulatorPartManager::createAllHandleParts()
{
    Vec3f cp[8];
    navCubeCornerPoints(cp);

    createCubeFaceHandlePart(BCF_Y_NEG, cp[0], cp[1], cp[5], cp[4]);
    createCubeFaceHandlePart(BCF_Y_POS, cp[2], cp[3], cp[7], cp[6]);

    createCubeFaceHandlePart(BCF_Z_POS, cp[4], cp[5], cp[6], cp[7]);
    createCubeFaceHandlePart(BCF_Z_NEG, cp[3], cp[2], cp[1], cp[0]);

    createCubeFaceHandlePart(BCF_X_NEG, cp[3], cp[0], cp[4], cp[7]);
    createCubeFaceHandlePart(BCF_X_POS, cp[1], cp[2], cp[6], cp[5]);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void BoxManipulatorPartManager::createCubeFaceHandlePart(BoxFace face, cvf::Vec3f p1, cvf::Vec3f p2, cvf::Vec3f p3, cvf::Vec3f p4)
{
    float handleSize = static_cast<float>(m_sizeOnStartManipulation.length() * 0.05);

    Vec3f center = (p1 + p2 + p3 + p4) / 4.0f;

    Vec3f nu = (p2 - p1).getNormalized();
    Vec3f nv = (p4 - p1).getNormalized();
    Vec3f nw = nu^nv;

    Vec3f u = nu * handleSize;
    Vec3f v = nv * handleSize;
    Vec3f w = nw * handleSize;

    Vec3f pi1 = center - u / 2.0f - v / 2.0f + w*0.025f;

    Vec3f pi2 = pi1 + u;
    Vec3f pi3 = pi2 + v;
    Vec3f pi4 = pi1 + v;
    Vec3f pi5 = center + w * 0.3f;

    ref<DrawableGeo> geo = createHandleGeo(pi1, pi2, pi3, pi4, pi5);

    cvf::ref<cvf::Part> handlePart = new cvf::Part;
    handlePart->setName("Box manipulator handle");
    handlePart->setDrawable(geo.p());

    handlePart->updateBoundingBox();

    caf::SurfaceEffectGenerator surfaceGen(cvf::Color3::MAGENTA, caf::PO_1);
    cvf::ref<cvf::Effect> eff = surfaceGen.generateCachedEffect();
    handlePart->setEffect(eff.p());

    handlePart->setSourceInfo(new BoxManipulatorSourceInfo(face, BCFI_CENTER));

    m_handleParts.push_back(handlePart.p());
    m_handleIds.push_back(std::make_pair(face, BCFI_CENTER));
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
cvf::ref<cvf::DrawableGeo> BoxManipulatorPartManager::createHandleGeo(const cvf::Vec3f& v0, 
                                                                      const cvf::Vec3f& v1,
                                                                      const cvf::Vec3f& v2,
                                                                      const cvf::Vec3f& v3,
                                                                      const cvf::Vec3f& v4)
{                                                                                        
    ref<DrawableGeo> geo = new DrawableGeo;

    ref<Vec3fArray> vertexArray = new Vec3fArray(18);

    vertexArray->set(0,  v0);
    vertexArray->set(1,  v1);
    vertexArray->set(2,  v2);
    vertexArray->set(3,  v0);
    vertexArray->set(4,  v2);
    vertexArray->set(5,  v3);
    vertexArray->set(6,  v0);
    vertexArray->set(7,  v1);
    vertexArray->set(8,  v4);
    vertexArray->set(9,  v1);
    vertexArray->set(10, v2);
    vertexArray->set(11, v4);
    vertexArray->set(12, v2);
    vertexArray->set(13, v3);
    vertexArray->set(14, v4);
    vertexArray->set(15, v3);
    vertexArray->set(16, v0);
    vertexArray->set(17, v4);

    geo->setVertexArray(vertexArray.p());
    ref<cvf::PrimitiveSetDirect> primSet = new cvf::PrimitiveSetDirect(cvf::PT_TRIANGLES);
    primSet->setIndexCount(18);

    geo->addPrimitiveSet(primSet.p());
    geo->computeNormals();

    return geo;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void BoxManipulatorPartManager::navCubeCornerPoints(cvf::Vec3f points[8])
{
    Vec3f offset(1.0, 1.0, 1.0);

    Vec3f min(m_origin);
    min -= offset;

    Vec3f max(m_origin + m_size);
    max += offset;

    points[0].set(min.x(), min.y(), min.z());
    points[1].set(max.x(), min.y(), min.z());
    points[2].set(max.x(), max.y(), min.z());
    points[3].set(min.x(), max.y(), min.z());
    points[4].set(min.x(), min.y(), max.z());
    points[5].set(max.x(), min.y(), max.z());
    points[6].set(max.x(), max.y(), max.z());
    points[7].set(min.x(), max.y(), max.z());
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void BoxManipulatorPartManager::clearAllGeometryAndParts()
{
    m_boundingBoxPart = nullptr;
    m_handleIds.clear();
    m_handleParts.clear();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void BoxManipulatorPartManager::recreateAllGeometryAndParts()
{
    createBoundingBoxPart();
    createAllHandleParts();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void BoxManipulatorPartManager::createBoundingBoxPart()
{
    m_boundingBoxPart = nullptr;

    cvf::ref<caf::BoxManipulatorGeometryGenerator> geometryGenerator = new caf::BoxManipulatorGeometryGenerator;
    geometryGenerator->setOrigin(m_origin);
    geometryGenerator->setSize(m_size);

    cvf::ref<cvf::DrawableGeo> geoMesh = geometryGenerator->createBoundingBoxMeshDrawable();
    if (geoMesh.notNull())
    {
        cvf::ref<cvf::Part> part = new cvf::Part;
        part->setName("Box Manipulator Mesh");
        part->setDrawable(geoMesh.p());

        part->updateBoundingBox();
        //                     part->setEnableMask(meshFaultBit);
        //                     part->setPriority(priMesh);

        cvf::ref<cvf::Effect> eff;
        caf::MeshEffectGenerator effectGenerator(cvf::Color3::MAGENTA);
        eff = effectGenerator.generateCachedEffect();
        part->setEffect(eff.p());

        m_boundingBoxPart = part;
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool BoxManipulatorPartManager::closestPointOfTwoLines(const cvf::Vec3d& L1p1, const cvf::Vec3d& L1p2, 
                                                       const cvf::Vec3d& L2p1, const cvf::Vec3d& L2p2, 
                                                       cvf::Vec3d* closestPointOnL1, cvf::Vec3d* closestPointOnL2)
{
    //    qDebug() << p1 << " " << q1 << " " << p2 << " " << q2;

    // Taken from Real-Time Collistion Detection, Christer Ericson, 2005, p146-147

    // L1(s) = P1 + sd1
    // L2(t) = P2 + td2

    // d1 = Q1-P1
    // d2 = Q2-P2

    // r = P1-P2

    // a = d1*d1
    // b = d1*d2
    // c = d1*r
    // e = d2*d2;
    // d = ae-b^2
    // f = d2*r

    // s = (bf-ce)/d
    // t = (af-bc)/d


    cvf::Vec3d d1 = L1p2 - L1p1;
    cvf::Vec3d d2 = L2p2 - L2p1;

    double a = d1.dot(d1);
    double b = d1.dot(d2);
    double e = d2.dot(d2);

    double d = a*e - b*b;

    if (d < std::numeric_limits<double>::epsilon())
    {
        // Parallel lines
        if (closestPointOnL1) *closestPointOnL1 = L1p1;
        if (closestPointOnL2) *closestPointOnL2 = L2p1;
        return false;
    }

    cvf::Vec3d r = L1p1 - L2p1;
    double c = d1.dot(r);
    double f = d2.dot(r);

    double s = (b*f - c*e) / d;
    double t = (a*f - b*c) / d;

    if (closestPointOnL1) *closestPointOnL1 = L1p1 + s*d1;
    if (closestPointOnL2) *closestPointOnL2 = L2p1 + t*d2;

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
    {
        return true;
    }
    else
    {
        return false;
    }
}

} // namespace cvf

