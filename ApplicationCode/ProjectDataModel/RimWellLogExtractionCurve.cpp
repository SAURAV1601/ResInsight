/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2015-     Statoil ASA
//  Copyright (C) 2015-     Ceetron Solutions AS
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

#include "RimWellLogExtractionCurve.h"

#include "RiaApplication.h"

#include "RigCaseCellResultsData.h"
#include "RigEclipseCaseData.h"
#include "RigEclipseWellLogExtractor.h"
#include "RigFemPartResultsCollection.h"
#include "RigGeoMechCaseData.h"
#include "RigGeoMechWellLogExtractor.h"
#include "RigResultAccessorFactory.h"
#include "RigWellLogCurveData.h"
#include "RigWellPath.h"

#include "RimEclipseCase.h"
#include "RimEclipseCellColors.h"
#include "RimEclipseResultDefinition.h"
#include "RimEclipseView.h"
#include "RimGeoMechCase.h"
#include "RimGeoMechResultDefinition.h"
#include "RimGeoMechView.h"
#include "RimOilField.h"
#include "RimProject.h"
#include "RimTools.h"
#include "RimWellLogCurve.h"
#include "RimWellLogPlot.h"
#include "RimWellLogPlotCollection.h"
#include "RimWellLogTrack.h"
#include "RimWellPath.h"
#include "RimWellPathCollection.h"

#include "RiuLineSegmentQwtPlotCurve.h"
#include "RiuWellLogTrack.h"

#include "cafPdmUiTreeOrdering.h"
#include "cafUtils.h"

#include <cmath>

//==================================================================================================
///  
///  
//==================================================================================================

CAF_PDM_SOURCE_INIT(RimWellLogExtractionCurve, "RimWellLogExtractionCurve");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellLogExtractionCurve::RimWellLogExtractionCurve()
{
    CAF_PDM_InitObject("Well Log Curve", "", "", "");

    CAF_PDM_InitFieldNoDefault(&m_wellPath, "CurveWellPath", "Well Path", "", "", "");
    m_wellPath.uiCapability()->setUiTreeChildrenHidden(true);
    //m_wellPath.uiCapability()->setUiHidden(true);

    CAF_PDM_InitFieldNoDefault(&m_case, "CurveCase", "Case", "", "", "");
    m_case.uiCapability()->setUiTreeChildrenHidden(true);
    //m_case.uiCapability()->setUiHidden(true);
    CAF_PDM_InitFieldNoDefault(&m_eclipseResultDefinition, "CurveEclipseResult", "", "", "", "");
    m_eclipseResultDefinition.uiCapability()->setUiHidden(true);
    m_eclipseResultDefinition.uiCapability()->setUiTreeChildrenHidden(true);
    m_eclipseResultDefinition = new RimEclipseResultDefinition;

    CAF_PDM_InitFieldNoDefault(&m_geomResultDefinition, "CurveGeomechResult", "", "", "", "");
    m_geomResultDefinition.uiCapability()->setUiHidden(true);
    m_geomResultDefinition.uiCapability()->setUiTreeChildrenHidden(true);
    m_geomResultDefinition = new RimGeoMechResultDefinition;

    CAF_PDM_InitField(&m_timeStep, "CurveTimeStep", 0,"Time Step", "", "", "");

    // Add some space before name to indicate these belong to the Auto Name field
    CAF_PDM_InitField(&m_addCaseNameToCurveName, "AddCaseNameToCurveName", true, "   Case Name", "", "", "");
    CAF_PDM_InitField(&m_addPropertyToCurveName, "AddPropertyToCurveName", true, "   Property", "", "", "");
    CAF_PDM_InitField(&m_addWellNameToCurveName, "AddWellNameToCurveName", true, "   Well Name", "", "", "");
    CAF_PDM_InitField(&m_addTimestepToCurveName, "AddTimestepToCurveName", false, "   Timestep", "", "", "");
    CAF_PDM_InitField(&m_addDateToCurveName, "AddDateToCurveName", true, "   Date", "", "", "");
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellLogExtractionCurve::~RimWellLogExtractionCurve()
{
    delete m_geomResultDefinition;
    delete m_eclipseResultDefinition;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::setWellPath(RimWellPath* wellPath)
{
    m_wellPath = wellPath;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimWellPath* RimWellLogExtractionCurve::wellPath() const
{
    return m_wellPath;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::setCase(RimCase* rimCase)
{
    m_case = rimCase;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimCase* RimWellLogExtractionCurve::rimCase() const
{
    return m_case;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::setPropertiesFromView(RimView* view)
{
    m_case = view ? view->ownerCase() : NULL;

    RimGeoMechCase* geomCase = dynamic_cast<RimGeoMechCase*>(m_case.value());
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());
    m_eclipseResultDefinition->setEclipseCase(eclipseCase);
    m_geomResultDefinition->setGeoMechCase(geomCase);

    RimEclipseView* eclipseView = dynamic_cast<RimEclipseView*>(view);
    if (eclipseView)
    {
        m_eclipseResultDefinition->simpleCopy(eclipseView->cellResult());

        m_timeStep = eclipseView->currentTimeStep();
    }

    RimGeoMechView* geoMechView = dynamic_cast<RimGeoMechView*>(view);
    if (geoMechView)
    {
        m_geomResultDefinition->setResultAddress(geoMechView->cellResultResultDefinition()->resultAddress());
        m_timeStep = geoMechView->currentTimeStep();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::clampTimestep()
{
    if (m_case)
    {
        if (m_timeStep > m_case->timeStepStrings().size() - 1)
        {
            m_timeStep = m_case->timeStepStrings().size() - 1;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue)
{
    RimWellLogCurve::fieldChangedByUi(changedField, oldValue, newValue);

    if (changedField == &m_case)
    {
        clampTimestep();

        this->loadDataAndUpdate();
    }    
    else if (changedField == &m_wellPath)
    {
        this->loadDataAndUpdate();
    }
    else if (changedField == &m_timeStep)
    {
        this->loadDataAndUpdate();
    }

    if (changedField == &m_addCaseNameToCurveName ||
        changedField == &m_addPropertyToCurveName ||
        changedField == &m_addWellNameToCurveName ||
        changedField == &m_addTimestepToCurveName ||
        changedField == &m_addDateToCurveName)
    {
        this->uiCapability()->updateConnectedEditors();
        updateCurveName();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::onLoadDataAndUpdate()
{
    RimWellLogCurve::updateCurvePresentation();

    if (isCurveVisible())
    {
        // Make sure we have set correct case data into the result definitions.

        RimGeoMechCase* geomCase = dynamic_cast<RimGeoMechCase*>(m_case.value());
        RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());
        m_eclipseResultDefinition->setEclipseCase(eclipseCase);
        m_geomResultDefinition->setGeoMechCase(geomCase);

        RimWellLogPlotCollection* wellLogCollection = NULL;
        this->firstAncestorOrThisOfType(wellLogCollection);
        CVF_ASSERT(wellLogCollection);
        if (!wellLogCollection) return;

        cvf::ref<RigEclipseWellLogExtractor> eclExtractor = wellLogCollection->findOrCreateExtractor(m_wellPath, eclipseCase);
        cvf::ref<RigGeoMechWellLogExtractor> geomExtractor = wellLogCollection->findOrCreateExtractor(m_wellPath, geomCase);

        std::vector<double> values;
        std::vector<double> measuredDepthValues;
        std::vector<double> tvDepthValues;

        RimDefines::DepthUnitType depthUnit = RimDefines::UNIT_METER;

        if (eclExtractor.notNull())
        {
            measuredDepthValues = eclExtractor->measuredDepth();
            tvDepthValues = eclExtractor->trueVerticalDepth();

            m_eclipseResultDefinition->loadResult();

            cvf::ref<RigResultAccessor> resAcc = RigResultAccessorFactory::createFromResultDefinition(eclipseCase->eclipseCaseData(),
                                                                                                      0,
                                                                                                      m_timeStep,
                                                                                                      m_eclipseResultDefinition);

            if (resAcc.notNull())
            {
                eclExtractor->curveData(resAcc.p(), &values);
            }

            RigEclipseCaseData::UnitsType eclipseUnitsType = eclipseCase->eclipseCaseData()->unitsType();
            if (eclipseUnitsType == RigEclipseCaseData::UNITS_FIELD)
            {
                // See https://github.com/OPM/ResInsight/issues/538
                
                depthUnit = RimDefines::UNIT_FEET;
            }
        }
        else if (geomExtractor.notNull()) // geomExtractor
        {

            measuredDepthValues =  geomExtractor->measuredDepth();
            tvDepthValues = geomExtractor->trueVerticalDepth();

            m_geomResultDefinition->loadResult();

            geomExtractor->curveData(m_geomResultDefinition->resultAddress(), m_timeStep, &values);
        }

        m_curveData = new RigWellLogCurveData;
        if (values.size() && measuredDepthValues.size())
        {
            if (!tvDepthValues.size())
            {
                m_curveData->setValuesAndMD(values, measuredDepthValues, depthUnit, true);
            }
            else
            {
                m_curveData->setValuesWithTVD(values, measuredDepthValues, tvDepthValues, depthUnit);
            }
        }

        RimDefines::DepthUnitType displayUnit = RimDefines::UNIT_METER;

        RimWellLogPlot* wellLogPlot;
        firstAncestorOrThisOfType(wellLogPlot);
        CVF_ASSERT(wellLogPlot);
        if (!wellLogPlot) return;

        displayUnit = wellLogPlot->depthUnit();

        if(wellLogPlot->depthType() == RimWellLogPlot::TRUE_VERTICAL_DEPTH)
        {
            m_qwtPlotCurve->setSamples(m_curveData->xPlotValues().data(), m_curveData->trueDepthPlotValues(displayUnit).data(), static_cast<int>(m_curveData->xPlotValues().size()));
        }
        else if (wellLogPlot->depthType() == RimWellLogPlot::MEASURED_DEPTH)
        {
            m_qwtPlotCurve->setSamples(m_curveData->xPlotValues().data(), m_curveData->measuredDepthPlotValues(displayUnit).data(), static_cast<int>(m_curveData->xPlotValues().size()));
        }

        m_qwtPlotCurve->setLineSegmentStartStopIndices(m_curveData->polylineStartStopIndices());

        updateZoomInParentPlot();

        setLogScaleFromSelectedResult();

        if (m_parentQwtPlot) m_parentQwtPlot->replot();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimWellLogExtractionCurve::calculateValueOptions(const caf::PdmFieldHandle* fieldNeedingOptions, bool * useOptionsOnly)
{
   QList<caf::PdmOptionItemInfo> options;

   options = RimWellLogCurve::calculateValueOptions(fieldNeedingOptions, useOptionsOnly);
   if (options.size() > 0) return options;

    if (fieldNeedingOptions == &m_wellPath)
    {
        RimTools::wellPathOptionItems(&options);

        if (options.size() > 0)
        {
            options.push_front(caf::PdmOptionItemInfo("None", nullptr));
        }
    }
    else if (fieldNeedingOptions == &m_case)
    {
        RimTools::caseOptionItems(&options);

        if (options.size() > 0)
        {
            options.push_front(caf::PdmOptionItemInfo("None", nullptr));
        }
    }
    else if (fieldNeedingOptions == &m_timeStep)
    {
        QStringList timeStepNames;

        if (m_case)
        {
            timeStepNames = m_case->timeStepStrings();
        }

        for (int i = 0; i < timeStepNames.size(); i++)
        {
            options.push_back(caf::PdmOptionItemInfo(timeStepNames[i], i));
        }
    }

    return options;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering)
{
    RimPlotCurve::updateOptionSensitivity();

    caf::PdmUiGroup* curveDataGroup = uiOrdering.addNewGroup("Curve Data");

    curveDataGroup->add(&m_wellPath);
    curveDataGroup->add(&m_case);

    RimGeoMechCase* geomCase = dynamic_cast<RimGeoMechCase*>(m_case.value());
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());

    if (eclipseCase)
    {
        curveDataGroup->add(&(m_eclipseResultDefinition->m_resultTypeUiField));
        curveDataGroup->add(&(m_eclipseResultDefinition->m_porosityModelUiField));
        curveDataGroup->add(&(m_eclipseResultDefinition->m_resultVariableUiField));

        if (m_eclipseResultDefinition->hasDynamicResult())
        {
            curveDataGroup->add(&m_timeStep);
        }
    }
    else if (geomCase)
    {
        curveDataGroup->add(&(m_geomResultDefinition->m_resultPositionTypeUiField));
        curveDataGroup->add(&(m_geomResultDefinition->m_resultVariableUiField));

        curveDataGroup->add(&m_timeStep);
    }

    caf::PdmUiGroup* appearanceGroup = uiOrdering.addNewGroup("Appearance");
    appearanceGroup->add(&m_curveColor);
    appearanceGroup->add(&m_curveThickness);
    appearanceGroup->add(&m_pointSymbol);
    appearanceGroup->add(&m_symbolSkipPixelDistance);
    appearanceGroup->add(&m_lineStyle);
    appearanceGroup->add(&m_curveName);
    appearanceGroup->add(&m_isUsingAutoName);
    if (m_isUsingAutoName)
    {
        appearanceGroup->add(&m_addWellNameToCurveName);
        appearanceGroup->add(&m_addCaseNameToCurveName);
        appearanceGroup->add(&m_addPropertyToCurveName);
        appearanceGroup->add(&m_addDateToCurveName);
        appearanceGroup->add(&m_addTimestepToCurveName);
    }


    uiOrdering.skipRemainingFields(true);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::initAfterRead()
{
    RimWellLogCurve::initAfterRead();

    RimGeoMechCase* geomCase = dynamic_cast<RimGeoMechCase*>(m_case.value());
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());

    m_eclipseResultDefinition->setEclipseCase(eclipseCase);
    m_geomResultDefinition->setGeoMechCase(geomCase);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::defineUiTreeOrdering(caf::PdmUiTreeOrdering& uiTreeOrdering, QString uiConfigName /*= ""*/)
{
    uiTreeOrdering.skipRemainingChildren(true);
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimWellLogExtractionCurve::setLogScaleFromSelectedResult()
{
    QString resVar = m_eclipseResultDefinition->resultVariable();

    if (resVar.toUpper().contains("PERM"))
    {
        RimWellLogTrack* track = NULL;
        this->firstAncestorOrThisOfType(track);
        if (track)
        {
            if (track->curveCount() == 1)
            {
                track->setLogarithmicScale(true);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimWellLogExtractionCurve::createCurveAutoName()
{
    RimGeoMechCase* geomCase = dynamic_cast<RimGeoMechCase*>(m_case.value());
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());
    QString generatedCurveName;

    if (m_addWellNameToCurveName && m_wellPath)
    {
        generatedCurveName += wellName();
    }

    if (m_addCaseNameToCurveName && m_case())
    {
        if (!generatedCurveName.isEmpty())
        {
            generatedCurveName += ", ";
        }

        generatedCurveName += m_case->caseUserDescription();
    }

    if (m_addPropertyToCurveName)
    {
        if (!generatedCurveName.isEmpty())
        {
            generatedCurveName += ",";
        }

        generatedCurveName += wellLogChannelName();
    }

    if (m_addTimestepToCurveName || m_addDateToCurveName)
    {
        size_t maxTimeStep = 0;
        
        if (eclipseCase)
        {
            RifReaderInterface::PorosityModelResultType porosityModel = RigCaseCellResultsData::convertFromProjectModelPorosityModel(m_eclipseResultDefinition->porosityModel());
            if (eclipseCase->eclipseCaseData())
            {
                maxTimeStep = eclipseCase->eclipseCaseData()->results(porosityModel)->maxTimeStepCount();
            }
        }
        else if (geomCase)
        {
            if (geomCase->geoMechData())
            {
                maxTimeStep = geomCase->geoMechData()->femPartResults()->frameCount();
            }
        }

        if (m_addDateToCurveName)
        {
            QString dateString = wellDate();
            if (!dateString.isEmpty())
            {
                if (!generatedCurveName.isEmpty())
                {
                    generatedCurveName += ", ";
                }

                generatedCurveName += dateString;
            }
        }

        if (m_addTimestepToCurveName)
        {
            if (!generatedCurveName.isEmpty())
            {
                generatedCurveName += ", ";
            }

            generatedCurveName += QString("[%1/%2]").arg(m_timeStep()).arg(maxTimeStep);
        }
    }

    return generatedCurveName;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimWellLogExtractionCurve::wellLogChannelName() const
{
    RimGeoMechCase* geoMechCase = dynamic_cast<RimGeoMechCase*>(m_case.value());
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());

    QString name;
    if (eclipseCase)
    {
        name = caf::Utils::makeValidFileBasename( m_eclipseResultDefinition->resultVariableUiName());
    }
    else if (geoMechCase)
    {
        QString resCompName = m_geomResultDefinition->resultComponentUiName();
        if (resCompName.isEmpty())
        {
            name = m_geomResultDefinition->resultFieldUiName();
        }
        else
        {
            name = m_geomResultDefinition->resultFieldUiName() + "." + resCompName;
        }
    }

    return name;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimWellLogExtractionCurve::wellName() const
{
    if (m_wellPath)
    {
        return m_wellPath->name();
    }
    else
    {
        return QString();
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimWellLogExtractionCurve::wellDate() const
{
    RimGeoMechCase* geomCase = dynamic_cast<RimGeoMechCase*>(m_case.value());
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());

    QStringList timeStepNames;

    if (eclipseCase)
    {
        RifReaderInterface::PorosityModelResultType porosityModel = RigCaseCellResultsData::convertFromProjectModelPorosityModel(m_eclipseResultDefinition->porosityModel());
        if (eclipseCase->eclipseCaseData())
        {
            timeStepNames = eclipseCase->timeStepStrings();
        }
    }
    else if (geomCase)
    {
        if (geomCase->geoMechData())
        {
            timeStepNames = geomCase->timeStepStrings();
        }
    }

    return (m_timeStep < timeStepNames.size()) ? timeStepNames[m_timeStep] : "";
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RimWellLogExtractionCurve::isEclipseCurve() const
{
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>(m_case.value());
    if (eclipseCase)
    {
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
QString RimWellLogExtractionCurve::caseName() const
{
    if (m_case)
    {
        return m_case->caseUserDescription();
    }

    return QString();
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
double RimWellLogExtractionCurve::rkbDiff() const
{
    if (m_wellPath && m_wellPath->wellPathGeometry())
    {
        RigWellPath* geo = m_wellPath->wellPathGeometry();

        if (geo->hasDatumElevation())
        {
            return geo->datumElevation();
        }

        // If measured depth is zero, use the z-value of the well path points
        if (geo->m_wellPathPoints.size() > 0 && geo->m_measuredDepths.size() > 0)
        {
            double epsilon = 1e-3;

            if (cvf::Math::abs(geo->m_measuredDepths[0]) < epsilon)
            {
                double diff = geo->m_measuredDepths[0] - (-geo->m_wellPathPoints[0].z());

                return diff;
            }
        }
    }

    return HUGE_VAL;
}
