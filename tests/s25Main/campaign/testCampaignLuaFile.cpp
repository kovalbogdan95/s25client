// Copyright (C) 2005 - 2023 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "lua/CampaignDataLoader.h"
#include "gameData/CampaignDescription.h"
#include "gameData/SelectionMapInputData.h"
#include "rttr/test/LocaleResetter.hpp"
#include "rttr/test/LogAccessor.hpp"
#include "rttr/test/TmpFolder.hpp"
#include <s25util/utf8.h>
#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/test/unit_test.hpp>
#include <RttrConfig.h>

/// Require that the log contains "content"
#define REQUIRE_LOG_CONTAINS(content)                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        const std::string log = logAcc.getLog();                                                                       \
        BOOST_TEST_REQUIRE((log.find(content) != std::string::npos), "Unexpected log: " << log << "\n"                 \
                                                                                        << "Expected: " << (content)); \
                                                                                                                       \
    } while(false)

namespace bnw = boost::nowide;

BOOST_AUTO_TEST_SUITE(CampaignLuaFile)

BOOST_AUTO_TEST_CASE(ScriptVersion)
{
    // No getRequiredLuaVersion
    {
        rttr::test::TmpFolder tmp;
        {
            bnw::ofstream file(tmp.get() / "campaign.lua");
            file << "";
        }

        CampaignDescription desc;
        CampaignDataLoader loader(desc, tmp.get());
        rttr::test::LogAccessor logAcc;
        BOOST_TEST_REQUIRE(!loader.Load());
        RTTR_REQUIRE_LOG_CONTAINS(
          "Lua script did not provide the function getRequiredLuaVersion()! It is probably outdated.", false);
    }

    // Correct version
    {
        rttr::test::TmpFolder tmp;
        {
            bnw::ofstream file(tmp.get() / "campaign.lua");
            file << boost::format("function getRequiredLuaVersion()\n return %1%\n end")
                      % CampaignDataLoader::GetVersion();
        }

        CampaignDescription desc;
        CampaignDataLoader loader(desc, tmp.get());
        rttr::test::LogAccessor logAcc;
        BOOST_TEST_REQUIRE(!loader.Load());
        BOOST_TEST_REQUIRE(loader.CheckScriptVersion());
        logAcc.clearLog();
    }

    // Backwards compatibility: version 2 can load version 1
    {
        rttr::test::TmpFolder tmp;
        {
            bnw::ofstream file(tmp.get() / "campaign.lua");

            file << "campaign ={\
                version = \"1\",\
                author = \"Max Meier\",\
                name = \"Meine Kampagne\",\
                shortDescription = \"Sehr kurze Beschreibung\",\
                longDescription = \"Das ist die lange Beschreibung\",\
                image = \"<RTTR_GAME>/GFX/PICS/WORLD.LBM\",\
                maxHumanPlayers = 1,\
                difficulty = \"easy\",\
                mapFolder = \"<RTTR_GAME>/DATA/MAPS\",\
                luaFolder = \"<RTTR_GAME>/CAMPAIGNS/ROMAN\",\
                maps = { \"dessert0.WLD\", \"dessert1.WLD\", \"dessert2.WLD\"}\
            }\
            function getRequiredLuaVersion()\n return 1\n end";
        }

        CampaignDescription desc;
        CampaignDataLoader loader(desc, tmp.get());
        rttr::test::LogAccessor logAcc;
        BOOST_TEST_REQUIRE(loader.Load());
        BOOST_TEST_REQUIRE(loader.CheckScriptVersion());
        logAcc.clearLog();
    }

    // Wrong version
    {
        rttr::test::TmpFolder tmp;
        {
            bnw::ofstream file(tmp.get() / "campaign.lua");
            file << boost::format("function getRequiredLuaVersion()\n return %1%\n end")
                      % (CampaignDataLoader::GetVersion() + 1);
        }

        CampaignDescription desc;
        CampaignDataLoader loader(desc, tmp.get());
        rttr::test::LogAccessor logAcc;
        BOOST_TEST_REQUIRE(!loader.Load());
        RTTR_REQUIRE_LOG_CONTAINS((boost::format("Wrong lua script version: %1%. Current version: %2%.\n")
                                   % (CampaignDataLoader::GetVersion() + 1) % CampaignDataLoader::GetVersion())
                                    .str(),
                                  false);
    }
}

BOOST_AUTO_TEST_CASE(LoadCampaignDescriptionWithoutTranslation)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp.get() / "campaign.lua");

        file << "campaign ={\
            version = \"1\",\
            author = \"Max Meier\",\
            name = \"Meine Kampagne\",\
            shortDescription = \"Sehr kurze Beschreibung\",\
            longDescription = \"Das ist die lange Beschreibung\",\
            image = \"<RTTR_GAME>/GFX/PICS/WORLD.LBM\",\
            maxHumanPlayers = 1,\
            difficulty = \"easy\",\
            mapFolder = \"<RTTR_GAME>/DATA/MAPS\",\
            luaFolder = \"<RTTR_GAME>/CAMPAIGNS/ROMAN\",\
            maps = { \"dessert0.WLD\", \"dessert1.WLD\", \"dessert2.WLD\"}\
        }";

        file << "function getRequiredLuaVersion() return 1 end";
    }

    CampaignDescription desc;
    CampaignDataLoader loader(desc, tmp.get());
    BOOST_TEST_REQUIRE(loader.Load());

    // campaign description
    BOOST_TEST(desc.version == "1");
    BOOST_TEST(desc.author == "Max Meier");
    BOOST_TEST(desc.name == "Meine Kampagne");
    BOOST_TEST(desc.shortDescription == "Sehr kurze Beschreibung");
    BOOST_TEST(desc.longDescription == "Das ist die lange Beschreibung");
    BOOST_TEST(desc.image == "<RTTR_GAME>/GFX/PICS/WORLD.LBM");
    BOOST_TEST(desc.maxHumanPlayers == 1u);
    BOOST_TEST(desc.difficulty == "easy");

    // maps
    BOOST_TEST(desc.getNumMaps() == 3u);
    BOOST_TEST(desc.getMapFilePath(0) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/DATA/MAPS/dessert0.WLD"));
    BOOST_TEST(desc.getMapFilePath(1) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/DATA/MAPS/dessert1.WLD"));
    BOOST_TEST(desc.getMapFilePath(2) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/DATA/MAPS/dessert2.WLD"));
    BOOST_TEST(desc.getLuaFilePath(0) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/CAMPAIGNS/ROMAN/dessert0.lua"));
    BOOST_TEST(desc.getLuaFilePath(1) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/CAMPAIGNS/ROMAN/dessert1.lua"));
    BOOST_TEST(desc.getLuaFilePath(2) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/CAMPAIGNS/ROMAN/dessert2.lua"));
}

BOOST_AUTO_TEST_CASE(LoadCampaignDescriptionFailsDueToMissingCampaignVariable)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp.get() / "campaign.lua");

        file << "roemer_campaign ={\
            version = \"1\",\
        }";

        file << "function getRequiredLuaVersion() return 2 end";
    }

    CampaignDescription desc;
    CampaignDataLoader loader(desc, tmp.get());
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    REQUIRE_LOG_CONTAINS("Failed to load campaign data!\nReason: Campaign table variable missing.");
}

BOOST_AUTO_TEST_CASE(LoadCampaignDescriptionFailsDueToIncorrectDifficulty)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp.get() / "campaign.lua");

        file << "campaign ={\
            version = \"1\",\
            author = \"Max Meier\",\
            name = \"Meine Kampagne\",\
            shortDescription = \"Sehr kurze Beschreibung\",\
            longDescription = \"Das ist die lange Beschreibung\",\
            image = \"<RTTR_GAME>/GFX/PICS/WORLD.LBM\",\
            maxHumanPlayers = 1,\
            difficulty = \"middle\",\
            mapFolder = \"<RTTR_GAME>/DATA/MAPS\",\
            luaFolder = \"<RTTR_GAME>/CAMPAIGNS/ROMAN\",\
            maps = { \"dessert0.WLD\", \"dessert1.WLD\", \"dessert2.WLD\"}\
        }";

        file << "function getRequiredLuaVersion() return 1 end";
    }

    CampaignDescription desc;
    CampaignDataLoader loader(desc, tmp.get());
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    REQUIRE_LOG_CONTAINS("Failed to load campaign data!\nReason: Invalid difficulty: middle");
}

BOOST_AUTO_TEST_CASE(LoadCampaignDescriptionFailsDueToMissingField)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp.get() / "campaign.lua");

        file << "campaign ={\
            version = \"1\",\
            author = \"Max Meier\",\
            name = \"Meine Kampagne\",\
            shortDescription = \"Sehr kurze Beschreibung\",\
            longDescription = \"Das ist die lange Beschreibung\",\
            image = \"<RTTR_GAME>/GFX/PICS/WORLD.LBM\",\
            maxHumanPlayers = 1,\
            difficulty = \"easy\",\
            mapFolder = \"<RTTR_GAME>/DATA/MAPS\",\
            maps = { \"dessert0.WLD\", \"dessert1.WLD\", \"dessert2.WLD\"}\
        }";

        file << "function getRequiredLuaVersion() return 1 end";
    }

    CampaignDescription desc;
    CampaignDataLoader loader(desc, tmp.get());
    rttr::test::LogAccessor logAcc;
    BOOST_TEST(!loader.Load());
    REQUIRE_LOG_CONTAINS(
      "Failed to load campaign data!\nReason: Failed to load game data: Required field 'luaFolder' not found");
}

BOOST_AUTO_TEST_CASE(CampaignDescriptionLoadWithTranslation)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp.get() / "campaign.lua");

        file << "rttr:RegisterTranslations(\
        {\
            en =\
            {\
                name = 'My campaign',\
                shortDescription = 'Very short description',\
                longDescription = 'That is the long description'\
            },\
            de =\
            {\
                name = 'Meine Kampagne',\
                shortDescription = 'Sehr kurze Beschreibung',\
                longDescription = 'Das ist die lange Beschreibung'\
            }\
        })";

        file << "campaign = {\
            version = \"1\",\
            author = \"Max Meier\",\
            name = _\"name\",\
            shortDescription = _\"shortDescription\",\
            longDescription = _\"longDescription\",\
            image = \"<RTTR_GAME>/GFX/PICS/WORLD.LBM\",\
            maxHumanPlayers = 1,\
            difficulty = \"easy\",\
            mapFolder = \"<RTTR_GAME>/DATA/MAPS\",\
            luaFolder = \"<RTTR_GAME>/CAMPAIGNS/ROMAN\",\
            maps = { \"dessert0.WLD\", \"dessert1.WLD\", \"dessert2.WLD\"}\
        }";

        file << "function getRequiredLuaVersion() return 1 end";
    }

    rttr::test::LocaleResetter loc("de");

    CampaignDescription desc;
    CampaignDataLoader loader(desc, tmp.get());
    BOOST_TEST_REQUIRE(loader.Load());

    // campaign description
    BOOST_TEST(desc.version == "1");
    BOOST_TEST(desc.author == "Max Meier");
    BOOST_TEST(desc.name == "Meine Kampagne");
    BOOST_TEST(desc.shortDescription == "Sehr kurze Beschreibung");
    BOOST_TEST(desc.longDescription == "Das ist die lange Beschreibung");
    BOOST_TEST(desc.image == "<RTTR_GAME>/GFX/PICS/WORLD.LBM");
    BOOST_TEST(desc.maxHumanPlayers == 1u);
    BOOST_TEST(desc.difficulty == "easy");

    // maps
    BOOST_TEST(desc.getNumMaps() == 3u);
    BOOST_TEST(desc.getMapFilePath(0) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/DATA/MAPS/dessert0.WLD"));
    BOOST_TEST(desc.getMapFilePath(1) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/DATA/MAPS/dessert1.WLD"));
    BOOST_TEST(desc.getMapFilePath(2) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/DATA/MAPS/dessert2.WLD"));
    BOOST_TEST(desc.getLuaFilePath(0) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/CAMPAIGNS/ROMAN/dessert0.lua"));
    BOOST_TEST(desc.getLuaFilePath(1) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/CAMPAIGNS/ROMAN/dessert1.lua"));
    BOOST_TEST(desc.getLuaFilePath(2) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/CAMPAIGNS/ROMAN/dessert2.lua"));

    // selection map
    BOOST_TEST(!desc.getSelectionMapData().has_value());
}

BOOST_AUTO_TEST_CASE(OptionalSelectionMapLoadTest)
{
    rttr::test::TmpFolder tmp;
    {
        bnw::ofstream file(tmp.get() / "campaign.lua");

        file << "campaign = {\
            version = \"1\",\
            author = \"Max Meier\",\
            name = \"Meine Kampagne\",\
            shortDescription = \"Sehr kurze Beschreibung\",\
            longDescription = \"Das ist die lange Beschreibung\",\
            image = \"<RTTR_GAME>/GFX/PICS/WORLD.LBM\",\
            maxHumanPlayers = 1,\
            difficulty = \"easy\",\
            mapFolder = \"<RTTR_GAME>/DATA/MAPS\",\
            luaFolder = \"<RTTR_GAME>/CAMPAIGNS/ROMAN\",\
            maps = { \"dessert0.WLD\", \"dessert1.WLD\", \"dessert2.WLD\"},\
            selectionMap = {\
                background     = {\"<RTTR_GAME>/GFX/PICS/SETUP990.LBM\", 0},\
                map            = {\"<RTTR_GAME>/GFX/PICS/WORLD.LBM\", 0},\
                missionMapMask = {\"<RTTR_GAME>/GFX/PICS/WORLDMSK.LBM\", 0},\
                marker         = {\"<RTTR_GAME>/DATA/IO/IO.DAT\", 231},\
                conquered      = {\"<RTTR_GAME>/DATA/IO/IO.DAT\", 232},\
                backgroundOffset = {64, 70},\
                disabledColor = 0x70000000,\
                missionSelectionInfos = {\
                    {0xffffff00, 243, 97},\
                    {0xffaf73cb, 55, 78},\
                    {0xff008fc3, 122, 193}\
                }\
            }\
        }";

        file << "function getRequiredLuaVersion() return 2 end";
    }

    CampaignDescription desc;
    CampaignDataLoader loader(desc, tmp.get());
    BOOST_TEST_REQUIRE(loader.Load());

    // campaign description
    BOOST_TEST(desc.version == "1");
    BOOST_TEST(desc.author == "Max Meier");
    BOOST_TEST(desc.name == "Meine Kampagne");
    BOOST_TEST(desc.shortDescription == "Sehr kurze Beschreibung");
    BOOST_TEST(desc.longDescription == "Das ist die lange Beschreibung");
    BOOST_TEST(desc.image == "<RTTR_GAME>/GFX/PICS/WORLD.LBM");
    BOOST_TEST(desc.maxHumanPlayers == 1u);
    BOOST_TEST(desc.difficulty == "easy");

    // maps
    BOOST_TEST(desc.getNumMaps() == 3u);
    BOOST_TEST(desc.getMapFilePath(0) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/DATA/MAPS/dessert0.WLD"));
    BOOST_TEST(desc.getMapFilePath(1) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/DATA/MAPS/dessert1.WLD"));
    BOOST_TEST(desc.getMapFilePath(2) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/DATA/MAPS/dessert2.WLD"));
    BOOST_TEST(desc.getLuaFilePath(0) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/CAMPAIGNS/ROMAN/dessert0.lua"));
    BOOST_TEST(desc.getLuaFilePath(1) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/CAMPAIGNS/ROMAN/dessert1.lua"));
    BOOST_TEST(desc.getLuaFilePath(2) == RTTRCONFIG.ExpandPath("<RTTR_GAME>/CAMPAIGNS/ROMAN/dessert2.lua"));

    // selection map
    auto selectionMap = desc.getSelectionMapData();
    BOOST_TEST(selectionMap->background.filePath == "<RTTR_GAME>/GFX/PICS/SETUP990.LBM");
    BOOST_TEST(selectionMap->background.index == 0u);
    BOOST_TEST(selectionMap->map.filePath == "<RTTR_GAME>/GFX/PICS/WORLD.LBM");
    BOOST_TEST(selectionMap->map.index == 0u);
    BOOST_TEST(selectionMap->missionMapMask.filePath == "<RTTR_GAME>/GFX/PICS/WORLDMSK.LBM");
    BOOST_TEST(selectionMap->missionMapMask.index == 0u);
    BOOST_TEST(selectionMap->marker.filePath == "<RTTR_GAME>/DATA/IO/IO.DAT");
    BOOST_TEST(selectionMap->marker.index == 231u);
    BOOST_TEST(selectionMap->conquered.filePath == "<RTTR_GAME>/DATA/IO/IO.DAT");
    BOOST_TEST(selectionMap->conquered.index == 232u);
    BOOST_TEST(selectionMap->mapOffsetInBackground == Position(64, 70));
    BOOST_TEST(selectionMap->disabledColor == 0x70000000u);
    BOOST_TEST(selectionMap->missionSelectionInfos.size() == 3u);
    BOOST_TEST(selectionMap->missionSelectionInfos[0].maskAreaColor == 0xffffff00u);
    BOOST_TEST(selectionMap->missionSelectionInfos[0].ankerPos == Position(243, 97));
    BOOST_TEST(selectionMap->missionSelectionInfos[1].maskAreaColor == 0xffaf73cbu);
    BOOST_TEST(selectionMap->missionSelectionInfos[1].ankerPos == Position(55, 78));
    BOOST_TEST(selectionMap->missionSelectionInfos[2].maskAreaColor == 0xff008fc3u);
    BOOST_TEST(selectionMap->missionSelectionInfos[2].ankerPos == Position(122, 193));
}

BOOST_AUTO_TEST_SUITE_END()
