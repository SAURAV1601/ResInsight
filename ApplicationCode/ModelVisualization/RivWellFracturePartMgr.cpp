/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017     Statoil ASA
// 
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
// 
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
// 
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "RivWellFracturePartMgr.h"

#include "RimEclipseView.h"
#include "RimEclipseWell.h"
#include "RimFracture.h"

#include "cafEffectGenerator.h"

#include "cafDisplayCoordTransform.h"
#include "cvfDrawableGeo.h"
#include "cvfModelBasicList.h"
#include "cvfPart.h"
#include "cvfPrimitiveSet.h"
#include "cvfPrimitiveSetIndexedUInt.h"
#include "cvfScalarMapperContinuousLinear.h"
#include "RimStimPlanFractureTemplate.h"
#include "RimLegendConfig.h"


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RivWellFracturePartMgr::RivWellFracturePartMgr(RimFracture* fracture)
    : m_rimFracture(fracture)
{
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RivWellFracturePartMgr::~RivWellFracturePartMgr()
{

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivWellFracturePartMgr::updatePartGeometry(caf::DisplayCoordTransform* displayCoordTransform)
{
    if (m_part.notNull()) return;
    if (!displayCoordTransform) return;

    if (m_rimFracture)
    {
        if (!m_rimFracture->hasValidGeometry())
        {
            m_rimFracture->computeGeometry();
        }

        if (!m_rimFracture->hasValidGeometry()) return;

        const std::vector<cvf::Vec3f>& nodeCoords = m_rimFracture->nodeCoords();
        const std::vector<cvf::uint>& triangleIndices = m_rimFracture->triangleIndices();
        std::vector<cvf::Vec3f> displayCoords;

        for (size_t i = 0; i < nodeCoords.size(); i++)
        {
            cvf::Vec3d nodeCoordsDouble = static_cast<cvf::Vec3d>(nodeCoords[i]);
            cvf::Vec3d displayCoordsDouble = displayCoordTransform->transformToDisplayCoord(nodeCoordsDouble);
            displayCoords.push_back(static_cast<cvf::Vec3f>(displayCoordsDouble));
        }

        cvf::ref<cvf::DrawableGeo> geo = createGeo(triangleIndices, displayCoords);

        m_part = new cvf::Part;
        m_part->setDrawable(geo.p());

        caf::SurfaceEffectGenerator surfaceGen(cvf::Color4f(cvf::Color3f(cvf::Color3::BROWN)), caf::PO_1);
        cvf::ref<cvf::Effect> eff = surfaceGen.generateCachedEffect();

        m_part->setEffect(eff.p());
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivWellFracturePartMgr::updatePartGeometryTexture(caf::DisplayCoordTransform* displayCoordTransform)
{
    if (m_part.notNull()) return;
    if (!displayCoordTransform) return;

    if (m_rimFracture)
    {
        if (!m_rimFracture->hasValidGeometry())
        {
            m_rimFracture->computeGeometry();
        }

        if (!m_rimFracture->hasValidGeometry()) return;

        RimFractureTemplate * fracTemplate = m_rimFracture->attachedFractureDefinition();
        RimStimPlanFractureTemplate* stimPlanFracTemplate;

        if (dynamic_cast<RimStimPlanFractureTemplate*>(fracTemplate))
        {
            stimPlanFracTemplate = dynamic_cast<RimStimPlanFractureTemplate*>(fracTemplate);
        }
        else return;

        int timeStepIndex = m_rimFracture->stimPlanTimeIndexToPlot;
        std::vector<std::vector<double> > dataToPlot;

        if (m_rimFracture->stimPlanParameterToPlot == RimFracture::CONDUCTIVITY)
        {
            dataToPlot = stimPlanFracTemplate->getConductivitiesAtTimeStep(timeStepIndex);
        }
        else if (m_rimFracture->stimPlanParameterToPlot == RimFracture::PERMEABILITY)
        {
            dataToPlot = stimPlanFracTemplate->getPermeabilitiesAtTimeStep(timeStepIndex);

        }
        else if (m_rimFracture->stimPlanParameterToPlot == RimFracture::WIDTH)
        {
            dataToPlot = stimPlanFracTemplate->getWidthsAtTimeStep(timeStepIndex);
        }

        if (dataToPlot.empty()) return; //TODO: Set all values to undefined if no data available...

        const std::vector<cvf::Vec3f>& nodeCoords = m_rimFracture->nodeCoords();
        const std::vector<cvf::uint>& triangleIndices = m_rimFracture->triangleIndices();
        std::vector<cvf::Vec3f> displayCoords;

        for (size_t i = 0; i < nodeCoords.size(); i++)
        {
            cvf::Vec3d nodeCoordsDouble = static_cast<cvf::Vec3d>(nodeCoords[i]);
            cvf::Vec3d displayCoordsDouble = displayCoordTransform->transformToDisplayCoord(nodeCoordsDouble);
            displayCoords.push_back(static_cast<cvf::Vec3f>(displayCoordsDouble));
        }

        cvf::ref<cvf::DrawableGeo> geo = createGeo(triangleIndices, displayCoords);

        m_part = new cvf::Part;
        m_part->setDrawable(geo.p());

        RimLegendConfig* legend = m_rimFracture->activeLegend();
        cvf::ScalarMapper* scalarMapper =  legend->scalarMapper();
//         cvf::ref<cvf::ScalarMapperContinuousLinear> scalarMapper = new cvf::ScalarMapperContinuousLinear;
//         {
//             cvf::Color3ubArray legendColors;
//             legendColors.resize(4);
//             legendColors[0] = cvf::Color3::GRAY;
//             legendColors[1] = cvf::Color3::GREEN;
//             legendColors[2] = cvf::Color3::BLUE;
//             legendColors[3] = cvf::Color3::RED;
//             scalarMapper->setColors(legendColors);
//             scalarMapper->setRange(0.0, 4.0);
//             scalarMapper->setLevelCount(4, true);
//         }

        //double scalarValue = i % 4;

        cvf::ref<cvf::Vec2fArray> textureCoords = new cvf::Vec2fArray;
        textureCoords->resize(nodeCoords.size());


        int i = 0;
        for (std::vector<double> depthData : dataToPlot)
        {
            std::vector<double> mirroredValuesAtDepth = mirrorDataAtSingleDepth(depthData);
            for (double gridXdata : mirroredValuesAtDepth)
            {
                cvf::Vec2f texCoord = scalarMapper->mapToTextureCoord(gridXdata);

                textureCoords->set(i, texCoord);
                i++;
            }
        }
     
        geo->setTextureCoordArray(textureCoords.p());

        caf::ScalarMapperEffectGenerator scalarMapperEffectGenerator(scalarMapper, caf::PO_NEG_LARGE);
        cvf::ref<cvf::Effect> eff = scalarMapperEffectGenerator.generateUnCachedEffect();

        m_part->setEffect(eff.p());
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
std::vector<double> RivWellFracturePartMgr::mirrorDataAtSingleDepth(std::vector<double> depthData)
{
    std::vector<double> mirroredValuesAtGivenDepth;
    mirroredValuesAtGivenDepth.push_back(depthData[0]);
    for (int i = 1; i < (depthData.size()); i++) //starting at 1 since we don't want center value twice
    {
        double valueAtGivenX = depthData[i];
        mirroredValuesAtGivenDepth.insert(mirroredValuesAtGivenDepth.begin(), valueAtGivenX);
        mirroredValuesAtGivenDepth.push_back(valueAtGivenX);
    }

    return mirroredValuesAtGivenDepth;
}



//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivWellFracturePartMgr::appendGeometryPartsToModel(cvf::ModelBasicList* model, caf::DisplayCoordTransform* displayCoordTransform)
{
    clearGeometryCache();

    if (m_part.isNull())
    {
        if (m_rimFracture->attachedFractureDefinition())
        {
            if (dynamic_cast<RimStimPlanFractureTemplate*>(m_rimFracture->attachedFractureDefinition()))
            {
                updatePartGeometryTexture(displayCoordTransform);
            }
            else
            {
                updatePartGeometry(displayCoordTransform);
            }
        }
    }

    if (m_part.notNull())
    {
        model->addPart(m_part.p());
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RivWellFracturePartMgr::clearGeometryCache()
{
    m_part = nullptr;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
cvf::ref<cvf::DrawableGeo> RivWellFracturePartMgr::createGeo(const std::vector<cvf::uint>& triangleIndices, const std::vector<cvf::Vec3f>& nodeCoords)
{
    cvf::ref<cvf::DrawableGeo> geo = new cvf::DrawableGeo;

    cvf::ref<cvf::UIntArray> indices = new cvf::UIntArray(triangleIndices);
    cvf::ref<cvf::Vec3fArray> vertices = new cvf::Vec3fArray(nodeCoords);

    geo->setVertexArray(vertices.p());
    geo->addPrimitiveSet(new cvf::PrimitiveSetIndexedUInt(cvf::PT_TRIANGLES, indices.p()));
    geo->computeNormals();

    return geo;
}
