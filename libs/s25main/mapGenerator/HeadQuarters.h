// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include "helpers/mathFuncs.h"
#include "mapGenerator/Map.h"
#include "mapGenerator/RandomUtility.h"
#include "mapGenerator/TextureHelper.h"
#include <stdexcept>

namespace rttr { namespace mapGenerator {

    /**
     * Find the largest connected area on the map. An area still counts as "connected" when it's divided by a tiny river
     * or small mountains. However, all nodes within a connected area should be reachable for any player. Even though
     * tiny rivers do not disconnect an area, water and mountain tiles are not considered part of the area.
     *
     * @param map reference to the map to look for the largest connected area
     *
     * @returns all map points within the largest connected area of the map.
     */
    std::vector<MapPoint> FindLargestConnectedArea(const Map& map);

    /**
     * Finds the most suitable position for a HQ in the specified area of the map. To find the most suitable position
     * for the entire map just leave the area empty. The resulting HQ positions are sorted by quality (highest quality
     * first). Good HQ positions are positions which are far away from other HQs and in a widely buildable area.
     *
     * @param map map to search for suitable HQ positions
     * @param area area within the HQ position should be
     * @param playerDistanceToMountains desired player distance to mountain
     *
     * @return all suitable HQ positions witihn the specified area (or the entire map if the area is empty).
     * @throw runtime_error
     */
    template<class T_Container>
    std::vector<MapPoint> FindHqPositions(const Map& map, const T_Container& area, unsigned playerDistanceToMountains)
    {
        if(area.empty())
        {
            throw std::runtime_error("Could not find any valid HQ position!");
        }

        const auto isValid = [](const MapPoint& pt) { return pt.isValid(); };
        const auto isObstacle = [&map](const MapPoint& pt) {
            return map.textureMap.Any(pt, [](auto t) { return !t.Is(ETerrain::Buildable); });
        };
        const auto isMountain = [&map](const MapPoint& pt) { return map.textureMap.Any(pt, IsMinableMountain); };
        std::vector<MapPoint> hqs;
        std::copy_if(map.hqPositions.begin(), map.hqPositions.end(), std::back_inserter(hqs), isValid);

        // Too consider a MapPoint as possible HQ position it requires:
        // 1. a minimum distances to obstacle
        // 2. a maximum distances to mountains
        // Quality of a possible HQ position depends on the distance to other HQ positions.
        NodeMapBase<unsigned> potentialHqQuality = DistancesTo(hqs, map.size);
        const auto obstacleDistance = DistancesTo(map.size, isObstacle);
        const auto minObstacleDistance = helpers::clamp(GetMaximum(obstacleDistance, area), 2u, 4u);
        const auto mountainDistance = DistancesTo(map.size, isMountain);
        const auto minMountainDistance = std::max(GetMinimum(mountainDistance, area), playerDistanceToMountains);
        const auto maxMountainDistance = static_cast<unsigned>(map.size.x + map.size.y);
        const auto noMountainsAvailable = minMountainDistance > maxMountainDistance;

        std::vector<MapPoint> positions;
        auto allowedMountainDistance = minMountainDistance;
        while(positions.empty())
        {
            for(const MapPoint& pt : area)
            {
                const auto mountainsInReach = mountainDistance[pt] < allowedMountainDistance + 5 && mountainDistance[pt] > allowedMountainDistance - 5;

                if(obstacleDistance[pt] >= minObstacleDistance && (noMountainsAvailable || mountainsInReach))
                {
                    positions.push_back(pt);
                }
            }
            if(allowedMountainDistance < maxMountainDistance)
            {
                allowedMountainDistance++;
            } else
            {
                // fall back to ignore desired mountain distance
                for(const MapPoint& pt : area)
                {
                    if(obstacleDistance[pt] >= minObstacleDistance)
                    {
                        positions.push_back(pt);
                    }
                }
                break;
            }
        }

        const auto isBetter = [&potentialHqQuality](MapPoint p1, MapPoint p2) {
            return potentialHqQuality[p1] > potentialHqQuality[p2];
        };
        std::sort(positions.begin(), positions.end(), isBetter);
        return positions;
    }

    /**
     * Tries to place a head quarter (HQ) for a single player within the specified area.
     *
     * @param map reference to the map to place the HQ on
     * @param index player index for the HQ
     * @param area area to place the HQ in
     * @param playerDistanceToMountains desired player distance to mountain
     *
     * @throw runtime_error
     */
    template<class T_Container>
    void PlaceHeadquarter(Map& map, int index, const T_Container& area, unsigned playerDistanceToMountains)
    {
        const auto positions = FindHqPositions(map, area, playerDistanceToMountains);
        if(positions.empty())
        {
            throw std::runtime_error("Could not find any valid HQ position!");
        }
        map.hqPositions[index] = positions.front();
    }

    /**
     * Tries to place a number of headquarters on the specified map.
     *
     * @param map map to place head quarters for all players on
     * @param rnd random number generated used for retrying HQ placement on failures
     * @param number number of HQs to place - equal to the number of players
     * @param retries number of retries to place valid HQs on this map
     * @param playerDistanceToMountains desired player distance to mountain
     *
     * @throw runtime_error
     */
    void PlaceHeadquarters(Map& map, RandomUtility& rnd, int number, unsigned playerDistanceToMountains, int retries = 10);

}} // namespace rttr::mapGenerator
