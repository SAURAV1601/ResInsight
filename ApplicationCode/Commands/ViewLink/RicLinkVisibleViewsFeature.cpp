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

#include "RicLinkVisibleViewsFeature.h"

#include "RiaApplication.h"

#include "RicLinkVisibleViewsFeatureUi.h"

#include "RimViewLink.h"
#include "RimProject.h"
#include "RimView.h"
#include "RimViewLinker.h"
#include "RimViewLinkerCollection.h"

#include "RiuMainWindow.h"

#include "cafPdmUiPropertyViewDialog.h"
#include "cafPdmUiTreeView.h"

#include <QAction>
#include <QTreeView>
#include <QMessageBox>


CAF_CMD_SOURCE_INIT(RicLinkVisibleViewsFeature, "RicLinkVisibleViewsFeature");

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RicLinkVisibleViewsFeature::isCommandEnabled()
{
    RimProject* proj = RiaApplication::instance()->project();
    std::vector<RimView*> views;
    proj->allVisibleViews(views);

    if (views.size() > 1) return true;

    return false;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicLinkVisibleViewsFeature::onActionTriggered(bool isChecked)
{
    std::vector<RimView*> views;
    findNotLinkedVisibleViews(views);

    linkViews(views);
    return;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicLinkVisibleViewsFeature::setupActionLook(QAction* actionToSetup)
{
    actionToSetup->setText("Link Visible Views");
    actionToSetup->setIcon(QIcon(":/chain.png"));
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicLinkVisibleViewsFeature::allLinkedViews(std::vector<RimView*>& views)
{
    RimProject* proj = RiaApplication::instance()->project();
    if (proj->viewLinkerCollection()->viewLinker())
    {
        proj->viewLinkerCollection()->viewLinker()->allViews(views);
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicLinkVisibleViewsFeature::findNotLinkedVisibleViews(std::vector<RimView*> &views)
{
    RimProject* proj = RiaApplication::instance()->project();

    std::vector<RimView*> alreadyLinkedViews;
    allLinkedViews(alreadyLinkedViews);

    std::vector<RimView*> visibleViews;
    proj->allVisibleViews(visibleViews);

    for (size_t i = 0; i < visibleViews.size(); i++)
    {
        bool isLinked = false;
        for (size_t j = 0; j < alreadyLinkedViews.size(); j++)
        {
            if (visibleViews[i] == alreadyLinkedViews[j])
            {
                isLinked = true;
            }
        }

        if (!isLinked)
        {
            views.push_back(visibleViews[i]);
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RicLinkVisibleViewsFeature::linkViews(std::vector<RimView*>& views)
{
    RimProject* proj = RiaApplication::instance()->project();
    RimViewLinker* viewLinker = NULL;

    if (proj->viewLinkerCollection->viewLinker())
    {
        // We have a view linker, add not already linked views
        viewLinker = proj->viewLinkerCollection->viewLinker();

        for (size_t i = 0; i < views.size(); i++)
        {
            RimView* rimView = views[i];
            if (rimView == viewLinker->mainView()) continue;

            RimViewLink* viewLink = new RimViewLink;
            viewLink->setManagedView(rimView);

            viewLinker->viewLinks.push_back(viewLink);

            viewLink->initAfterReadRecursively();
            viewLink->updateOptionSensitivity();
            viewLink->updateUiIconFromActiveState();
        }
    }
    else
    {
        // Create a new view linker

        if (views.size() < 2)
        {
            return;
        }

        RicLinkVisibleViewsFeatureUi featureUi;
        featureUi.setViews(views);

        caf::PdmUiPropertyViewDialog propertyDialog(NULL, &featureUi, "Select Master View", "");
        propertyDialog.setWindowIcon(QIcon(":/chain.png"));
        if (propertyDialog.exec() != QDialog::Accepted) return;

        RimView* masterView = featureUi.masterView();
        viewLinker = new RimViewLinker;
        proj->viewLinkerCollection()->viewLinker = viewLinker;
        viewLinker->setMainView(masterView);

        for (size_t i = 0; i < views.size(); i++)
        {
            RimView* rimView = views[i];
            if (rimView == masterView) continue;

            RimViewLink* viewLink = new RimViewLink;
            viewLink->setManagedView(rimView);

            viewLinker->viewLinks.push_back(viewLink);

            viewLink->initAfterReadRecursively();
            viewLink->updateOptionSensitivity();
            viewLink->updateUiIconFromActiveState();
        }

        viewLinker->updateUiIcon();

    }

    viewLinker->applyAllOperations();
    proj->viewLinkerCollection.uiCapability()->updateConnectedEditors();
    proj->updateConnectedEditors();

    // Set managed view collection to selected and expanded in project tree
    caf::PdmUiTreeView* projTreeView = RiuMainWindow::instance()->projectTreeView();
    QModelIndex modIndex = projTreeView->findModelIndex(viewLinker);
    projTreeView->treeView()->setCurrentIndex(modIndex);

    projTreeView->treeView()->setExpanded(modIndex, true);
}
