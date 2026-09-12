#define main cleanTestMain
#include "WitchboardCleanTest.cpp"
#undef main

struct TenChannels
{
	_NT_algorithmRequirements requirements = {};
	_NT_algorithmMemoryPtrs memory;
	WitchboardAlgorithm* algorithm;
	std::vector<int16_t> values;
	TenChannels()
	{
		const int32_t specs[] = {10};
		calculateRequirements(requirements, specs);
		memory = allocateMemory(requirements);
		algorithm = static_cast<WitchboardAlgorithm*>(constructWitchboard(memory, requirements, specs));
		values.resize(requirements.numParameters);
		for (unsigned p = 0; p < requirements.numParameters; ++p)
			values[p] = algorithm->parameters[p].def;
		algorithm->v = values.data();
		algorithm->vIncludingCommon = values.data();
	}
	~TenChannels() { freeMemory(memory); }
};

void testPagesAndDefaults()
{
	TenChannels f;
	assert(specifications[0].max == 10 && specifications[0].def == 10);
	assert(f.requirements.numParameters == 237);
	assert(f.algorithm->parameterPages->numPages == 15);
	assert(kParamBypassOffset == 86);
	assert(kNumGlobalParams == 87 && kNumChannelParams == 15);
	bool seen[237] = {};
	for (int channel = 0; channel < 10; ++channel)
	{
		const auto& page = f.algorithm->parameterPages->pages[5+channel];
		char name[20], prefix[kNT_parameterUiPrefixSize];
		snprintf(name, sizeof(name), "Channel %d", channel+1);
		assert(strcmp(page.name, name) == 0);
		assert(page.numParams == 15);
		for (int field = 0; field < 15; ++field)
		{
			int p = page.params[field];
			assert(p == 87 + channel*15 + field && p < 237);
			assert(!seen[p]); seen[p] = true;
			parameterUiPrefix(f.algorithm, p, prefix);
			snprintf(name, sizeof(name), "%d:", channel+1);
			assert(strcmp(prefix, name) == 0);
		}
	}
	const auto& outputs = f.algorithm->parameterPages->pages[2];
	const int expectedOutputs[] = {49,50,51,52,86};
	assert(strcmp(outputs.name, "Final Outputs") == 0 && outputs.numParams == 5);
	for (int i = 0; i < 5; ++i) assert(outputs.params[i] == expectedOutputs[i]);
	const auto& master = f.algorithm->parameterPages->pages[4];
	assert(master.numParams == 18);
	for (int i = 0; i < 7; ++i) assert(master.params[i] == 68+i);
	for (int i = 0; i < master.numParams; ++i) assert(master.params[i] != 86);
	assert(f.values[kParamSidechainMode] == 0 && f.values[kParamSidechainKeyInput] == 0);
	assert(f.values[kParamSidechainDepth] == 69);
	assert(f.values[kParamSidechainLookahead] == 60);
	assert(f.values[kParamSidechainEnvLength] == 486);
	assert(f.values[kParamSidechainCurve] == 20 && f.values[kParamSidechainSmooth] == 4);
	assert(f.values[kParamMasterFilterQ] == 10);
	assert(f.values[kParamMasterFilterLpCutoff] == 20);
	assert(f.values[kParamMasterFilterHpCutoff] == 70);
	assert(f.values[kParamMasterFilterEnable] == 0 && f.values[kParamMasterFilterSweep] == 0);
	assert(f.values[kParamMasterGain] == 0);
	// The final channel only grows existing page/runtime storage, not delay rings.
	const int32_t nineSpecs[] = {9};
	_NT_algorithmRequirements nine = {};
	calculateRequirements(nine, nineSpecs);
	assert(f.requirements.sram == nine.sram);
	assert(f.requirements.dram > nine.dram && f.requirements.dram - nine.dram < 256);
	assert(f.requirements.dtc == 0 && f.requirements.itc == 0);
	const uintptr_t begin = reinterpret_cast<uintptr_t>(f.memory.dram);
	const uintptr_t end = begin + f.requirements.dram;
	assert(reinterpret_cast<uintptr_t>(f.algorithm->mainDelay.data) >= begin);
	assert(reinterpret_cast<uintptr_t>(f.algorithm->bypassDelay.data + 2*kBypassDelayCapacity) <= end);
}

void testChannelTenRouting()
{
	// Main, Bypass, Insert 1, Insert 2, FX Send 1, FX Send 2.
	for (int path = 0; path < 6; ++path)
	{
		TenChannels f;
		auto& v = f.values;
		const int first = channelBase(0), last = channelBase(9);
		v[kParamFadeMs] = 0;
		v[first+kChannelInputL] = 3; v[first+kChannelInputR] = 4;
		v[last+kChannelInputL] = 1; v[last+kChannelInputR] = 2;
		v[kParamMainL] = 13; v[kParamMainR] = 14;
		v[kParamBypassL] = 15; v[kParamBypassR] = 16;
		v[kParamFx1L] = 17; v[kParamFx1R] = 18; v[kParamFx1Width] = kWidthStereo;
		v[kParamFx2L] = 19; v[kParamFx2R] = 20; v[kParamFx2Width] = kWidthStereo;
		v[routeParam(0,kRouteOutputL)] = 21; v[routeParam(0,kRouteOutputR)] = 22;
		v[routeParam(0,kRouteSendWidth)] = kWidthStereo;
		v[routeParam(0,kRouteReturnL)] = 7; v[routeParam(0,kRouteReturnR)] = 8;
		v[routeParam(0,kRouteReturnWidth)] = kWidthStereo;
		if (path == 1) v[last+kChannelOutputPath] = kOutputPathBypass;
		if (path == 2) {v[last+kChannelInsert1] = 1; v[last+kChannelInsert1Slot1] = 0;}
		if (path == 3) {v[last+kChannelInsert2] = 1; v[last+kChannelInsert2Slot1] = 0;}
		if (path == 4) v[last+kChannelFx1Mix] = 100;
		if (path == 5) v[last+kChannelFx2Mix] = 100;
		// Host-style notifications affect channel 10 only.
		for (int p = last; p < last+15; ++p) parameterChanged(f.algorithm, p);
		std::vector<float> buses(kNT_lastBus*4, 0);
		fillBus(buses,1,0.25f); fillBus(buses,2,-0.5f);
		fillBus(buses,3,2); fillBus(buses,4,-3);
		fillBus(buses,7,0.75f); fillBus(buses,8,-1);
		step(f.algorithm,buses.data(),1);
		const bool insert = path == 2 || path == 3;
		assertBus(buses,13,2+(path == 0 ? 0.25f : insert ? 0.75f : 0));
		assertBus(buses,14,-3+(path == 0 ? -0.5f : insert ? -1 : 0));
		for (int out : {15,17,19,21})
		{
			const bool active = (out == 15 && path == 1) || (out == 17 && path == 4)
				|| (out == 19 && path == 5) || (out == 21 && insert);
			assertBus(buses,out,active ? 0.25f : 0);
			assertBus(buses,out+1,active ? -0.5f : 0);
		}
		assert(f.algorithm->runtime[0].insertState[0] == 0);
		assert(f.algorithm->runtime[0].insertState[1] == 0);
		// Parameter values are serialized by NT, custom metadata by this plugin.
		JsonTape tape;
		{_NT_jsonStream stream(&tape); stream.openObject(); serialise(f.algorithm,stream); stream.closeObject();}
		TenChannels loaded;
		loaded.values = v;
		for (unsigned p = 0; p < v.size(); ++p) parameterChanged(loaded.algorithm,p);
		{_NT_jsonParse parse(&tape,0); assert(deserialise(loaded.algorithm,parse));}
		std::vector<float> copy(kNT_lastBus*4, 0);
		fillBus(copy,1,0.25f); fillBus(copy,2,-0.5f); fillBus(copy,3,2); fillBus(copy,4,-3);
		fillBus(copy,7,0.75f); fillBus(copy,8,-1);
		step(loaded.algorithm,copy.data(),1);
		assert(copy == buses && loaded.values == v);
	}
}

int main()
{
	testPagesAndDefaults(); testChannelTenRouting();
	printf("PASS: 10 channels, 237 identities, output-page placement, locked defaults, channel-10 stereo routes and preset restoration at %.0f Hz\n",double(NT_globals.sampleRate));
}
