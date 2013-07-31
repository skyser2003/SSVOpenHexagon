// Copyright (c) 2013 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: http://opensource.org/licenses/AFL-3.0

#ifndef HG_ONLINE_UTILS
#define HG_ONLINE_UTILS

#include <string>
#include <future>
#include <SSVUtilsJson/SSVUtilsJson.h>
#include <SSVStart/SSVStart.h>
#include <SFML/Network.hpp>
#include "SSVOpenHexagon/Online/Compression.h"

namespace hg
{
	namespace Online
	{
		enum class LogMode { Quiet, Verbose };

		namespace Internal
		{
			template<typename TArg> inline void pBuildHelper(sf::Packet& mP, TArg&& mArg) { mP << mArg; }
			template<typename TArg, typename... TArgs> inline void pBuildHelper(sf::Packet& mP, TArg&& mArg, TArgs&&... mArgs) { mP << mArg; pBuildHelper(mP, mArgs...); }

			template<unsigned int TIndex, typename TArg> inline void jBuildHelper(ssvuj::Value& mP, TArg&& mArg) { ssvuj::set(mP, TIndex, mArg); }
			template<unsigned int TIndex, typename TArg, typename... TArgs> inline void jBuildHelper(ssvuj::Value& mP, TArg&& mArg, TArgs&&... mArgs) { ssvuj::set(mP, TIndex, mArg); jBuildHelper<TIndex + 1>(mP, mArgs...); }

			inline ssvuj::Value getDecompressedJsonString(const std::string& mData) { ssvuj::Value result{ssvuj::getRootFromString(getZLIBDecompress(mData))}; return result; }
			template<typename... TArgs> inline ssvuj::Value buildJsonDataArray(TArgs&&... mArgs) { ssvuj::Value result; Internal::jBuildHelper<0>(result, mArgs...); return result; }
			template<typename... TArgs> inline std::string buildCompressedJsonString(TArgs&&... mArgs)
			{
				ssvuj::Value request{buildJsonDataArray(mArgs...)}; std::string requestString;
				ssvuj::writeRootToString(request, requestString); return getZLIBCompress(requestString);
			}
		}
		template<unsigned int TType> inline sf::Packet buildPacket() { sf::Packet result; result << TType; return result; }
		template<unsigned int TType, typename... TArgs> inline sf::Packet buildPacket(TArgs&&... mArgs) { sf::Packet result; result << TType; Internal::pBuildHelper(result, mArgs...); return result; }

		template<unsigned int TType, typename... TArgs> inline sf::Packet buildCompressedPacket(TArgs&&... mArgs) { sf::Packet result; result << TType << Internal::buildCompressedJsonString(mArgs...); return result; }
		inline ssvuj::Value getDecompressedPacket(sf::Packet& mPacket) { std::string data; mPacket >> data; return Internal::getDecompressedJsonString(data); }

		template<int TTimes = 5, LogMode TLM = LogMode::Quiet> inline std::future<bool> retry(std::function<bool()> mFunc, const std::chrono::duration<int, std::milli>& mDuration = std::chrono::milliseconds(1500))
		{
			auto result(std::async(std::launch::async, [=]
			{
				for(int i{0}; i < TTimes; ++i)
				{
					if(mFunc()) return true;

					if(TLM == LogMode::Verbose) ssvu::lo << ssvu::lt("asyncTry") << "Error - retrying (" << i + 1 << "/" << TTimes << ")" << std::endl;
					std::this_thread::sleep_for(mDuration);
				}

				return false;
			}));

			return result;
		}
	}
}

#endif
