#pragma once

#include "../../Common/Interfaces/ModuleInterface.h"
#include "../../Common/Helpers/ModuleHook.hpp"

#include <chrono>
#include <ctime>
#include <random>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <spdlog.h>

#include <rapidjson/prettywriter.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

namespace GameServer
{
    namespace Addon
    {
        class CLootExchangeExtended
            : public Yorozuya::Module::IModule
            , CModuleHook
        {
        public:
			class item_fld {
			public:
				item_fld(std::string id,
					uint32_t type, int value) : id(id)
					, type(type)
					, value(value) {};
				std::string id;
				uint32_t type;
				int value;
			};

			static std::vector<item_fld*> m_records;
			static std::shared_ptr<spdlog::logger> logger;
			static bool m_bActivated;

			CLootExchangeExtended() { };

            virtual void load() override;

            virtual void unload() override;

            virtual Yorozuya::Module::ModuleName_t get_name() override;

            virtual void configure(const rapidjson::Value& nodeConfig) override;

			static void WINAPIV pc_TakeGroundingItem(
				ATF::CPlayer* pObj,
				ATF::CItemBox* pBox,
				uint16_t wAddSerial,
				ATF::Info::CPlayerpc_TakeGroundingItem1947_ptr next
			);

			static bool AddMoney(ATF::CPlayer * pObj,
				uint32_t nMoneyType,
				int nMoneyValue);
        };
    };
};
