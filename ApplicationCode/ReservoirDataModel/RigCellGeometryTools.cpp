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


#include "RigCellGeometryTools.h"
#include "cvfStructGrid.h"
#include "cvfGeometryTools.h"

#include "cafHexGridIntersectionTools/cafHexGridIntersectionTools.h"
#include "cvfBoundingBox.h"

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RigCellGeometryTools::RigCellGeometryTools()
{

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
RigCellGeometryTools::~RigCellGeometryTools()
{

}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool RigCellGeometryTools::planeHexCellIntersection(cvf::Vec3d * hexCorners, cvf::Plane fracturePlane, std::list<std::pair<cvf::Vec3d, cvf::Vec3d > > & intersectionLineSegments)
{
    bool isCellIntersected = false;
    for (int face = 0; face < 6; ++face)
    {
        cvf::ubyte faceVertexIndices[4];
        cvf::StructGridInterface::cellFaceVertexIndices(static_cast<cvf::StructGridInterface::FaceType>(face), faceVertexIndices);

        cvf::Vec3d faceCenter = cvf::GeometryTools::computeFaceCenter(hexCorners[faceVertexIndices[0]], hexCorners[faceVertexIndices[1]], hexCorners[faceVertexIndices[2]], hexCorners[faceVertexIndices[3]]);

        for (int i = 0; i < 4; i++)
        {
            int next = i < 3 ? i + 1 : 0;
            caf::HexGridIntersectionTools::ClipVx triangleIntersectionPoint1;
            caf::HexGridIntersectionTools::ClipVx triangleIntersectionPoint2;

            bool isMostVxesOnPositiveSideOfP1 = false;

            bool isIntersectingPlane = caf::HexGridIntersectionTools::planeTriangleIntersection(fracturePlane,
                hexCorners[faceVertexIndices[i]], 0,
                hexCorners[faceVertexIndices[next]], 1,
                faceCenter, 2,
                &triangleIntersectionPoint1, &triangleIntersectionPoint2, &isMostVxesOnPositiveSideOfP1);

            if (isIntersectingPlane)
            {
                isCellIntersected = true;
                intersectionLineSegments.push_back({ triangleIntersectionPoint1.vx, triangleIntersectionPoint2.vx });
            }
        }
    }    return isCellIntersected;
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void RigCellGeometryTools::createPolygonFromLineSegments(std::list<std::pair<cvf::Vec3d, cvf::Vec3d>> &intersectionLineSegments, std::vector<std::vector<cvf::Vec3d>> &polygons)
{
    bool startNewPolygon = true;
    while (!intersectionLineSegments.empty())
    {
        if (startNewPolygon)
        {
            std::vector<cvf::Vec3d> polygon;
            //Add first line segments to polygon and remove from list
            std::pair<cvf::Vec3d, cvf::Vec3d > linesegment = intersectionLineSegments.front();
            polygon.push_back(linesegment.first);
            polygon.push_back(linesegment.second);
            intersectionLineSegments.remove(linesegment);
            polygons.push_back(polygon);
            startNewPolygon = false;
        }

        std::vector<cvf::Vec3d>& polygon = polygons.back();

        //Search remaining list for next point...

        bool isFound = false;
        float tolerance = 0.0001f;

        for (std::list<std::pair<cvf::Vec3d, cvf::Vec3d > >::iterator lIt = intersectionLineSegments.begin(); lIt != intersectionLineSegments.end(); lIt++)
        {
            cvf::Vec3d lineSegmentStart = lIt->first;
            cvf::Vec3d lineSegmentEnd = lIt->second;
            cvf::Vec3d polygonEnd = polygon.back();
            if (((lineSegmentStart - polygonEnd).lengthSquared() < tolerance*tolerance))
            {
                polygon.push_back(lIt->second);
                intersectionLineSegments.erase(lIt);
                isFound = true;
                break;
            }

            if (((lineSegmentEnd - polygonEnd).lengthSquared() < tolerance*tolerance))
            {
                polygon.push_back(lIt->first);
                intersectionLineSegments.erase(lIt);
                isFound = true;
                break;
            }
        }

        if (isFound)
        {
            continue;
        }
        else
        {
            startNewPolygon = true;
        }
    }



}

//--------------------------------------------------------------------------------------------------
///  
//--------------------------------------------------------------------------------------------------
void RigCellGeometryTools::findCellLocalXYZ(cvf::Vec3d * hexCorners, cvf::Vec3d &localXdirection, cvf::Vec3d &localYdirection, cvf::Vec3d &localZdirection)
{
    cvf::ubyte faceVertexIndices[4];
    cvf::StructGridInterface::FaceEnum face;

    face = cvf::StructGridInterface::NEG_I;
    cvf::StructGridInterface::cellFaceVertexIndices(face, faceVertexIndices);
    cvf::Vec3d faceCenterNegI = cvf::GeometryTools::computeFaceCenter(hexCorners[faceVertexIndices[0]], hexCorners[faceVertexIndices[1]], hexCorners[faceVertexIndices[2]], hexCorners[faceVertexIndices[3]]);
    //TODO: Should we use face centroids instead of face centers?

    face = cvf::StructGridInterface::POS_I;
    cvf::StructGridInterface::cellFaceVertexIndices(face, faceVertexIndices);
    cvf::Vec3d faceCenterPosI = cvf::GeometryTools::computeFaceCenter(hexCorners[faceVertexIndices[0]], hexCorners[faceVertexIndices[1]], hexCorners[faceVertexIndices[2]], hexCorners[faceVertexIndices[3]]);

    face = cvf::StructGridInterface::NEG_J;
    cvf::StructGridInterface::cellFaceVertexIndices(face, faceVertexIndices);
    cvf::Vec3d faceCenterNegJ = cvf::GeometryTools::computeFaceCenter(hexCorners[faceVertexIndices[0]], hexCorners[faceVertexIndices[1]], hexCorners[faceVertexIndices[2]], hexCorners[faceVertexIndices[3]]);

    face = cvf::StructGridInterface::POS_J;
    cvf::StructGridInterface::cellFaceVertexIndices(face, faceVertexIndices);
    cvf::Vec3d faceCenterPosJ = cvf::GeometryTools::computeFaceCenter(hexCorners[faceVertexIndices[0]], hexCorners[faceVertexIndices[1]], hexCorners[faceVertexIndices[2]], hexCorners[faceVertexIndices[3]]);
    
    cvf::Vec3d faceCenterCenterVectorI = faceCenterPosI - faceCenterNegI;
    cvf::Vec3d faceCenterCenterVectorJ = faceCenterPosJ - faceCenterNegJ;

    localZdirection.cross(faceCenterCenterVectorI, faceCenterCenterVectorJ);
    localZdirection.normalize();

    cvf::Vec3d crossPoductJZ;
    crossPoductJZ.cross(faceCenterCenterVectorJ, localZdirection);
    localXdirection = faceCenterCenterVectorI + crossPoductJZ;
    localXdirection.normalize();

    cvf::Vec3d crossPoductIZ;
    crossPoductIZ.cross(faceCenterCenterVectorI, localZdirection);
    localYdirection = faceCenterCenterVectorJ - crossPoductIZ;
    localYdirection.normalize();
    
    //TODO: Check if we end up with 0-vectors, and handle this case...
}

//--------------------------------------------------------------------------------------------------
///  
//--------------------------------------------------------------------------------------------------
void RigCellGeometryTools::polygonAreaWeightedLength(cvf::Vec3d directionOfLength, std::vector<cvf::Vec3d> polygon2d)
{
    //TODO: Check that polygon is in xy plane

    //Find bounding box
    cvf::BoundingBox polygonBBox;
    for (cvf::Vec3d nodeCoord : polygon2d) polygonBBox.add(nodeCoord);
    cvf::Vec3d bboxCorners[8];
    polygonBBox.cornerVertices(bboxCorners);


    //Split bounding box in multplie polygons (2D)
    int resolutionOfLengthCalc = 20;


    //Use clipper to find overlap between bbpolygon and fracture

    //Calculate length (max-min) and area

    //Calculate area-weighted average of above vectors. 


}