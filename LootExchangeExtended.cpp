#include "stdafx.h"

#include "LootExchangeExtended.h"
#include "../../Common/ETypes.h"
#include "../../Common/Helpers/RapidHelper.hpp"

#include <bitset>
#include <ATF/global.hpp>

namespace fs = ::std::experimental::filesystem::v1;

namespace GameServer
{
    namespace Addon
    {
		bool CLootExchangeExtended::m_bActivated = false;
		std::shared_ptr<spdlog::logger> CLootExchangeExtended::logger;
		std::vector<CLootExchangeExtended::item_fld*> CLootExchangeExtended::m_records;

        void CLootExchangeExtended::load()
        {
			enable_hook(&ATF::CPlayer::pc_TakeGroundingItem, &CLootExchangeExtended::pc_TakeGroundingItem);
        }

		void WINAPIV CLootExchangeExtended::pc_TakeGroundingItem(
			ATF::CPlayer* pObj,
			ATF::CItemBox* pBox,
			uint16_t wAddSerial,
			ATF::Info::CPlayerpc_TakeGroundingItem1947_ptr next)
		{
			bool bThrowToNext = true;

			do
			{
				if (!CLootExchangeExtended::m_bActivated)
				{
					break;
				}

				if (!pBox->m_bLive)
				{
					break;
				}

				if (!pBox->IsTakeRight(pObj))
				{
					break;
				}

				if (ATF::Global::Get3DSqrt(pBox->m_fCurPos, pObj->m_fCurPos) > 100.f)
				{
					break;
				}

				if (pBox->m_Item.m_byTableCode >= _countof(ATF::Global::g_MainThread->m_tblItemData))
				{
					break;
				}

				auto& ItemRecords = ATF::Global::g_MainThread->m_tblItemData[pBox->m_Item.m_byTableCode];
				auto pRecordFld = ItemRecords.GetRecord(pBox->m_Item.m_wItemIndex);

				if (pRecordFld == nullptr)
				{
					break;
				}

				const auto& it = std::find_if(CLootExchangeExtended::m_records.begin(), CLootExchangeExtended::m_records.end(),
					[&](const item_fld* fld)
				{
					return pRecordFld->m_strCode == fld->id;
				});

				if (it == CLootExchangeExtended::m_records.end())
				{
					break;
				}

				__int64 index = std::distance(CLootExchangeExtended::m_records.begin(), it);
				auto& el = CLootExchangeExtended::m_records.at(index);

				int newValue = el->value;
				newValue *= pBox->m_Item.m_dwDur ? pBox->m_Item.m_dwDur : 1;

				bool bOk = CLootExchangeExtended::AddMoney(pObj,
					el->type,
					newValue);

				if (!bOk)
				{
					break;
				}

				ATF::_STORAGE_LIST::_db_con BoxItem;
				memcpy(&BoxItem, &pBox->m_Item, sizeof(BoxItem));
				pBox->Destroy();
				pObj->SendMsg_TakeAddResult(0, &BoxItem);

				bThrowToNext = false;
			} while (false);

			if (bThrowToNext)
			{
				next(pObj, pBox, wAddSerial);
			}
		}

		bool CLootExchangeExtended::AddMoney(
			ATF::CPlayer * pObj,
			uint32_t nMoneyType,
			int nMoneyValue)
		{
			if (nMoneyType < 0)
			{
				return false;
			}

			bool result = false;

			auto fnAddActPoint = [&](char byActionCode)->bool
			{
				bool result = ATF::CActionPointSystemMgr::Instance()->GetEventStatus(byActionCode) == 2;

				if (!result)
					return result;

				auto dwPoint = pObj->m_pUserDB->GetActPoint(byActionCode);
				dwPoint += nMoneyValue;
				pObj->m_pUserDB->Update_User_Action_Point(byActionCode, dwPoint);
				pObj->SendMsg_Alter_Action_Point(byActionCode, dwPoint);

				return result;
			};

			switch ((e_money_type)nMoneyType)
			{
			case e_money_type::cp:
				pObj->AlterDalant(nMoneyValue);
				pObj->SendMsg_AlterMoneyInform(0);
				result = true;
				break;
			case e_money_type::gold:
				pObj->AlterGold(nMoneyValue);
				pObj->SendMsg_AlterMoneyInform(0);
				result = true;
				break;
			case e_money_type::pvp_point:
				pObj->AlterPvPPoint(nMoneyValue, ATF::PVP_ALTER_TYPE::cheat, -1);
				result = true;
				break;
			case e_money_type::pvp_point_2:
				pObj->AlterPvPCashBag(nMoneyValue, ATF::PVP_MONEY_ALTER_TYPE::pm_kill);
				result = true;
				break;
			case e_money_type::processing_point:
				result = fnAddActPoint(0);
				break;
			case e_money_type::hunter_point:
				result = fnAddActPoint(1);
				break;
			case e_money_type::gold_point:
				result = fnAddActPoint(2);
				break;
			default:
				result = false;
			}

			return result;
		};

        void CLootExchangeExtended::unload()
        {
            cleanup_all_hook();
        }

        Yorozuya::Module::ModuleName_t CLootExchangeExtended::get_name()
        {
            static const Yorozuya::Module::ModuleName_t name = "addon.loot_exchange_extended";
            return name;
        }

        void CLootExchangeExtended::configure(const rapidjson::Value & nodeConfig)
        {
			m_bActivated = RapidHelper::GetValueOrDefault(nodeConfig, "activated", false);
			bool m_bFlushLogs = RapidHelper::GetValueOrDefault(nodeConfig, "flush_logs", true);

			std::string sPathToConfig = RapidHelper::GetValueOrDefault<std::string>(nodeConfig, "config_path", "./YorozuyaGS/loot_exchange_extended.json");
			CLootExchangeExtended::logger = spdlog::basic_logger_mt(get_name(), "YorozuyaGS/Logs/LootExchangeExtended.txt");

			if (m_bFlushLogs)
			{
				CLootExchangeExtended::logger->flush();
			}

			CLootExchangeExtended::logger->info("configure...");

			if (!m_bActivated)
			{
				CLootExchangeExtended::logger->info("addon is disabled");
				return;
			}

			// try to loading config file
			CLootExchangeExtended::logger->info("try to read configuration file {}", sPathToConfig);
			if (!fs::exists(sPathToConfig))
			{
				CLootExchangeExtended::logger->info("configuration file not found, addon force disabled");
				m_bActivated = false;
				return;
			}

			std::ifstream ifs(sPathToConfig);

			rapidjson::IStreamWrapper isw(ifs);
			rapidjson::Document LootExchangeExtendedConfig;

			if (LootExchangeExtendedConfig.ParseStream(isw).HasParseError())
			{
				CLootExchangeExtended::logger->info("config file - corrupted, addon force disabled");
				m_bActivated = false;
				return;
			}

			for (auto& rec : LootExchangeExtendedConfig.GetArray())
			{
				if (!rec.HasMember("activated")
					|| !rec["activated"].IsBool()
					|| !rec["activated"].GetBool())
				{
					continue;
				}

				if (!rec.HasMember("id") || !rec["id"].IsString())
				{
					continue;
				}

				item_fld *fld = new item_fld(rec["id"].GetString(),
					((rec.HasMember("type") && rec["type"].IsInt())
						? rec["type"].GetInt() : -1),
					((rec.HasMember("value") && rec["value"].IsInt())
						? rec["value"].GetInt() : -1));

				CLootExchangeExtended::m_records.push_back(fld);
				CLootExchangeExtended::logger->info("created fld id: {} type: {} value: {}", fld->id, fld->type, fld->value);
			}
        }
    }
}
