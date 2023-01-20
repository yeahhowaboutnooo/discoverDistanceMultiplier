#include "Hooks.h"
#include <xbyak/xbyak.h>

namespace logger = SKSE::log;

namespace Hooks
{
	float discoverDistanceMultiplier = 2.0f;
	void fail() {
		logger::error("failed to modify discover Distance!");
		return;
	}
	
	uint64_t injectionPoint, origCodeSize;
	void modifyDiscoverDistance()
	{
		//1.5.97 6b8340 39663
		auto fPlayerCharacter__sub_1406B8340 = REL::RelocationID(39663, 40750).address();
		if (!fPlayerCharacter__sub_1406B8340) {
			fail();
			return;
		}
		uintptr_t offset = 0;
		
		switch (REL::Module::GetRuntime()) {
			case REL::Module::Runtime::SE:
				offset = 0x1a4;
				break;
			case REL::Module::Runtime::AE:
				offset = 0x1c4;
				break;
			default:
				fail();
				return;
		}
		
		injectionPoint = fPlayerCharacter__sub_1406B8340 + offset;

		struct OrigCode : Xbyak::CodeGenerator
		{
			OrigCode() { 
				mulss(xmm0, xmm0);
				comiss(xmm6, xmm0);
			}
		};
		OrigCode orig;
		orig.ready();

		//erase the original instruction with nops
		std::vector<std::uint8_t> nops(orig.getSize());
		std::fill(nops.begin(), nops.end(), (std::uint8_t)0x90);
		REL::safe_write<std::uint8_t>(injectionPoint, nops);


		origCodeSize = orig.getSize();
		struct MultiplyDiscoverDistance : Xbyak::CodeGenerator
		{
			MultiplyDiscoverDistance()
			{
				mulss(xmm0, ptr[rip + 21]);
				mulss(xmm0, xmm0);                                           // 4 bytes
				comiss(xmm6, xmm0);                                          // 3 bytes
				jmp(ptr[rip]);                                               // 5 bytes
				dq(injectionPoint + origCodeSize);                           // 8 bytes
				dd(reinterpret_cast<uint32_t&>(discoverDistanceMultiplier)); //total = 20 bytes + 1 = 21 bytes
			}
		};
		MultiplyDiscoverDistance mdd;
		mdd.ready();

		static SKSE::Trampoline injectedCode;
		auto injectedCodeSize = mdd.getSize() + 32; //write_branch5 allocates too much mem
		injectedCode.create(injectedCodeSize);

		//writes:
		//jmp from injectionPoint -> end_InjectedCode
		//jmp from end_InjectedCode -> start_InjectedCode
		injectedCode.write_branch<5>(injectionPoint, 
		                             injectedCode.allocate(mdd));

		logger::info("injected Code @ {0:x}", injectionPoint);
		logger::info("multiplied discover Distance with {0:.2f}!", discoverDistanceMultiplier);
		
	}

	void Install()
	{
		modifyDiscoverDistance();
	}
}
