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

#pragma once

#include "RimWellLogPlotCurve.h"

#include "cafPdmPtrField.h"
#include "cafPdmField.h"

#include <vector>

class RimWellPath;
class RimWellLog;

//==================================================================================================
///  
///  
//==================================================================================================
class RimWellLogFileCurve : public RimWellLogPlotCurve
{
    CAF_PDM_HEADER_INIT;

public:
    RimWellLogFileCurve();
    virtual ~RimWellLogFileCurve();
    
    virtual void updatePlotData();

    void setWellPath(RimWellPath* wellPath);
    void setWellLogChannelName(const QString& name);

protected:
    virtual void fieldChangedByUi(const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue);
    virtual void defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering);
    virtual void defineUiTreeOrdering(caf::PdmUiTreeOrdering& uiTreeOrdering, QString uiConfigName = "");
    virtual QList<caf::PdmOptionItemInfo> calculateValueOptions(const caf::PdmFieldHandle* fieldNeedingOptions, bool * useOptionsOnly);

protected:
    caf::PdmPtrField<RimWellPath*>  m_wellPath;
    caf::PdmField<QString>          m_wellLogChannnelName;
};

