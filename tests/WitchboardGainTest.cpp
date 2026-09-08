#define main cleanTestMain
#include "WitchboardCleanTest.cpp"
#undef main

int main()
{
	// Check gain placement with real stereo audio, active filter/sidechain,
	// existing content on output buses, and all three master routing modes.
	for (int mode = kMasterSplit; mode <= kMasterInsert; ++mode)
	for (int dsp = 0; dsp < 2; ++dsp)
	for (int gainDb = -12; gainDb <= 6; gainDb += 6)
	{
		const int32_t specs[] = {2};
		_NT_algorithmRequirements req = {};
		calculateRequirements(req, specs);
		_NT_algorithmMemoryPtrs mem[2] = {allocateMemory(req), allocateMemory(req)};
		_NT_algorithm* alg[2];
		std::vector<int16_t> values[2];
		for (int n = 0; n < 2; ++n)
		{
			alg[n] = constructWitchboard(mem[n], req, specs);
			values[n].resize(req.numParameters);
			for (unsigned i = 0; i < req.numParameters; ++i) values[n][i] = alg[n]->parameters[i].def;
			auto& v = values[n];
			v[channelBase(0)+kChannelInputL] = 1; v[channelBase(0)+kChannelInputR] = 2;
			v[channelBase(1)+kChannelInputL] = 3; v[channelBase(1)+kChannelInputR] = 4;
			v[channelBase(1)+kChannelOutputPath] = kOutputPathBypass;
			v[kParamMainL] = 13; v[kParamMainR] = 14;
			v[kParamBypassL] = 15; v[kParamBypassR] = 16;
			v[kParamMasterSendL] = 17; v[kParamMasterSendR] = 18;
			v[kParamMasterReturnL] = 19; v[kParamMasterReturnR] = 20;
			v[kParamMasterMode] = mode; v[kParamMasterGain] = n ? gainDb : 0;
			v[kParamMasterFilterEnable] = dsp; v[kParamMasterFilterSweep] = -50;
			v[kParamSidechainMode] = dsp;
			v[kParamSidechainKeyInput] = 21;
			alg[n]->v = v.data();
		}
		for (int block = 0; block < 150; ++block)
		{
			std::vector<float> buses[2];
			for (int n = 0; n < 2; ++n)
			{
				buses[n].resize(kNT_lastBus*4, 0);
				for (int bus = 1; bus <= kNT_lastBus; ++bus)
					for (int i = 0; i < 4; ++i)
						busSample(buses[n],bus,i) = bus >= 13 && bus <= 18 ? 0.125f
							: 0.05f*sin(0.03f*bus*(block*4+i));
				step(alg[n], buses[n].data(), 1);
			}
			const float factor = powf(10.0f, gainDb/20.0f);
			for (int bus = 13; bus <= 18; ++bus)
			for (int i = 0; i < 4; ++i)
			{
				// Final gain never rescales existing bus audio or the master insert send.
				const float scale = bus <= 16 ? factor : 1.0f;
				assert(fabsf(busSample(buses[1],bus,i) -
					(0.125f+(busSample(buses[0],bus,i)-0.125f)*scale)) < 2e-6f);
			}
		}
		for (auto& m : mem) freeMemory(m);
	}

	// Positive channel gain is clean, before inserts and FX, with unity defaults.
	const int32_t specs[] = {1};
	_NT_algorithmRequirements req = {};
	calculateRequirements(req, specs);
	auto mem = allocateMemory(req);
	auto* alg = constructWitchboard(mem, req, specs);
	std::vector<int16_t> v(req.numParameters);
	for (unsigned i = 0; i < req.numParameters; ++i) v[i] = alg->parameters[i].def;
	assert(alg->parameters[channelBase(0)+kChannelGain].max == 6);
	assert(alg->parameters[channelBase(0)+kChannelGain].def == 0);
	assert(alg->parameters[kParamMasterGain].min == -12);
	assert(alg->parameters[kParamMasterGain].max == 6);
	assert(alg->parameters[kParamMasterGain].def == 0);
	v[channelBase(0)+kChannelInputL] = 1;
	v[channelBase(0)+kChannelGain] = 6;
	v[kParamMainL] = 13;
	alg->v = v.data();
	std::vector<float> buses(kNT_lastBus*4,0);
	fillBus(buses,1,0.25f); step(alg,buses.data(),1);
	assertBus(buses,13,0.25f*powf(10,6.0f/20));

	// A live master change ramps, then returns exactly to unity/fast-path operation.
	for (int target : {6, -12, 0})
	{
		v[kParamMasterGain] = target;
		const int samples = static_cast<int>(NT_globals.sampleRate*0.01f);
		for (int block = 0; block < samples/4+2; ++block)
		{
			buses.assign(kNT_lastBus*4,0); fillBus(buses,1,0.25f);
			step(alg,buses.data(),1);
			for (float x : buses) assert(isfinite(x));
			if (block == 0) assert(static_cast<WitchboardAlgorithm*>(alg)->masterGain.samplesRemaining > 0);
		}
		assertBus(buses,13,0.25f*powf(10,6.0f/20)*powf(10,target/20.0f));
		assert(static_cast<WitchboardAlgorithm*>(alg)->masterGain.samplesRemaining == 0);
	}
	freeMemory(mem);
	printf("PASS: gain ranges, stereo post-DSP placement, split/sum/insert routing, additive buses, live smoothing at %.0f Hz\n",static_cast<double>(NT_globals.sampleRate));
}
