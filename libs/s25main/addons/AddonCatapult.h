#pragma once

#include "AddonList.h"
#include <boost/format.hpp>

const std::array<unsigned, 5> SUPPRESS_UNUSED catapultRange = {{14, 16, 18, 20, 9999}};

/**
 *  Addon for changing gold deposits to other resources or
 *  to remove them completely
 */
class AddonCatapult : public AddonList
{
public:
    AddonCatapult()
        : AddonList(AddonId::CATAPULT, AddonGroup::Military, "Change catapult range",
                    "Looking for description...",
                    {
                      (boost::format(_("%1%")) % catapultRange[0]).str(),
                      (boost::format(_("%1%")) % catapultRange[1]).str(),
                      (boost::format(_("%1%")) % catapultRange[2]).str(),
                      (boost::format(_("%1%")) % catapultRange[3]).str(),
                      (boost::format(_("%1%")) % catapultRange[4]).str(),
                    })
    {}
};
