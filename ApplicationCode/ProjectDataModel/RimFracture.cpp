/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2016-     Statoil ASA
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

#include "RimFracture.h"

#include "RimFractureDefinition.h"
#include "RimWellPath.h"

#include "cafPdmObject.h"


namespace caf 
{
    template<>
    void caf::AppEnum< RimFracture::FractureWellEnum>::setUp()
    {
        addItem(RimFracture::FRACTURE_SIMULATION_WELL,  "SIMULATION_WELL", "Simulation Well");
        addItem(RimFracture::FRACTURE_WELL_PATH,        "WELL_PATH", "Well Path");

        setDefault(RimFracture::FRACTURE_SIMULATION_WELL);
    }
}


CAF_PDM_SOURCE_INIT(RimFracture, "Fracture");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimFracture::RimFracture(void)
{
    CAF_PDM_InitObject("Fracture", "", "", "");

    CAF_PDM_InitField(&name,    "UserDescription", QString("Fracture Name"), "Name", "", "", "");
    CAF_PDM_InitField(&welltype,"Type", caf::AppEnum<FractureWellEnum>(FRACTURE_SIMULATION_WELL), "Type", "", "", "");

    CAF_PDM_InitField(         &measuredDepth,  "MeasuredDepth",    650.0f, "Measured Depth Location (if along well path)", "", "", "");
    CAF_PDM_InitFieldNoDefault(&wellpath,       "WellPath",                 "Well path for measured deph", "", "", "");

    CAF_PDM_InitField(&i,               "I",                1,      "Fracture location cell I", "", "", "");
    CAF_PDM_InitField(&j,               "J",                1,      "Fracture location cell J", "", "", "");
    CAF_PDM_InitField(&k,               "K",                1,      "Fracture location cell K", "", "", "");

    CAF_PDM_InitFieldNoDefault(&fractureDefinition, "FractureDef", "FractureDef", "", "", "");

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RimFracture::~RimFracture()
{
}
 
//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RimFracture::defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering)
{
    uiOrdering.add(&name);

    caf::PdmUiGroup* geometryGroup = uiOrdering.addNewGroup("Fractures");
    geometryGroup->add(&fractureDefinition);
    geometryGroup->add(&welltype);

    if (welltype == FRACTURE_WELL_PATH)
    {
        geometryGroup->add(&wellpath);
        geometryGroup->add(&measuredDepth);
    }

    else if (welltype == FRACTURE_SIMULATION_WELL)
    {
        geometryGroup->add(&i);
        geometryGroup->add(&j);
        geometryGroup->add(&k);

    }

    uiOrdering.setForgetRemainingFields(true);

}