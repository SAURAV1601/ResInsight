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

#include "qwt_plot.h"

class RimWellLogPlotTrack;
class QwtPlotGrid;
class QwtLegend;

class QEvent;

//==================================================================================================
//
//
//
//==================================================================================================
class RiuWellLogTrackPlot : public QwtPlot
{
    Q_OBJECT

public:
    RiuWellLogTrackPlot(RimWellLogPlotTrack* plotTrackDefinition, QWidget* parent = NULL);
    virtual ~RiuWellLogTrackPlot();

    void setDepthRange(double minDepth, double maxDepth);
    void setDepthTitle(const QString& title);

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event);
    virtual void focusInEvent(QFocusEvent* event);
    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

private:
    void setDefaults();
    void selectClosestCurve(const QPoint& pos);

private:
    RimWellLogPlotTrack*    m_plotTrackDefinition;
    QwtPlotGrid*            m_grid;
    QwtLegend*              m_legend;
};
