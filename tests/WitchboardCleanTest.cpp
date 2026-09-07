#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include "../plugins/Witchboard/Witchboard.cpp"

void _NT_jsonStream::openArray() {}
void _NT_jsonStream::closeArray() {}
void _NT_jsonStream::openObject() {}
void _NT_jsonStream::closeObject() {}
void _NT_jsonStream::addMemberName(const char*) {}
void _NT_jsonStream::addNumber(int) {}
void _NT_jsonStream::addNumber(float) {}
void _NT_jsonStream::addString(const char*) {}
void _NT_jsonStream::addFourCC(uint32_t) {}
void _NT_jsonStream::addBoolean(bool) {}
void _NT_jsonStream::addNull() {}

bool _NT_jsonParse::numberOfArrayElements(int&) { return false; }
bool _NT_jsonParse::numberOfObjectMembers(int&) { return false; }
bool _NT_jsonParse::matchName(const char*) { return false; }
bool _NT_jsonParse::skipMember() { return false; }
bool _NT_jsonParse::number(int&) { return false; }
bool _NT_jsonParse::number(float&) { return false; }
bool _NT_jsonParse::string(const char*&) { return false; }
bool _NT_jsonParse::boolean(bool&) { return false; }
bool _NT_jsonParse::null() { return false; }

#ifndef WITCHBOARD_TEST_SAMPLE_RATE
#define WITCHBOARD_TEST_SAMPLE_RATE 48000
#endif

const _NT_globals NT_globals = {
	.sampleRate = WITCHBOARD_TEST_SAMPLE_RATE,
	.maxFramesPerStep = 64,
	.workBuffer = NULL,
	.workBufferSizeBytes = 0,
	.streamSizeBytes = 0,
	.streamBufferSizeBytes = 0,
};

_NT_algorithmMemoryPtrs allocateMemory(const _NT_algorithmRequirements& requirements)
{
	_NT_algorithmMemoryPtrs memory = {};
	memory.sram = static_cast<uint8_t*>(malloc(requirements.sram));
	assert(memory.sram);
	if (requirements.dram > 0)
	{
		memory.dram = static_cast<uint8_t*>(malloc(requirements.dram));
		assert(memory.dram);
	}
	return memory;
}

void freeMemory(_NT_algorithmMemoryPtrs& memory)
{
	free(memory.dram);
	free(memory.sram);
}

void stepOnce(_NT_algorithm* algorithm, std::vector<int16_t>& values)
{
	algorithm->v = values.data();
	algorithm->vIncludingCommon = values.data();
	std::vector<float> buses(kNT_lastBus * 4, 0.0f);
	step(algorithm, buses.data(), 1);
}

float& busSample(std::vector<float>& buses, int bus, int frame)
{
	return buses[(bus - 1) * 4 + frame];
}

void fillBus(std::vector<float>& buses, int bus, float value)
{
	for (int frame = 0; frame < 4; ++frame)
		busSample(buses, bus, frame) = value;
}

void assertBus(const std::vector<float>& buses, int bus, float expected)
{
	for (int frame = 0; frame < 4; ++frame)
	{
		if (fabsf(buses[(bus - 1) * 4 + frame] - expected) >= 0.0001f)
			fprintf(stderr, "bus %d frame %d actual %.4f expected %.4f\n",
				bus, frame, buses[(bus - 1) * 4 + frame], expected);
		assert(fabsf(buses[(bus - 1) * 4 + frame] - expected) < 0.0001f);
	}
}

void assertClose(float actual, float expected)
{
	assert(fabsf(actual - expected) < 0.0001f);
}

void assertCrossfadeRouting(float fx1Mix, float fx2Mix,
	float expectedDry, float expectedFx1, float expectedFx2)
{
	float dry[1] = {};
	float wet1[1] = {};
	float wet2[1] = {};
	OutputPair outputs[kNumOutputPairs] = {};
	outputs[0] = { dry, NULL, false };
	outputs[2] = { wet1, NULL, false };
	outputs[3] = { wet2, NULL, false };
	const float* returnLeft[kNumRoutes] = {};
	const float* returnRight[kNumRoutes] = {};
	bool returnStereo[kNumRoutes] = {};

	processPath(outputs, returnLeft, returnRight, returnStereo,
		0, 1.0f, 1.0f, false, -1, -1, true, 0, 1.0f, 1.0f,
		shapedCrossfade(fx1Mix), shapedCrossfade(fx2Mix));
	assertClose(dry[0], expectedDry);
	assertClose(wet1[0], expectedFx1);
	assertClose(wet2[0], expectedFx2);
}

int main()
{
	assert(witchboardFactory.guid == NT_MULTICHAR('W', 't', 'E', 'Q'));
	assert(strcmp(witchboardFactory.name, "Witchboard EQ") == 0);
	assert(witchboardFactory.parameterChanged == parameterChanged);
	assert(witchboardFactory.midiMessage == NULL);
	assert(witchboardFactory.midiRealtime == NULL);
	assert(witchboardFactory.midiSysEx == NULL);
	assert(witchboardFactory.parameterUiPrefix == parameterUiPrefix);
	assert(witchboardFactory.parameterString == parameterString);
	assert(witchboardFactory.serialise != NULL);
	assert(witchboardFactory.deserialise != NULL);

	CrossfadeGains gains = shapedCrossfade(0.0f);
	assert(gains.dry == 1.0f && gains.wet == 0.0f);
	gains = shapedCrossfade(0.25f);
	assertClose(gains.dry, 1.0f);
	assertClose(gains.wet, 0.5f);
	gains = shapedCrossfade(0.5f);
	assertClose(gains.dry, 1.0f);
	assertClose(gains.wet, 1.0f);
	gains = shapedCrossfade(0.75f);
	assertClose(gains.dry, 0.5f);
	assertClose(gains.wet, 1.0f);
	gains = shapedCrossfade(1.0f);
	assert(gains.dry == 0.0f && gains.wet == 1.0f);
	for (int i = 0; i <= 1000; ++i)
	{
		gains = shapedCrossfade(i / 1000.0f);
		assert(gains.dry == gains.dry && gains.wet == gains.wet);
		assert(gains.dry >= 0.0f && gains.dry <= 1.0f);
		assert(gains.wet >= 0.0f && gains.wet <= 1.0f);
	}
	assertCrossfadeRouting(0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	assertCrossfadeRouting(1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	assertCrossfadeRouting(0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	assertCrossfadeRouting(0.5f, 0.0f, 1.0f, 1.0f, 0.0f);
	assertCrossfadeRouting(0.0f, 0.5f, 1.0f, 0.0f, 1.0f);
	assertCrossfadeRouting(0.5f, 0.5f, 1.0f, 1.0f, 1.0f);

	const int32_t oneChannelSpecs[] = { 1 };
	const int32_t specs[] = { 4 };
	const int32_t eightChannelSpecs[] = { 8 };
	const int32_t maxChannelSpecs[] = { 11 };
	_NT_algorithmRequirements oneChannelRequirements = {};
	_NT_algorithmRequirements requirements = {};
	_NT_algorithmRequirements eightChannelRequirements = {};
	_NT_algorithmRequirements maxChannelRequirements = {};
	calculateRequirements(oneChannelRequirements, oneChannelSpecs);
	calculateRequirements(requirements, specs);
	calculateRequirements(eightChannelRequirements, eightChannelSpecs);
	calculateRequirements(maxChannelRequirements, maxChannelSpecs);
	assert(requirements.numParameters == 137);
	assert(maxChannelRequirements.numParameters == 242);
	assert(specifications[0].min == 1 && specifications[0].max == 11);
	for (int channels = 1; channels <= kMaxChannels; ++channels)
	{
		const int32_t channelSpecs[] = { channels };
		_NT_algorithmRequirements channelRequirements = {};
		calculateRequirements(channelRequirements, channelSpecs);
		assert(channelRequirements.numParameters == static_cast<uint32_t>(77 + channels * 15));
		_NT_algorithmMemoryPtrs pageMemory = allocateMemory(channelRequirements);
		_NT_algorithm* pageAlgorithm = constructWitchboard(pageMemory,channelRequirements,channelSpecs);
		assert(pageAlgorithm->parameterPages->numPages == static_cast<uint32_t>(5+channels));
		const _NT_parameterPage& eqPage = pageAlgorithm->parameterPages->pages[4];
		assert(strcmp(eqPage.name,"Sidechain/Master") == 0 && eqPage.numParams == 27);
		for (int i = 0; i < 9; ++i) assert(eqPage.params[13+i] == kParamEq1Freq+i);
		freeMemory(pageMemory);
	}
	const int32_t unsupportedSpecs[] = { 12 };
	_NT_algorithmRequirements clampedRequirements = {};
	calculateRequirements(clampedRequirements, unsupportedSpecs);
	assert(clampedRequirements.numParameters == maxChannelRequirements.numParameters);
	assert(clampedRequirements.sram == maxChannelRequirements.sram);
	assert(clampedRequirements.dram == maxChannelRequirements.dram);
	assert(oneChannelRequirements.sram == requirements.sram);
	assert(requirements.sram == eightChannelRequirements.sram);
	assert(eightChannelRequirements.sram == maxChannelRequirements.sram);
	assert(maxChannelRequirements.sram < 4096);
	assert(oneChannelRequirements.dram > 0);
	assert(oneChannelRequirements.dram < requirements.dram);
	assert(requirements.dram < eightChannelRequirements.dram);
	assert(eightChannelRequirements.dram < maxChannelRequirements.dram);

	_NT_algorithmMemoryPtrs memory = allocateMemory(requirements);
	_NT_algorithm* algorithm = constructWitchboard(memory, requirements, specs);
	WitchboardAlgorithm* witchboard = static_cast<WitchboardAlgorithm*>(algorithm);
	_NT_algorithmMemoryPtrs maxMemory = allocateMemory(maxChannelRequirements);
	_NT_algorithm* maxAlgorithm = constructWitchboard(
		maxMemory, maxChannelRequirements, maxChannelSpecs);
	assert(algorithm->parameterPages->numPages == 9);
	assert(maxAlgorithm->parameterPages->numPages == 16);

	std::vector<int16_t> values(requirements.numParameters);
	for (uint32_t i = 0; i < requirements.numParameters; ++i)
		values[i] = algorithm->parameters[i].def;

	assert(strcmp(algorithm->parameters[channelBase(0) + kChannelInsert1].name,
		"Insert 1") == 0);
	assert(strcmp(algorithm->parameters[channelBase(0) + kChannelInsert1Slot1].name,
		"Insert 1 slot 1") == 0);
	assert(strcmp(algorithm->parameters[channelBase(0) + kChannelInsert1Slot2].name,
		"Insert 1 slot 2") == 0);
	assert(strcmp(algorithm->parameters[channelBase(0) + kChannelInsert1Slot3].name,
		"Insert 1 slot 3") == 0);
	assert(strcmp(algorithm->parameters[channelBase(0) + kChannelFx1Mix].name,
		"FX Send 1 mix") == 0);
	assert(strcmp(algorithm->parameters[channelBase(0) + kChannelInsert2].name,
		"Insert 2") == 0);
	assert(strcmp(algorithm->parameters[channelBase(0) + kChannelInsert2Slot1].name,
		"Insert 2 slot 1") == 0);
	assert(strcmp(algorithm->parameters[channelBase(0) + kChannelInsert2Slot2].name,
		"Insert 2 slot 2") == 0);
	assert(strcmp(algorithm->parameters[channelBase(0) + kChannelInsert2Slot3].name,
		"Insert 2 slot 3") == 0);
	assert(strcmp(algorithm->parameters[channelBase(1) + kChannelInsert1].name,
		"Insert 1") == 0);
	assert(strcmp(algorithm->parameters[channelBase(1) + kChannelFx1Mix].name,
		"FX Send 1 mix") == 0);
	assert(strcmp(algorithm->parameters[channelBase(1) + kChannelInsert2].name,
		"Insert 2") == 0);
	char prefix[kNT_parameterUiPrefixSize] = {};
	assert(parameterUiPrefix(algorithm, kParamFadeMs, prefix) == 0);
	assert(parameterUiPrefix(algorithm, channelBase(0), prefix) == 2);
	assert(strcmp(prefix, "1:") == 0);
	assert(parameterUiPrefix(algorithm, channelBase(3) + kChannelInsert2, prefix) == 2);
	assert(strcmp(prefix, "4:") == 0);
	assert(parameterUiPrefix(maxAlgorithm,
		channelBase(10) + kChannelInsert2, prefix) == 3);
	assert(strcmp(prefix, "11:") == 0);
	assert(algorithm->parameters[channelBase(0) + kChannelInsert1].unit
		== kNT_unitEnum);
	assert(algorithm->parameters[channelBase(0) + kChannelInsert1Slot1].unit
		== kNT_unitHasStrings);
	assert(algorithm->parameters[channelBase(0) + kChannelInsert1].max == 4);
	assert(algorithm->parameters[channelBase(0) + kChannelInsert2].max == 4);
	assert(algorithm->parameters[channelBase(0) + kChannelInsert1].enumStrings
		== insertStateStrings);
	assert(strcmp(algorithm->parameters[channelBase(0) + kChannelInsert1].enumStrings[0],
		"Dry") == 0);
	assert(strcmp(algorithm->parameters[channelBase(0) + kChannelInsert1].enumStrings[4],
		"Slot 3") == 0);
	assert(algorithm->parameters[channelBase(0) + kChannelInsert1Slot1].enumStrings == NULL);

	char label[kNT_parameterStringSize] = {};
	assert(parameterString(algorithm, channelBase(0) + kChannelInsert1, 0, label) == 3);
	assert(strcmp(label, "Dry") == 0);
	assert(parameterString(algorithm, channelBase(0) + kChannelInsert1Slot1, 4, label) == 7);
	assert(strcmp(label, "Route E") == 0);

	copyText(witchboard->routeNames[0], kHardwareNameLength, "Mono Filter");
	copyText(witchboard->routeNames[1], kHardwareNameLength, "Stereo FX");
	copyText(witchboard->fxNames[0], kHardwareNameLength, "Shared FX");
	assert(strcmp(algorithm->parameters[routeParam(0, kRouteOutputL)].name,
		"Route A output L") == 0);
	assert(strcmp(algorithm->parameters[kParamFx1L].name, "FX Send 1 L") == 0);
	assert(strcmp(algorithm->parameters[kParamSidechainMode].name, "Sidechain") == 0);
	assert(strcmp(algorithm->parameters[kParamSidechainKeyMode].enumStrings[0],
		"Trigger") == 0);
	assert(strcmp(algorithm->parameters[kParamSidechainKeyMode].enumStrings[1],
		"Gate") == 0);
	assert(strcmp(algorithm->parameters[kParamSidechainKeyMode].enumStrings[2],
		"Audio") == 0);
	assert(algorithm->parameters[kParamSidechainKeyMode].max == 2);
	assert(algorithm->parameters[kParamSidechainDepth].min == 0);
	assert(algorithm->parameters[kParamSidechainDepth].max == 100);
	assert(algorithm->parameters[kParamSidechainDepth].def == 90);
	assert(algorithm->parameters[kParamSidechainAttack].min == 0);
	assert(algorithm->parameters[kParamSidechainAttack].max == 50);
	assert(algorithm->parameters[kParamSidechainAttack].def == 0);
	assert(algorithm->parameters[kParamSidechainRelease].min == 5);
	assert(algorithm->parameters[kParamSidechainRelease].max == 800);
	assert(algorithm->parameters[kParamSidechainRelease].def == 120);
	assert(algorithm->parameters[kParamSidechainReleaseCurve].min == -300);
	assert(algorithm->parameters[kParamSidechainReleaseCurve].max == 200);
	assert(algorithm->parameters[kParamSidechainReleaseCurve].def == -120);
	assert(algorithm->parameters[kParamSidechainMakeup].min == 0);
	assert(algorithm->parameters[kParamSidechainMakeup].max == 6);
	assert(algorithm->parameters[kParamSidechainMakeup].def == 2);
	assert(strcmp(algorithm->parameters[kParamMasterMode].name, "Master output") == 0);
	assert(strcmp(algorithm->parameters[kParamMasterMode].enumStrings[0], "Split") == 0);
	assert(strcmp(algorithm->parameters[kParamMasterFilterEnable].name, "Filter enable") == 0);
	assert(algorithm->parameters[kParamMasterFilterEnable].min == 0);
	assert(algorithm->parameters[kParamMasterFilterEnable].max == 1);
	assert(algorithm->parameters[kParamMasterFilterEnable].def == 0);
	assert(strcmp(algorithm->parameters[kParamMasterFilterHpCutoff].name, "HP limit") == 0);
	assert(algorithm->parameters[kParamMasterFilterHpCutoff].min == 0);
	assert(algorithm->parameters[kParamMasterFilterHpCutoff].max == 100);
	assert(algorithm->parameters[kParamMasterFilterHpCutoff].def == 100);
	assert(strcmp(algorithm->parameters[kParamMasterFilterLpCutoff].name, "LP limit") == 0);
	assert(algorithm->parameters[kParamMasterFilterLpCutoff].min == 0);
	assert(algorithm->parameters[kParamMasterFilterLpCutoff].max == 100);
	assert(algorithm->parameters[kParamMasterFilterLpCutoff].def == 0);
	assert(strcmp(algorithm->parameters[kParamMasterFilterQ].name, "Filter Q") == 0);
	assert(algorithm->parameters[kParamMasterFilterQ].min == 0);
	assert(algorithm->parameters[kParamMasterFilterQ].max == 100);
	assert(algorithm->parameters[kParamMasterFilterQ].def == 0);
	assert(strcmp(algorithm->parameters[kParamMasterFilterSweep].name, "Filter sweep") == 0);
	assert(algorithm->parameters[kParamMasterFilterSweep].min == -100);
	assert(algorithm->parameters[kParamMasterFilterSweep].max == 100);
	assert(algorithm->parameters[kParamMasterFilterSweep].def == 0);
	const _NT_parameterPage& sidechainMasterPage = algorithm->parameterPages->pages[4];
	assert(strcmp(sidechainMasterPage.name, "Sidechain/Master") == 0);
	assert(sidechainMasterPage.numParams == kMasterPageParams);
	printf("PAGE %u %s (%u params):\n", 4u, sidechainMasterPage.name,
		sidechainMasterPage.numParams);
	for (int p = 0; p < sidechainMasterPage.numParams; ++p)
		printf("  %u: %s\n", sidechainMasterPage.params[p],
			algorithm->parameters[sidechainMasterPage.params[p]].name);
	const uint8_t expectedMasterParams[kMasterPageParams] = {
		kParamSidechainMode,
		kParamSidechainKeyInput,
		kParamSidechainKeyMode,
		kParamSidechainDepth,
		kParamSidechainAttack,
		kParamSidechainRelease,
		kParamSidechainReleaseCurve,
		kParamSidechainMakeup,
		kParamMasterFilterEnable,
		kParamMasterFilterHpCutoff,
		kParamMasterFilterLpCutoff,
		kParamMasterFilterQ,
		kParamMasterFilterSweep,
		kParamEq1Freq, kParamEq1Gain, kParamEq1Q,
		kParamEq2Freq, kParamEq2Gain, kParamEq2Q,
		kParamEq3Freq, kParamEq3Gain, kParamEq3Q,
		kParamMasterMode,
		kParamMasterSendL,
		kParamMasterSendR,
		kParamMasterReturnL,
		kParamMasterReturnR,
	};
	for (int p = 0; p < kMasterPageParams; ++p)
		assert(sidechainMasterPage.params[p] == expectedMasterParams[p]);
	SidechainRuntime testSidechain = {};
	const float triggerGain = processSidechain(testSidechain, 5.0f, kKeyTrigger,
		90, 0, 120, 0);
	assert(triggerGain > 0.09f);
	assert(triggerGain < 0.12f);

	SidechainRuntime heldTriggerSidechain = {};
	const float firstHeldGain = processSidechain(heldTriggerSidechain, 5.0f, kKeyTrigger,
		100, 0, 5, 0);
	assert(firstHeldGain < 0.01f);
	for (int i = 0; i < 300; ++i)
		processSidechain(heldTriggerSidechain, 5.0f, kKeyTrigger, 100, 0, 5, 0);
	assert(heldTriggerSidechain.gain > 0.99f);
	processSidechain(heldTriggerSidechain, 0.0f, kKeyTrigger, 100, 0, 5, 0);
	assert(processSidechain(heldTriggerSidechain, 5.0f, kKeyTrigger, 100, 0, 5, 0)
		< 0.01f);

	SidechainRuntime linearCurveSidechain = {};
	SidechainRuntime holdCurveSidechain = {};
	processSidechain(linearCurveSidechain, 5.0f, kKeyTrigger, 90, 0, 100, 0);
	processSidechain(holdCurveSidechain, 5.0f, kKeyTrigger, 90, 0, 100, -200);
	for (int i = 0; i < 600; ++i)
	{
		processSidechain(linearCurveSidechain, 0.0f, kKeyTrigger, 90, 0, 100, 0);
		processSidechain(holdCurveSidechain, 0.0f, kKeyTrigger, 90, 0, 100, -200);
	}
	assert(holdCurveSidechain.gain < linearCurveSidechain.gain);

	for (int depth = 0; depth <= 100; ++depth)
	{
		for (int curve = -300; curve <= 200; curve += 25)
		{
			SidechainRuntime finiteSidechain = {};
			const float gain = processSidechain(finiteSidechain, 5.0f, kKeyTrigger,
				depth, 0, 5, curve);
			assert(isfinite(gain));
			assert(gain >= 0.0f && gain <= 1.0f);
		}
	}

	SidechainRuntime zeroKeySidechain = {};
	assertClose(processSidechain(zeroKeySidechain, 0.0f, kKeyTrigger,
		100, 0, 5, 0), 1.0f);
	assertClose(processSidechain(zeroKeySidechain, 0.0f, kKeyGate,
		100, 0, 5, 0), 1.0f);

	SidechainRuntime gateSidechain = {};
	for (int i = 0; i < 100; ++i)
		assert(processSidechain(gateSidechain, 5.0f, kKeyGate, 75, 0, 50, 0)
			< 0.26f);
	processSidechain(gateSidechain, 0.0f, kKeyGate, 75, 0, 50, 0);
	for (int i = 0; i < 3000; ++i)
		processSidechain(gateSidechain, 0.0f, kKeyGate, 75, 0, 50, 0);
	assert(gateSidechain.gain > 0.99f);

	SidechainRuntime audioSidechain = {};
	float audioGain = processSidechain(audioSidechain, 0.5f, kKeyAudio, 80, 0, 50, 0);
	assert(audioGain > 0.59f);
	assert(audioGain < 0.61f);
	for (int i = 0; i < 3000; ++i)
		audioGain = processSidechain(audioSidechain, 0.0f, kKeyAudio, 80, 0, 50, -200);
	assert(audioGain > 0.60f);
	assert(audioGain < 1.0f);
	for (int i = 0; i < 24000; ++i)
		audioGain = processSidechain(audioSidechain, 0.0f, kKeyAudio, 80, 0, 50, 200);
	assert(audioGain > 0.99f);
	assert(parameterString(algorithm, channelBase(0) + kChannelInsert1Slot1, 0, label)
		== 11);
	assert(strcmp(label, "Mono Filter") == 0);
	values[channelBase(0) + kChannelInsert1Slot2] = 1;
	algorithm->v = values.data();
	algorithm->vIncludingCommon = values.data();
	assert(parameterString(algorithm, channelBase(0) + kChannelInsert1, 2, label) == 6);
	assert(strcmp(label, "Slot 2") == 0);
	copyText(witchboard->slotNames[0][2], kSlotNameLength, "Filter");
	copyText(witchboard->slotNames[1][3], kSlotNameLength, "Scatter");
	assert(parameterString(algorithm, channelBase(0) + kChannelInsert1, 2, label) == 6);
	assert(strcmp(label, "Filter") == 0);
	assert(parameterString(algorithm, channelBase(0) + kChannelInsert2, 4, label) == 7);
	assert(strcmp(label, "Scatter") == 0);

	for (int channel = 0; channel < 4; ++channel)
	{
		const _NT_parameterPage& page = algorithm->parameterPages->pages[5 + channel];
		assert(page.numParams == kNumChannelParams);
		for (int p = 0; p < kNumChannelParams; ++p)
			assert(page.params[p] == channelBase(channel) + p);
	}
	for (int channel = 0; channel < kMaxChannels; ++channel)
	{
		const _NT_parameterPage& page = maxAlgorithm->parameterPages->pages[5 + channel];
		assert(page.numParams == kNumChannelParams);
		for (int p = 0; p < kNumChannelParams; ++p)
			assert(page.params[p] == channelBase(channel) + p);
	}

	values[kParamFadeMs] = 0;
	stepOnce(algorithm, values);
	for (int channel = 0; channel < 4; ++channel)
	{
		assert(witchboard->runtime[channel].insertState[0] == 0);
		assert(witchboard->runtime[channel].insertState[1] == 0);
	}

	// Regression: a native/MIDI parameter change must invalidate the cached
	// insert state immediately, so the next audio block consumes the new route
	// without requiring an unrelated Gain change.
	values[channelBase(1) + kChannelInsert2] = 2;
	algorithm->v = values.data();
	algorithm->vIncludingCommon = values.data();
	parameterChanged(algorithm, channelBase(1) + kChannelInsert2);
	assert(witchboard->runtime[1].insertState[1] == -1);
	stepOnce(algorithm, values);
	assert(witchboard->runtime[0].insertState[0] == 0);
	assert(witchboard->runtime[0].insertState[1] == 0);
	assert(witchboard->runtime[1].insertState[0] == 0);
	assert(witchboard->runtime[1].insertState[1] == 2);
	assert(witchboard->runtime[2].insertState[0] == 0);
	assert(witchboard->runtime[2].insertState[1] == 0);
	assert(witchboard->runtime[3].insertState[0] == 0);
	assert(witchboard->runtime[3].insertState[1] == 0);

	values[channelBase(1) + kChannelInsert2Slot2] = 4;
	stepOnce(algorithm, values);
	assert(selectedRoute(witchboard, 1, 1, 2) == 4);
	assert(selectedRoute(witchboard, 1, 1, 0) == -1);

	values[channelBase(3) + kChannelInsert1] = 3;
	stepOnce(algorithm, values);
	assert(witchboard->runtime[0].insertState[0] == 0);
	assert(witchboard->runtime[1].insertState[0] == 0);
	assert(witchboard->runtime[1].insertState[1] == 2);
	assert(witchboard->runtime[2].insertState[0] == 0);
	assert(witchboard->runtime[3].insertState[0] == 3);
	assert(witchboard->runtime[3].insertState[1] == 0);

	values[channelBase(3) + kChannelInsert1] = 4;
	stepOnce(algorithm, values);
	assert(witchboard->runtime[3].insertState[0] == 3);

	values[channelBase(1) + kChannelFx1Mix] = 100;
	stepOnce(algorithm, values);
	assert(witchboard->runtime[0].fx1Mix.parameterValue == 0);
	assert(witchboard->runtime[1].fx1Mix.parameterValue == 100);
	assert(witchboard->runtime[2].fx1Mix.parameterValue == 0);
	assert(witchboard->runtime[3].fx1Mix.parameterValue == 0);

	const int32_t routingSpecs[] = { 2 };
	_NT_algorithmRequirements routingRequirements = {};
	calculateRequirements(routingRequirements, routingSpecs);
	_NT_algorithmMemoryPtrs routingMemory = allocateMemory(routingRequirements);
	_NT_algorithm* routingAlgorithm = constructWitchboard(
		routingMemory, routingRequirements, routingSpecs);
	WitchboardAlgorithm* routingWitchboard =
		static_cast<WitchboardAlgorithm*>(routingAlgorithm);
	std::vector<int16_t> routingValues(routingRequirements.numParameters);
	for (uint32_t i = 0; i < routingRequirements.numParameters; ++i)
		routingValues[i] = routingAlgorithm->parameters[i].def;
	routingAlgorithm->v = routingValues.data();
	routingAlgorithm->vIncludingCommon = routingValues.data();

	const int mainBus = kNT_numInputBusses + 1;
	const int mainBusR = mainBus + 1;
	const int bypassBus = mainBusR + 1;
	const int routeSendBus = kNT_numInputBusses + kNT_numOutputBusses + 1;
	const int routeReturnBus = routeSendBus + 1;
	const int keyBus = routeReturnBus + 1;
	const int masterSendBus = keyBus + 1;
	const int masterReturnBus = masterSendBus + 1;
	const int masterReturnBusR = masterReturnBus + 1;
	routingValues[kParamFadeMs] = 0;
	routingValues[kParamMainL] = mainBus;
	routingValues[kParamMainR] = mainBusR;
	routingValues[routeParam(0, kRouteOutputL)] = routeSendBus;
	routingValues[routeParam(0, kRouteReturnL)] = routeReturnBus;
	routingValues[channelBase(0) + kChannelInputL] = 1;
	routingValues[channelBase(1) + kChannelInputL] = 2;
	routingValues[channelBase(0) + kChannelInsert1] = 1;

	std::vector<float> buses(kNT_lastBus * 4, 0.0f);
	fillBus(buses, 1, 1.0f);
	fillBus(buses, 2, 2.0f);
	fillBus(buses, routeReturnBus, 10.0f);
	step(routingAlgorithm, buses.data(), 1);
	assertBus(buses, routeSendBus, 1.0f);
	assertBus(buses, mainBus, 12.0f);

	routingValues[kParamBypassL] = bypassBus;
	routingValues[channelBase(1) + kChannelOutputPath] = kOutputPathBypass;
	routingValues[kParamMasterMode] = kMasterSum;
	buses.assign(kNT_lastBus * 4, 0.0f);
	fillBus(buses, 1, 1.0f);
	fillBus(buses, 2, 2.0f);
	fillBus(buses, routeReturnBus, 10.0f);
	step(routingAlgorithm, buses.data(), 1);
	assertBus(buses, routeSendBus, 1.0f);
	assertBus(buses, bypassBus, 0.0f);
	assertBus(buses, mainBus, 12.0f);

	routingValues[channelBase(1) + kChannelOutputPath] = kOutputPathMain;
	routingValues[kParamMasterMode] = kMasterSplit;
	routingValues[kParamSidechainMode] = 1;
	routingValues[kParamSidechainKeyInput] = keyBus;
	routingValues[kParamSidechainKeyMode] = kKeyTrigger;
	routingValues[kParamSidechainDepth] = 90;
	routingValues[kParamSidechainAttack] = 0;
	routingValues[kParamSidechainRelease] = 120;
	routingValues[kParamSidechainReleaseCurve] = -120;
	buses.assign(kNT_lastBus * 4, 0.0f);
	fillBus(buses, 1, 1.0f);
	fillBus(buses, 2, 2.0f);
	fillBus(buses, keyBus, 5.0f);
	fillBus(buses, routeReturnBus, 10.0f);
	step(routingAlgorithm, buses.data(), 1);
	assertBus(buses, bypassBus, 0.0f);
	assert(busSample(buses, mainBus, 0) > 1.4f);
	assert(busSample(buses, mainBus, 0) < 1.6f);
	assert(routingWitchboard->sidechain.gain < 1.0f);

	routingValues[kParamSidechainMode] = 0;
	routingValues[kParamSidechainKeyInput] = 0;
	routingValues[kParamMasterFilterEnable] = 1;
	routingValues[kParamMasterFilterHpCutoff] = 100;
	routingValues[kParamMasterFilterLpCutoff] = 0;
	routingValues[kParamMasterFilterQ] = 0;
	routingValues[kParamMasterFilterSweep] = 100;
	routingValues[kParamMasterMode] = kMasterSplit;
	buses.assign(kNT_lastBus * 4, 0.0f);
	fillBus(buses, 1, 1.0f);
	fillBus(buses, 2, 2.0f);
	fillBus(buses, routeReturnBus, 10.0f);
	step(routingAlgorithm, buses.data(), 1);
	assert(fabsf(busSample(buses, mainBus, 0)) < 1.0f);
	assertBus(buses, bypassBus, 0.0f);
	routingValues[kParamMasterFilterHpCutoff] = 100;
	routingValues[kParamMasterFilterLpCutoff] = 0;
	routingValues[kParamMasterFilterSweep] = -100;
	memset(&routingWitchboard->masterFilter, 0, sizeof(routingWitchboard->masterFilter));
	buses.assign(kNT_lastBus * 4, 0.0f);
	fillBus(buses, 1, 1.0f);
	fillBus(buses, 2, 2.0f);
	fillBus(buses, routeReturnBus, 10.0f);
	step(routingAlgorithm, buses.data(), 1);
	assert(busSample(buses, mainBus, 0) >= 0.0f);
	assert(isfinite(busSample(buses, mainBus, 0)));
	routingValues[kParamMasterFilterSweep] = 80;
	buses.assign(kNT_lastBus * 4, 0.0f);
	fillBus(buses, 1, 1.0f);
	fillBus(buses, 2, 2.0f);
	fillBus(buses, routeReturnBus, 10.0f);
	step(routingAlgorithm, buses.data(), 1);
	assert(isfinite(busSample(buses, mainBus, 0)));
	assert(fabsf(busSample(buses, mainBus, 0) - 12.0f) > 0.0001f);
	routingValues[kParamMasterFilterEnable] = 0;
	routingValues[kParamMasterFilterHpCutoff] = 100;
	routingValues[kParamMasterFilterLpCutoff] = 0;
	routingValues[kParamMasterFilterSweep] = 0;
	memset(&routingWitchboard->masterFilter, 0, sizeof(routingWitchboard->masterFilter));
	buses.assign(kNT_lastBus * 4, 0.0f);
	fillBus(buses, 1, 1.0f);
	fillBus(buses, 2, 2.0f);
	fillBus(buses, routeReturnBus, 10.0f);
	step(routingAlgorithm, buses.data(), 1);
	assertBus(buses, mainBus, 12.0f);

	routingValues[kParamMasterMode] = kMasterInsert;
	routingValues[kParamMasterSendL] = masterSendBus;
	routingValues[kParamMasterReturnL] = masterReturnBus;
	routingValues[kParamMasterReturnR] = masterReturnBusR;
	buses.assign(kNT_lastBus * 4, 0.0f);
	fillBus(buses, 1, 1.0f);
	fillBus(buses, 2, 2.0f);
	fillBus(buses, routeReturnBus, 10.0f);
	fillBus(buses, masterReturnBus, 100.0f);
	fillBus(buses, masterReturnBusR, 200.0f);
	step(routingAlgorithm, buses.data(), 1);
	assertBus(buses, masterSendBus, 12.0f);
	assertBus(buses, mainBus, 100.0f);
	assertBus(buses, mainBusR, 200.0f);

	routingValues[kParamMasterMode] = kMasterSplit;
	routingValues[kParamMasterSendL] = 0;
	routingValues[kParamMasterReturnL] = 0;
	routingValues[kParamMasterReturnR] = 0;
	routingValues[kParamBypassL] = 0;
	routingValues[channelBase(1) + kChannelOutputPath] = kOutputPathMain;

	routingValues[channelBase(1) + kChannelInsert1] = 1;
	buses.assign(kNT_lastBus * 4, 0.0f);
	fillBus(buses, 1, 1.0f);
	fillBus(buses, 2, 2.0f);
	fillBus(buses, routeReturnBus, 10.0f);
	step(routingAlgorithm, buses.data(), 1);
	assert(routingValues[channelBase(0) + kChannelInsert1] == 1);
	assert(routingValues[channelBase(1) + kChannelInsert1] == 1);
	assertBus(buses, routeSendBus, 3.0f);
	assertBus(buses, mainBus, 20.0f);

	routingValues[channelBase(1) + kChannelInsert2] = 1;
	buses.assign(kNT_lastBus * 4, 0.0f);
	fillBus(buses, 1, 1.0f);
	fillBus(buses, 2, 2.0f);
	fillBus(buses, routeReturnBus, 10.0f);
	step(routingAlgorithm, buses.data(), 1);
	assert(routingValues[channelBase(1) + kChannelInsert1] == 1);
	assert(routingValues[channelBase(1) + kChannelInsert2] == 1);
	assertBus(buses, routeSendBus, 3.0f);
	assertBus(buses, mainBus, 20.0f);

	routingValues[kParamRepeatProtection] = 0;
	buses.assign(kNT_lastBus * 4, 0.0f);
	fillBus(buses, 1, 1.0f);
	fillBus(buses, 2, 2.0f);
	fillBus(buses, routeReturnBus, 10.0f);
	step(routingAlgorithm, buses.data(), 1);
	assertBus(buses, routeSendBus, 13.0f);
	assertBus(buses, mainBus, 20.0f);
	routingValues[kParamRepeatProtection] = 1;

	routingValues[kParamFadeMs] = 2;
	routingValues[channelBase(0) + kChannelGain] = 0;
	for (int change = 0; change < 64; ++change)
	{
		const int state = change % kNumInsertStates;
		routingValues[channelBase(0) + kChannelInsert1] = state;
		buses.assign(kNT_lastBus * 4, 0.0f);
		fillBus(buses, 1, 1.0f);
		fillBus(buses, 2, 2.0f);
		fillBus(buses, routeReturnBus, 10.0f);
		step(routingAlgorithm, buses.data(), 1);
		assert(routingValues[channelBase(0) + kChannelInsert1] == state);
		assert(routingWitchboard->runtime[0].insertState[0] == state);
		assert(routingWitchboard->runtime[0].gain.parameterValue == 0);
		assertClose(routingWitchboard->runtime[0].gain.value, 1.0f);
	}

	printf("PASS: Witchboard EQ has direct routes, stable rapid switching, triggered gain shaping, master SVF filter, master insert, and repeat protection (SRAM %u/%u/%u/%u, DRAM %u/%u/%u/%u host bytes for 1/4/8/11 channels).\n",
		oneChannelRequirements.sram, requirements.sram,
		eightChannelRequirements.sram, maxChannelRequirements.sram,
		oneChannelRequirements.dram, requirements.dram,
		eightChannelRequirements.dram, maxChannelRequirements.dram);
	freeMemory(routingMemory);
	freeMemory(maxMemory);
	freeMemory(memory);
	return 0;
}
