#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <new>

#include <distingnt/api.h>
#include <distingnt/serialisation.h>

#ifndef _NT_DRAM_SECTION
#define _NT_DRAM_SECTION
#endif

#ifndef WITCHBOARD_ENABLE_DRAM_CODE
#undef _NT_DRAM_SECTION
#define _NT_DRAM_SECTION
#endif

namespace
{

constexpr int kMaxChannels = 10;
constexpr int kNumRoutes = 6;
constexpr int kNumFx = 4;
constexpr int kNumFxParams = 7;
constexpr int kRouteOutputBase = 2 + kNumFx;
constexpr int kNumInserts = 2;
constexpr int kNumRouteParams = 6;
constexpr int kNumInsertStates = 4;
constexpr int kInsertParameterMax = 4;
constexpr int kNumOutputPairs = kRouteOutputBase + kNumRoutes;
constexpr int kHardwareNameLength = 24;
constexpr int kSlotNameLength = 20;

constexpr int kInsertDry = 0;

enum RouteField
{
	kRouteOutputL,
	kRouteOutputR,
	kRouteReturnL,
	kRouteReturnR,
	kRouteSendWidth,
	kRouteReturnWidth,
};

enum OutputPath
{
	kOutputPathMain,
	kOutputPathBypass,
};

enum Width
{
	kWidthMono,
	kWidthStereo,
};

enum MasterMode
{
	kMasterSplit,
	kMasterSum,
	kMasterInsert,
};

enum MasterFilterMode
{
	kFilterLowpass,
	kFilterHighpass,
};

constexpr int kParamFadeMs = 0;
constexpr int kParamRoutes = kParamFadeMs + 1;
constexpr int kParamMainL = kParamRoutes + kNumRoutes * kNumRouteParams;
constexpr int kParamMainR = kParamMainL + 1;
constexpr int kParamBypassL = kParamMainL + 2;
constexpr int kParamBypassR = kParamMainL + 3;
constexpr int kParamFx1L = kParamMainL + 4;
constexpr int kParamFx1R = kParamMainL + 5;
constexpr int kParamFx1Width = kParamMainL + 6;
constexpr int kParamFx1ReturnL = kParamMainL + 7;
constexpr int kParamFx1ReturnR = kParamMainL + 8;
constexpr int kParamFx1ReturnWidth = kParamMainL + 9;
constexpr int kParamFx1ReturnPath = kParamMainL + 10;
constexpr int kParamFx2L = kParamMainL + 11;
constexpr int kParamFx2R = kParamMainL + 12;
constexpr int kParamFx2Width = kParamMainL + 13;
constexpr int kParamFx2ReturnL = kParamMainL + 14;
constexpr int kParamFx2ReturnR = kParamMainL + 15;
constexpr int kParamFx2ReturnWidth = kParamMainL + 16;
constexpr int kParamFx2ReturnPath = kParamMainL + 17;
constexpr int kParamRepeatProtection = kParamFx1L + kNumFx * kNumFxParams;
constexpr int kParamSidechainMode = kParamRepeatProtection + 1;
constexpr int kParamSidechainKeyInput = kParamSidechainMode + 1;
constexpr int kParamSidechainDepth = kParamSidechainMode + 2;
constexpr int kParamSidechainLookahead = kParamSidechainMode + 3;
constexpr int kParamSidechainEnvLength = kParamSidechainMode + 4;
constexpr int kParamSidechainCurve = kParamSidechainMode + 5;
constexpr int kParamSidechainSmooth = kParamSidechainMode + 6;
constexpr int kParamMasterFilterEnable = kParamSidechainSmooth + 1;
constexpr int kParamMasterFilterHpCutoff = kParamMasterFilterEnable + 1;
constexpr int kParamMasterFilterLpCutoff = kParamMasterFilterEnable + 2;
constexpr int kParamMasterFilterQ = kParamMasterFilterEnable + 3;
constexpr int kParamMasterFilterSweep = kParamMasterFilterEnable + 4;
constexpr int kParamMasterMode = kParamMasterFilterSweep + 1;
constexpr int kParamMasterSendL = kParamMasterMode + 1;
constexpr int kParamMasterSendR = kParamMasterMode + 2;
constexpr int kParamMasterReturnL = kParamMasterMode + 3;
constexpr int kParamMasterReturnR = kParamMasterMode + 4;
constexpr int kParamMasterGain = kParamMasterReturnR + 1;
constexpr int kParamBypassOffset = kParamMasterGain + 1;
constexpr int kNumGlobalParams = kParamBypassOffset + 1;

enum ChannelParam
{
	kChannelEnable,
	kChannelInputL,
	kChannelInputR,
	kChannelGain,
	kChannelInsert1,
	kChannelInsert1Slot1,
	kChannelInsert1Slot2,
	kChannelInsert1Slot3,
	kChannelSendSelect,
	kChannelInsert2,
	kChannelInsert2Slot1,
	kChannelInsert2Slot2,
	kChannelInsert2Slot3,
	kChannelOutputPath,
	kChannelSendAmount,

	kNumChannelParams,
};

constexpr int kGlobalPageParams = 2;
constexpr int kRouteSetupParams = kParamMainL - 1;
constexpr int kFinalOutputParams = kParamFx1L - kParamMainL + 1; // Bypass Offset
constexpr int kFxSetupParams = kNumFx * kNumFxParams;
constexpr int kMasterPageParams = kNumGlobalParams - kParamSidechainMode - 1;
constexpr int kMaxParams = kNumGlobalParams + kMaxChannels * kNumChannelParams + 2;
static_assert(kMaxParams == 241, "10-channel/6-route/4-send parameter budget changed");
static_assert(kMaxParams <= 241, "disting NT supports at most 241 parameters per algorithm");
static_assert(kNumGlobalParams == 89, "global parameter count changed");
static_assert(kParamMainL == 37, "final output page indices changed");
static_assert(kParamFx1L == 41, "FX setup page indices changed");
static_assert(kParamRepeatProtection == 69, "repeat protection index changed");
static_assert(kParamSidechainMode == 70, "master page indices changed");
static_assert(kNumChannelParams == 15, "channel parameter count changed");
static_assert(kChannelGain == 3, "Gain offset changed");
static_assert(kChannelInsert1 == 4, "Insert 1 offset changed");
static_assert(kChannelSendSelect == 8, "Send select offset changed");
static_assert(kChannelInsert2 == 9, "Insert 2 offset changed");
static_assert(kChannelOutputPath == 13, "Output path offset changed");
static_assert(kChannelSendAmount == 14, "Send amount offset changed");
static_assert(kMaxParams <= 256, "parameter page indices are uint8_t");

static char const* const offOnStrings[] = {
	"Off", "On",
};

static char const* const outputPathStrings[] = {
	"Main", "Bypass",
};

static char const* const masterModeStrings[] = {
	"Split", "Sum", "Insert",
};

static char const* const widthStrings[] = {
	"Mono", "Stereo",
};

static char const* const channelPageNames[kMaxChannels] = {
	"Channel 1", "Channel 2", "Channel 3", "Channel 4",
	"Channel 5", "Channel 6", "Channel 7", "Channel 8",
	"Channel 9", "Channel 10",
};

static char const* const channelSuffixes[kNumChannelParams] = {
	"Enable",
	"Input/Left",
	"Right Input",
	"Gain",
	"Insert 1",
	"Insert 1 slot 1",
	"Insert 1 slot 2",
	"Insert 1 slot 3",
	"Send select",
	"Insert 2",
	"Insert 2 slot 1",
	"Insert 2 slot 2",
	"Insert 2 slot 3",
	"Output path",
	"Send amount",
};

static char const* const defaultRouteNames[kNumRoutes] = {
	"Route A", "Route B", "Route C", "Route D",
	"Route E", "Route F",
};

static char const* const defaultFxNames[kNumFx] = {
	"FX Send 1", "FX Send 2", "FX Send 3", "FX Send 4",
};

static char const* const defaultSlotNames[kNumInserts][kNumInsertStates] = {
	{ "Dry", "Slot 1", "Slot 2", "Slot 3" },
	{ "Dry", "Slot 1", "Slot 2", "Slot 3" },
};

static char const* const insertStateStrings[] = {
	"Dry", "Slot 1", "Slot 2", "Slot 3", "Slot 3",
};

static char const* const defaultRouteParameterNames[kNumRoutes][kNumRouteParams] = {
	{
		"Route A output L", "Route A output R", "Route A return L",
		"Route A return R", "Route A send width", "Route A return width",
	},
	{
		"Route B output L", "Route B output R", "Route B return L",
		"Route B return R", "Route B send width", "Route B return width",
	},
	{
		"Route C output L", "Route C output R", "Route C return L",
		"Route C return R", "Route C send width", "Route C return width",
	},
	{
		"Route D output L", "Route D output R", "Route D return L",
		"Route D return R", "Route D send width", "Route D return width",
	},
	{
		"Route E output L", "Route E output R", "Route E return L",
		"Route E return R", "Route E send width", "Route E return width",
	},
	{
		"Route F output L", "Route F output R", "Route F return L",
		"Route F return R", "Route F send width", "Route F return width",
	},
};

static char const* const defaultFxParameterNames[kNumFx][kNumFxParams] = {
	{
		"FX Send 1 L", "FX Send 1 R", "FX Send 1 width",
		"FX 1 return L", "FX 1 return R", "FX 1 return width",
		"FX 1 return path",
	},
	{
		"FX Send 2 L", "FX Send 2 R", "FX Send 2 width",
		"FX 2 return L", "FX 2 return R", "FX 2 return width",
		"FX 2 return path",
	},
	{
		"FX Send 3 L", "FX Send 3 R", "FX Send 3 width",
		"FX 3 return L", "FX 3 return R", "FX 3 return width",
		"FX 3 return path",
	},
	{
		"FX Send 4 L", "FX Send 4 R", "FX Send 4 width",
		"FX 4 return L", "FX 4 return R", "FX 4 return width",
		"FX 4 return path",
	},
};

constexpr int routeParam(int route, int field)
{
	return kParamRoutes + route * kNumRouteParams + field;
}

static const uint8_t globalPageParams[kGlobalPageParams] = {
	kParamFadeMs, kParamRepeatProtection,
};

static const uint8_t routeSetupPageParams[kRouteSetupParams] = {
	routeParam(0, kRouteOutputL), routeParam(0, kRouteOutputR),
	routeParam(0, kRouteReturnL), routeParam(0, kRouteReturnR),
	routeParam(0, kRouteSendWidth), routeParam(0, kRouteReturnWidth),
	routeParam(1, kRouteOutputL), routeParam(1, kRouteOutputR),
	routeParam(1, kRouteReturnL), routeParam(1, kRouteReturnR),
	routeParam(1, kRouteSendWidth), routeParam(1, kRouteReturnWidth),
	routeParam(2, kRouteOutputL), routeParam(2, kRouteOutputR),
	routeParam(2, kRouteReturnL), routeParam(2, kRouteReturnR),
	routeParam(2, kRouteSendWidth), routeParam(2, kRouteReturnWidth),
	routeParam(3, kRouteOutputL), routeParam(3, kRouteOutputR),
	routeParam(3, kRouteReturnL), routeParam(3, kRouteReturnR),
	routeParam(3, kRouteSendWidth), routeParam(3, kRouteReturnWidth),
	routeParam(4, kRouteOutputL), routeParam(4, kRouteOutputR),
	routeParam(4, kRouteReturnL), routeParam(4, kRouteReturnR),
	routeParam(4, kRouteSendWidth), routeParam(4, kRouteReturnWidth),
	routeParam(5, kRouteOutputL), routeParam(5, kRouteOutputR),
	routeParam(5, kRouteReturnL), routeParam(5, kRouteReturnR),
	routeParam(5, kRouteSendWidth), routeParam(5, kRouteReturnWidth),
};

static const uint8_t finalOutputPageParams[kFinalOutputParams] = {
	kParamMainL, kParamMainR,
	kParamBypassL, kParamBypassR, kParamBypassOffset,
};

static uint8_t fxSetupPageParams[kFxSetupParams];

constexpr int fxParam(int fx, int field)
{
	return kParamFx1L + fx * kNumFxParams + field;
}

static const uint8_t masterPageParams[kMasterPageParams] = {
	kParamSidechainMode, kParamSidechainKeyInput, kParamSidechainDepth,
	kParamSidechainLookahead, kParamSidechainEnvLength, kParamSidechainCurve,
	kParamSidechainSmooth,
	kParamMasterFilterEnable, kParamMasterFilterHpCutoff, kParamMasterFilterLpCutoff,
	kParamMasterFilterQ, kParamMasterFilterSweep, kParamMasterMode,
	kParamMasterSendL, kParamMasterSendR, kParamMasterReturnL, kParamMasterReturnR,
	kParamMasterGain,
};

static const _NT_specification specifications[] = {
	{ .name = "Channels", .min = 1, .max = kMaxChannels, .def = kMaxChannels, .type = kNT_typeGeneric },
};

struct SmoothedValue
{
	int parameterValue;
	int samplesRemaining;
	float target;
	float value;
	float increment;
};

struct ChannelRuntime
{
	bool initialised;
	int8_t insertState[kNumInserts];
	int insertSamplesRemaining[kNumInserts];
	float insertGain[kNumInserts][kNumInsertStates];
	float insertIncrement[kNumInserts][kNumInsertStates];
	SmoothedValue gain;
	SmoothedValue fxMix[kNumFx];
};

// Four fixed-send MIDI targets plus a fifth target following Send select.
struct SendMidiMapping
{
	int16_t channel, cc, minimum, maximum, pickup;
	int16_t previous;
	bool caught;
};

struct SendState
{
	int16_t levels[kNumFx];
	SendMidiMapping midi[kNumFx + 1];
	int selected, displayed;
	bool initialised, restorePending, publishing;
};

struct OutputPair
{
	float* left;
	float* right;
	bool singleFrame;
};

struct RampSmoothRuntime
{
	float value, target, increment;
	int remaining, samples;
};

struct SidechainRuntime
{
	bool initialised, keyHigh;
	int remaining;
	double value, increment, multiplier;
	float gain;
	RampSmoothRuntime smooth;
};

// Stereo rings belong to each instance's DRAM; insert timing is route sized.
constexpr int kInsertDelayCapacity = 1921; // 20 ms at 96 kHz + current sample
constexpr int kMainDelayCapacity = 961;    // 10 ms at 96 kHz + current sample
constexpr int kBypassDelayCapacity = 9601; // 100 ms at 96 kHz + current sample
struct StereoDelay
{
	float* data;
	int capacity, write, valid, current, target, requested, fadePosition, fadeSamples;
	bool initialised;
};

struct SvfRuntime
{
	float ic1eq;
	float ic2eq;
};

struct MasterFilterRuntime
{
	SvfRuntime hpLeft;
	SvfRuntime hpRight;
	SvfRuntime lpLeft;
	SvfRuntime lpRight;
};

typedef uint8_t ChannelPage[kNumChannelParams];

static _NT_parameter parameterDefs[kMaxParams];
static bool parameterTablesBuilt = false;

void buildParameters();

struct WitchboardAlgorithm : public _NT_algorithm
{
	WitchboardAlgorithm(int channels, uint8_t* dram);

	void buildPages();
	void setDefaultNames();

	int numChannels;
	_NT_parameterPages pages;
	_NT_parameterPage* pageDefs;
	ChannelPage* channelPages;
	ChannelRuntime* runtime;
	SendState* sends;
	SidechainRuntime sidechain;
	MasterFilterRuntime masterFilter;
	StereoDelay mainDelay, bypassDelay, insertDryDelay, insertBypassDryDelay, insertKeyDelay;
	StereoDelay* insertReturnDelays;
	int16_t insertLatencies[kNumRoutes]; // tenths of a millisecond
	int insertSelected, insertDisplayed;
	bool insertInitialised, insertRestorePending, insertPublishing;
	uint8_t finalOutputParams[kFinalOutputParams + 2];
	int insertSelectParam() const { return kNumGlobalParams + numChannels * kNumChannelParams; }
	int insertLatencyParam() const { return insertSelectParam() + 1; }
	bool latencyInitialised;
	int previousAuto, previousEffective, manualTrim;
	bool savedTrim;
	int cachedLengthControl, cachedCurveControl, envSamples;
	float cachedSampleRate, curveBeta;
	SmoothedValue masterGain;
	bool masterGainInitialised;
	char channelNames[kMaxChannels][kHardwareNameLength];
	char routeNames[kNumRoutes][kHardwareNameLength];
	char fxNames[kNumFx][kHardwareNameLength];
	char slotNames[kNumInserts][kNumInsertStates][kSlotNameLength];
};

inline size_t alignedSize(size_t size, size_t alignment)
{
	return (size + alignment - 1) & ~(alignment - 1);
}

template <typename T>
size_t addStorage(size_t size, int count)
{
	return alignedSize(size, alignof(T)) + sizeof(T) * count;
}

template <typename T>
T* takeStorage(uint8_t*& cursor, int count)
{
	const uintptr_t address = alignedSize(reinterpret_cast<uintptr_t>(cursor), alignof(T));
	cursor = reinterpret_cast<uint8_t*>(address + sizeof(T) * count);
	return reinterpret_cast<T*>(address);
}

size_t requiredSram(int)
{
	return sizeof(WitchboardAlgorithm);
}

// Keep the algorithm and master DSP in SRAM. Allocate channel-dependent
// pages and routing state in the API's DRAM pool to reduce SRAM pressure.
size_t requiredDram(int channels)
{
	size_t size = 0;
	size = addStorage<_NT_parameterPage>(size, 5 + channels);
	size = addStorage<ChannelPage>(size, channels);
	size = addStorage<ChannelRuntime>(size, channels);
	size = addStorage<SendState>(size, channels);
	size = addStorage<float>(size, 2 * (kMainDelayCapacity + kBypassDelayCapacity));
	size = addStorage<_NT_parameter>(size, kNumGlobalParams + channels * kNumChannelParams + 2);
	size = addStorage<float>(size, 2 * kInsertDelayCapacity * 3);
	size = addStorage<StereoDelay>(size, kNumRoutes);
	size = addStorage<float>(size, 2 * kInsertDelayCapacity * kNumRoutes);
	return size;
}

WitchboardAlgorithm::WitchboardAlgorithm(int channels, uint8_t* dram)
	: numChannels(channels)
{
	pageDefs = takeStorage<_NT_parameterPage>(dram, 5 + numChannels);
	channelPages = takeStorage<ChannelPage>(dram, numChannels);
	runtime = takeStorage<ChannelRuntime>(dram, numChannels);
	sends = takeStorage<SendState>(dram, numChannels);
	memset(sends, 0, sizeof(SendState) * numChannels);
	for (int channel = 0; channel < numChannels; ++channel)
		for (int target = 0; target <= kNumFx; ++target)
		{
			sends[channel].midi[target].maximum = 100;
			sends[channel].midi[target].previous = -1;
		}
	memset(runtime, 0, sizeof(ChannelRuntime) * numChannels);
	memset(&sidechain, 0, sizeof(sidechain));
	memset(&masterFilter, 0, sizeof(masterFilter));
	memset(&mainDelay, 0, sizeof(mainDelay));
	memset(&bypassDelay, 0, sizeof(bypassDelay));
	mainDelay.capacity = kMainDelayCapacity;
	bypassDelay.capacity = kBypassDelayCapacity;
	mainDelay.data = takeStorage<float>(dram, 2 * kMainDelayCapacity);
	bypassDelay.data = takeStorage<float>(dram, 2 * kBypassDelayCapacity);
	memset(mainDelay.data, 0, sizeof(float) * 2 * kMainDelayCapacity);
	memset(bypassDelay.data, 0, sizeof(float) * 2 * kBypassDelayCapacity);
	latencyInitialised = false;
	previousAuto = previousEffective = manualTrim = 0;
	savedTrim = false;
	cachedLengthControl = cachedCurveControl = -10000;
	cachedSampleRate = curveBeta = 0;
	envSamples = 1;
	memset(&masterGain, 0, sizeof(masterGain));
	masterGainInitialised = false;
	setDefaultNames();
	if (!parameterTablesBuilt)
	{
		buildParameters();
		parameterTablesBuilt = true;
	}
	// Append controls after the instantiated channels: all existing IDs stay stable.
	_NT_parameter* instanceParameters = takeStorage<_NT_parameter>(dram, insertLatencyParam() + 1);
	memcpy(instanceParameters, parameterDefs, sizeof(_NT_parameter) * insertSelectParam());
	instanceParameters[insertSelectParam()] = parameterDefs[kMaxParams - 2];
	instanceParameters[insertLatencyParam()] = parameterDefs[kMaxParams - 1];
	parameters = instanceParameters;
	memset(&insertDryDelay, 0, sizeof(insertDryDelay));
	memset(&insertBypassDryDelay, 0, sizeof(insertBypassDryDelay));
	memset(&insertKeyDelay, 0, sizeof(insertKeyDelay));
	for (int i = 0; i < 3; ++i)
	{
		StereoDelay& delay = i == 0 ? insertDryDelay
			: (i == 1 ? insertBypassDryDelay : insertKeyDelay);
		delay.capacity = kInsertDelayCapacity;
		delay.data = takeStorage<float>(dram, 2 * kInsertDelayCapacity);
		memset(delay.data, 0, sizeof(float) * 2 * kInsertDelayCapacity);
	}
	insertReturnDelays = takeStorage<StereoDelay>(dram, kNumRoutes);
	memset(insertReturnDelays, 0, sizeof(StereoDelay) * kNumRoutes);
	for (int route = 0; route < kNumRoutes; ++route)
	{
		StereoDelay& delay = insertReturnDelays[route];
		delay.capacity = kInsertDelayCapacity;
		delay.data = takeStorage<float>(dram, 2 * kInsertDelayCapacity);
		memset(delay.data, 0, sizeof(float) * 2 * kInsertDelayCapacity);
	}
	memset(insertLatencies, 0, sizeof(insertLatencies));
	insertSelected = insertDisplayed = 0;
	insertInitialised = insertRestorePending = insertPublishing = false;
	for (int i = 0; i < kFinalOutputParams; ++i)
		finalOutputParams[i] = finalOutputPageParams[i];
	finalOutputParams[kFinalOutputParams] = insertSelectParam();
	finalOutputParams[kFinalOutputParams + 1] = insertLatencyParam();
	buildPages();
	parameterPages = &pages;
}

inline int clampInt(int value, int minimum, int maximum)
{
	return value < minimum ? minimum : (value > maximum ? maximum : value);
}

inline int channelBase(int channel)
{
	return kNumGlobalParams + channel * kNumChannelParams;
}

inline int channelInsertSlotParam(int insert, int slot)
{
	return (insert == 0 ? kChannelInsert1Slot1 : kChannelInsert2Slot1) + slot;
}

inline int insertParameterToState(int value)
{
	return clampInt(value, 0, kNumInsertStates - 1);
}

void copyText(char* destination, int capacity, const char* source)
{
	if (!source)
		source = "";
	int i = 0;
	for (; i < capacity - 1 && source[i]; ++i)
		destination[i] = source[i];
	destination[i] = 0;
}

bool textEquals(const char* a, const char* b)
{
	if (!a)
		a = "";
	if (!b)
		b = "";
	while (*a && *b)
	{
		if (*a != *b)
			return false;
		++a;
		++b;
	}
	return *a == *b;
}

bool isDefaultChannelName(const WitchboardAlgorithm* self, int channel)
{
	return textEquals(self->channelNames[channel], channelPageNames[channel]);
}

bool isAutoSlotName(const char* name, int insert, int state)
{
	return state != kInsertDry
		&& (!name || !name[0] || textEquals(name, defaultSlotNames[insert][state]));
}

void setParameter(_NT_parameter& parameter, const char* name, int minimum, int maximum,
	int defaultValue, uint8_t unit, char const* const* strings = NULL)
{
	parameter.name = name;
	parameter.min = minimum;
	parameter.max = maximum;
	parameter.def = defaultValue;
	parameter.unit = unit;
	parameter.scaling = 0;
	parameter.enumStrings = strings;
}

void setInput(_NT_parameter& parameter, const char* name)
{
	setParameter(parameter, name, 0, kNT_lastBus, 0, kNT_unitAudioInput);
}

void setOutput(_NT_parameter& parameter, const char* name)
{
	setParameter(parameter, name, 0, kNT_lastBus, 0, kNT_unitAudioOutput);
}

void setWidth(_NT_parameter& parameter, const char* name, int defaultValue)
{
	setParameter(parameter, name, kWidthMono, kWidthStereo, defaultValue,
		kNT_unitEnum, widthStrings);
}

void WitchboardAlgorithm::setDefaultNames()
{
	for (int channel = 0; channel < kMaxChannels; ++channel)
		copyText(channelNames[channel], kHardwareNameLength, channelPageNames[channel]);
	for (int route = 0; route < kNumRoutes; ++route)
		copyText(routeNames[route], kHardwareNameLength, defaultRouteNames[route]);
	for (int fx = 0; fx < kNumFx; ++fx)
		copyText(fxNames[fx], kHardwareNameLength, defaultFxNames[fx]);
	for (int insert = 0; insert < kNumInserts; ++insert)
	{
		copyText(slotNames[insert][kInsertDry], kSlotNameLength,
			defaultSlotNames[insert][kInsertDry]);
		for (int state = 0; state < kNumInsertStates; ++state)
			if (state != kInsertDry)
				copyText(slotNames[insert][state], kSlotNameLength, "");
	}
}

void buildParameters()
{
	setParameter(parameterDefs[kMaxParams - 2], "Insert route", 1, kNumRoutes, 1, kNT_unitNone);
	setParameter(parameterDefs[kMaxParams - 1], "Insert return offset", 0, 200, 0, kNT_unitMs);
	parameterDefs[kMaxParams - 1].scaling = kNT_scaling10;
	setParameter(parameterDefs[kParamFadeMs], "Switch fade", 0, 100, 2, kNT_unitMs);

	for (int route = 0; route < kNumRoutes; ++route)
	{
		setOutput(parameterDefs[routeParam(route, kRouteOutputL)],
			defaultRouteParameterNames[route][kRouteOutputL]);
		setOutput(parameterDefs[routeParam(route, kRouteOutputR)],
			defaultRouteParameterNames[route][kRouteOutputR]);
		setInput(parameterDefs[routeParam(route, kRouteReturnL)],
			defaultRouteParameterNames[route][kRouteReturnL]);
		setInput(parameterDefs[routeParam(route, kRouteReturnR)],
			defaultRouteParameterNames[route][kRouteReturnR]);
		setWidth(parameterDefs[routeParam(route, kRouteSendWidth)],
			defaultRouteParameterNames[route][kRouteSendWidth], kWidthMono);
		setWidth(parameterDefs[routeParam(route, kRouteReturnWidth)],
			defaultRouteParameterNames[route][kRouteReturnWidth], kWidthMono);
	}

	setOutput(parameterDefs[kParamMainL], "Main L");
	setOutput(parameterDefs[kParamMainR], "Main R");
	setOutput(parameterDefs[kParamBypassL], "Bypass L");
	setOutput(parameterDefs[kParamBypassR], "Bypass R");

	for (int fx = 0; fx < kNumFx; ++fx)
	{
		for (int field = 0; field < kNumFxParams; ++field)
			fxSetupPageParams[fx * kNumFxParams + field] = fxParam(fx, field);
		setOutput(parameterDefs[fxParam(fx, 0)], defaultFxParameterNames[fx][0]);
		setOutput(parameterDefs[fxParam(fx, 1)], defaultFxParameterNames[fx][1]);
		setWidth(parameterDefs[fxParam(fx, 2)], defaultFxParameterNames[fx][2], kWidthStereo);
		setInput(parameterDefs[fxParam(fx, 3)], defaultFxParameterNames[fx][3]);
		setInput(parameterDefs[fxParam(fx, 4)], defaultFxParameterNames[fx][4]);
		setWidth(parameterDefs[fxParam(fx, 5)], defaultFxParameterNames[fx][5], kWidthStereo);
		setParameter(parameterDefs[fxParam(fx, 6)], defaultFxParameterNames[fx][6],
			0, 1, kOutputPathMain, kNT_unitEnum, outputPathStrings);
	}

	setParameter(parameterDefs[kParamRepeatProtection], "Repeat protection", 0, 1, 1,
		kNT_unitEnum, offOnStrings);

	setParameter(parameterDefs[kParamSidechainMode], "Sidechain", 0, 1, 0,
		kNT_unitEnum, offOnStrings);
	setInput(parameterDefs[kParamSidechainKeyInput], "SC Trigger Input");
	parameterDefs[kParamSidechainKeyInput].unit = kNT_unitCvInput;
	setParameter(parameterDefs[kParamSidechainDepth], "SC Depth", 0, 100, 69, kNT_unitPercent);
	setParameter(parameterDefs[kParamSidechainLookahead], "SC Lookahead", 0, 100, 60, kNT_unitMs);
	parameterDefs[kParamSidechainLookahead].scaling = kNT_scaling10;
	// Normalized log control, displayed in actual milliseconds by parameterString.
	setParameter(parameterDefs[kParamSidechainEnvLength], "SC Env Length", 0, 1000, 486, kNT_unitHasStrings);
	setParameter(parameterDefs[kParamSidechainCurve], "SC Curve", -100, 100, 20, kNT_unitNone);
	setParameter(parameterDefs[kParamSidechainSmooth], "SC Smooth", 0, 100, 4, kNT_unitPercent);
	setParameter(parameterDefs[kParamBypassOffset], "Bypass Offset", 0, 1000, 0, kNT_unitMs);
	parameterDefs[kParamBypassOffset].scaling = kNT_scaling10;

	setParameter(parameterDefs[kParamMasterFilterEnable], "Filter enable", 0, 1, 0,
		kNT_unitEnum, offOnStrings);
	setParameter(parameterDefs[kParamMasterFilterHpCutoff], "HP limit", 0, 100, 70,
		kNT_unitPercent);
	setParameter(parameterDefs[kParamMasterFilterLpCutoff], "LP limit", 0, 100, 20,
		kNT_unitPercent);
	setParameter(parameterDefs[kParamMasterFilterQ], "Filter Q", 0, 100, 10,
		kNT_unitPercent);
	setParameter(parameterDefs[kParamMasterFilterSweep], "Filter sweep", -100, 100, 0,
		kNT_unitPercent);

	setParameter(parameterDefs[kParamMasterMode], "Master output", 0, 2, 0,
		kNT_unitEnum, masterModeStrings);
	setOutput(parameterDefs[kParamMasterSendL], "Master send L");
	setOutput(parameterDefs[kParamMasterSendR], "Master send R");
	setInput(parameterDefs[kParamMasterReturnL], "Master return L");
	setInput(parameterDefs[kParamMasterReturnR], "Master return R");

	setParameter(parameterDefs[kParamMasterGain], "Master Gain", -12, 6, 0, kNT_unitDb);

	for (int channel = 0; channel < kMaxChannels; ++channel)
	{
		const int base = channelBase(channel);
		setParameter(parameterDefs[base + kChannelEnable],
			channelSuffixes[kChannelEnable], 0, 1, 1, kNT_unitEnum, offOnStrings);
		setInput(parameterDefs[base + kChannelInputL], channelSuffixes[kChannelInputL]);
		setInput(parameterDefs[base + kChannelInputR], channelSuffixes[kChannelInputR]);
		setParameter(parameterDefs[base + kChannelGain],
			channelSuffixes[kChannelGain], -60, 6, 0, kNT_unitDb_minInf);
		setParameter(parameterDefs[base + kChannelInsert1],
			channelSuffixes[kChannelInsert1], 0, kInsertParameterMax, 0,
			kNT_unitHasStrings);
		for (int slot = 0; slot < 3; ++slot)
			setParameter(parameterDefs[base + kChannelInsert1Slot1 + slot],
				channelSuffixes[kChannelInsert1Slot1 + slot], 0, kNumRoutes - 1,
				slot, kNT_unitHasStrings);
		setParameter(parameterDefs[base + kChannelSendSelect],
			// Like the insert selectors, values 3 and 4 both select the last slot.
			channelSuffixes[kChannelSendSelect], 0, kNumFx, 0, kNT_unitHasStrings);
		setParameter(parameterDefs[base + kChannelInsert2],
			channelSuffixes[kChannelInsert2], 0, kInsertParameterMax, 0,
			kNT_unitHasStrings);
		for (int slot = 0; slot < 3; ++slot)
			setParameter(parameterDefs[base + kChannelInsert2Slot1 + slot],
				channelSuffixes[kChannelInsert2Slot1 + slot], 0, kNumRoutes - 1,
				slot, kNT_unitHasStrings);
		setParameter(parameterDefs[base + kChannelOutputPath],
			channelSuffixes[kChannelOutputPath], 0, 1, kOutputPathMain,
			kNT_unitEnum, outputPathStrings);
		setParameter(parameterDefs[base + kChannelSendAmount],
			channelSuffixes[kChannelSendAmount], 0, 100, 0, kNT_unitPercent);
	}
}

void WitchboardAlgorithm::buildPages()
{
	int page = 0;
	pageDefs[page++] = {
		.name = "Global",
		.numParams = kGlobalPageParams,
		.group = 1,
		.unused = { 0, 0 },
		.params = globalPageParams,
	};

	pageDefs[page++] = {
		.name = "Route Setup",
		.numParams = kRouteSetupParams,
		.group = 2,
		.unused = { 0, 0 },
		.params = routeSetupPageParams,
	};

	pageDefs[page++] = {
		.name = "Final Outputs",
		.numParams = kFinalOutputParams + 2,
		.group = 3,
		.unused = { 0, 0 },
		.params = finalOutputParams,
	};

	pageDefs[page++] = {
		.name = "FX Setup",
		.numParams = kFxSetupParams,
		.group = 4,
		.unused = { 0, 0 },
		.params = fxSetupPageParams,
	};

	pageDefs[page++] = {
		.name = "Sidechain/Master",
		.numParams = kMasterPageParams,
		.group = 5,
		.unused = { 0, 0 },
		.params = masterPageParams,
	};

	for (int channel = 0; channel < numChannels; ++channel)
	{
		const int base = channelBase(channel);
		for (int i = 0; i < kNumChannelParams; ++i)
			{
			// Put the shared editor controls together without changing their identities.
			const int field = i == 9 ? kChannelSendAmount : (i > 9 ? i - 1 : i);
			channelPages[channel][i] = base + field;
		}
		pageDefs[page++] = {
			.name = channelNames[channel],
			.numParams = kNumChannelParams,
			.group = static_cast<uint8_t>(6 + channel),
			.unused = { 0, 0 },
			.params = channelPages[channel],
		};
	}

	pages.numPages = page;
	pages.pages = pageDefs;
}

inline const float* inputBus(const float* busFrames, int bus, int numFrames)
{
	return bus > 0 && bus <= kNT_lastBus ? busFrames + (bus - 1) * numFrames : NULL;
}

inline float* outputBus(float* busFrames, int bus, int numFrames)
{
	return bus > 0 && bus <= kNT_lastBus ? busFrames + (bus - 1) * numFrames : NULL;
}

inline float dbGain(int decibels)
{
	return decibels <= -60 ? 0.0f : powf(10.0f, decibels / 20.0f);
}

inline float percentGain(int percent)
{
	return clampInt(percent, 0, 100) * 0.01f;
}

inline float clampFloat(float value, float minimum, float maximum)
{
	return value < minimum ? minimum : (value > maximum ? maximum : value);
}

constexpr float kPi = 3.14159265358979323846f;

float envLengthFromNormalized(float x)
{
	return 50.0f * powf(40.0f, clampFloat(x, 0.0f, 1.0f));
}

int millisecondsToSamples(float ms, float sampleRate)
{
	return static_cast<int>(ms * sampleRate * 0.001f + 0.5f);
}

float curveToBeta(float c)
{
	c = clampFloat(c, -1.0f, 1.0f);
	if (fabsf(c) < 0.001f) return 0.0f;
	float hp = powf((fabsf(c) + 1.0e-20f) * 1.2f, 0.41f) * 0.91f;
	hp = clampFloat(hp, 0.0f, 0.999999f);
	const float fp = hp / (1.0f - hp);
	return c < 0.0f ? -fp : fp;
}

void startSidechainEnvelope(SidechainRuntime& sc, int lengthSamples, float beta)
{
	sc.remaining = lengthSamples < 1 ? 1 : lengthSamples;
	sc.value = 0.0;
	sc.multiplier = 1.0;
	sc.increment = 1.0 / sc.remaining;
	if (beta != 0.0f)
	{
		// expm1(beta/N), without small-argument subtraction or an extra libm import.
		// |z| < 0.031 for the public ranges at all supported sample rates.
		const double z = static_cast<double>(beta) / sc.remaining;
		const double qm1 = z * (1 + z * (0.5 + z * (1.0/6 + z * (1.0/24
			+ z * (1.0/120 + z / 720)))));
		sc.multiplier = 1.0 + qm1;
		sc.increment = qm1 / (static_cast<double>(powf(2.718281828459045f, beta)) - 1.0);
	}
}

float rampSmooth(RampSmoothRuntime& ramp, float input, int samples)
{
	if (samples <= 1)
	{
		ramp.value = ramp.target = input;
		ramp.remaining = 0;
		ramp.samples = samples;
		return input;
	}
	// rampsmooth~ semantics: each changed input starts a new linear ramp.
	// A constant input must finish in N samples, rather than decay exponentially.
	if (input != ramp.target || samples != ramp.samples)
	{
		ramp.target = input;
		ramp.samples = samples;
		ramp.remaining = samples;
		ramp.increment = (input - ramp.value) / samples;
	}
	if (ramp.remaining > 0)
	{
		ramp.value += ramp.increment;
		if (--ramp.remaining == 0) ramp.value = ramp.target;
	}
	return clampFloat(ramp.value, 0.0f, 1.0f);
}

float processSidechain(SidechainRuntime& sc, float key, int lengthSamples,
	float beta, int smoothSamples, float depth)
{
	if (!sc.initialised)
	{
		sc.value = sc.gain = sc.smooth.value = sc.smooth.target = 1.0f;
		sc.initialised = true;
	}
	const bool high = fabsf(key) > 0.1f;
	if (high && !sc.keyHigh)
		startSidechainEnvelope(sc, lengthSamples, beta);
	else if (sc.remaining > 0)
	{
		sc.value += sc.increment;
		sc.increment *= sc.multiplier;
		if (--sc.remaining == 0) sc.value = 1.0;
	}
	sc.keyHigh = high;
	const float envelope = rampSmooth(sc.smooth,
		clampFloat(static_cast<float>(sc.value), 0.0f, 1.0f), smoothSamples);
	sc.gain = 1.0f - clampFloat(depth, 0.0f, 1.0f) * (1.0f - envelope);
	return sc.gain;
}

void setDelay(StereoDelay& delay, int samples, int fadeSamples)
{
	delay.requested = clampInt(samples, 0, delay.capacity - 1);
	delay.fadeSamples = fadeSamples < 1 ? 1 : fadeSamples;
	if (!delay.initialised)
	{
		delay.current = delay.target = delay.requested;
		delay.initialised = true;
	}
}

void processDelay(StereoDelay& delay, float& left, float& right)
{
	delay.data[2 * delay.write] = left;
	delay.data[2 * delay.write + 1] = right;
	// Keep the history ring warm for a later live delay request, but avoid the
	// read/return path when the delay is stably zero.
	if (delay.current == 0 && delay.target == 0 && delay.requested == 0
		&& delay.fadePosition == 0)
	{
		if (++delay.write == delay.capacity) delay.write = 0;
		return;
	}
	// Finish a fade before adopting the latest request. Never reset an audible
	// crossfade mid-flight, even if CV supplies a different delay every block.
	if (delay.fadePosition == 0 && delay.current != delay.requested)
		delay.target = delay.requested;
	int oldRead = delay.write - delay.current;
	if (oldRead < 0) oldRead += delay.capacity;
	left = delay.data[2 * oldRead];
	right = delay.data[2 * oldRead + 1];
	if (delay.current != delay.target)
	{
		int newRead = delay.write - delay.target;
		if (newRead < 0) newRead += delay.capacity;
		const float mix = static_cast<float>(++delay.fadePosition) / delay.fadeSamples;
		left += mix * (delay.data[2 * newRead] - left);
		right += mix * (delay.data[2 * newRead + 1] - right);
		if (delay.fadePosition >= delay.fadeSamples)
		{
			delay.current = delay.target;
			delay.fadePosition = 0;
		}
	}
	if (++delay.write == delay.capacity) delay.write = 0;
}

// Insert routes and the aggregate dry path may stop and restart. A cold ring
// starts at zero delay, then fades to the requested tap after it has history.
void setPrimedDelay(StereoDelay& delay, int samples, int fadeSamples)
{
	if (!delay.initialised)
	{
		delay.valid = delay.write = delay.current = delay.target = 0;
		delay.requested = delay.fadePosition = 0;
		delay.initialised = true;
	}
	setDelay(delay, samples, fadeSamples);
}

void processSharedReturnDelay(StereoDelay& delay, float& left, float& right)
{
	delay.data[2 * delay.write] = left;
	delay.data[2 * delay.write + 1] = right;
	if (delay.valid < delay.capacity) ++delay.valid;
	if (delay.current == 0 && delay.target == 0 && delay.requested == 0
		&& delay.fadePosition == 0)
	{
		if (++delay.write == delay.capacity) delay.write = 0;
		return;
	}
	if (delay.fadePosition == 0 && delay.current != delay.requested
		&& delay.valid > delay.requested)
		delay.target = delay.requested;
	int oldRead = delay.write - delay.current;
	if (oldRead < 0) oldRead += delay.capacity;
	left = delay.valid > delay.current ? delay.data[2 * oldRead] : 0.0f;
	right = delay.valid > delay.current ? delay.data[2 * oldRead + 1] : 0.0f;
	if (delay.current != delay.target)
	{
		int newRead = delay.write - delay.target;
		if (newRead < 0) newRead += delay.capacity;
		const float mix = static_cast<float>(++delay.fadePosition) / delay.fadeSamples;
		left += mix * ((delay.valid > delay.target ? delay.data[2 * newRead] : 0.0f) - left);
		right += mix * ((delay.valid > delay.target ? delay.data[2 * newRead + 1] : 0.0f) - right);
		if (delay.fadePosition >= delay.fadeSamples)
		{
			delay.current = delay.target;
			delay.fadePosition = 0;
		}
	}
	if (++delay.write == delay.capacity) delay.write = 0;
}

int followBypassOffset(WitchboardAlgorithm* self)
{
	const int activeAuto = self->v[kParamSidechainMode]
		? clampInt(self->v[kParamSidechainLookahead], 0, 100) : 0;
	const int publicValue = clampInt(self->v[kParamBypassOffset], 0, 1000);
	// First audio block after loading: saved effective delay is authoritative.
	// Optional trim metadata preserves intent when physical delay was clamped.
	if (!self->latencyInitialised)
	{
		if (!self->savedTrim || clampInt(self->manualTrim + activeAuto, 0, 1000) != publicValue)
			self->manualTrim = publicValue - activeAuto;
		self->savedTrim = false;
	}
	else if (publicValue != self->previousEffective)
		self->manualTrim = publicValue - self->previousAuto;
	const int effective = clampInt(self->manualTrim + activeAuto, 0, 1000);
	// Update state before the API setter, which may synchronously notify us.
	self->previousAuto = activeAuto;
	self->previousEffective = effective;
	self->latencyInitialised = true;
	if (effective != publicValue)
	{
		const int index = NT_algorithmIndex(self);
		if (index >= 0)
			NT_setParameterFromAudio(index, kParamBypassOffset + NT_parameterOffset(), effective);
	}
	return effective;
}

// Small decimal formatter avoids introducing printf into the NT object.
int numberString(char* buffer, unsigned value, int decimals, const char* suffix)
{
	char digits[12];
	int count = 0;
	do { digits[count++] = static_cast<char>('0' + value % 10); value /= 10; }
	while (value || count <= decimals);
	int length = 0;
	while (count)
	{
		buffer[length++] = digits[--count];
		if (decimals && count == decimals) buffer[length++] = '.';
	}
	while (*suffix) buffer[length++] = *suffix++;
	buffer[length] = 0;
	return length;
}

struct SvfCoefficients
{
	float a1;
	float a2;
	float a3;
	float k;
	float m0;
	float m1;
	float m2;
};

float hpCutoffPercentToHz(int cutoffPercent, float sampleRate)
{
	const float percent = clampFloat(cutoffPercent * 0.01f, 0.0f, 1.0f);
	const float frequency = 20.0f * powf(1000.0f, percent);
	const float maximum = sampleRate * 0.45f;
	return clampFloat(frequency, 20.0f, maximum);
}

float filterQFromPercent(int qPercent)
{
	return 0.70710678f + clampFloat(qPercent * 0.01f, 0.0f, 1.0f) * 11.29289322f;
}

float filterTan(float radians)
{
	radians = clampFloat(radians, 0.0f, kPi * 0.45f);
	const float x2 = radians * radians;
	const float pi2 = kPi * kPi;
	const float denominator = pi2 - 4.0f * x2;
	if (denominator <= 0.001f)
		return 64.0f;
	return radians * (pi2 - x2) / denominator;
}

SvfCoefficients makeFilterCoefficients(int mode, float frequency, float q)
{
	const float configuredSampleRate = NT_globals.sampleRate;
	const float sampleRate = configuredSampleRate > 0.0f ? configuredSampleRate : 48000.0f;
	const float g = filterTan(kPi * frequency / sampleRate);
	const float k = 1.0f / q;
	SvfCoefficients coeffs = {};

	coeffs.a1 = 1.0f / (1.0f + g * (g + k));
	coeffs.a2 = g * coeffs.a1;
	coeffs.a3 = g * coeffs.a2;
	coeffs.k = k;

	switch (mode)
	{
	case kFilterHighpass:
		coeffs.m0 = 1.0f;
		coeffs.m1 = -k;
		coeffs.m2 = -1.0f;
		break;
	default:
		coeffs.m0 = 0.0f;
		coeffs.m1 = 0.0f;
		coeffs.m2 = 1.0f;
		break;
	}
	return coeffs;
}

float processFilterSample(SvfRuntime& state, float input,
	const SvfCoefficients& coeffs)
{
	float v0 = input;
	const float v3 = v0 - state.ic2eq;
	const float v1 = coeffs.a1 * state.ic1eq + coeffs.a2 * v3;
	const float v2 = state.ic2eq + coeffs.a2 * state.ic1eq + coeffs.a3 * v3;
	state.ic1eq = 2.0f * v1 - state.ic1eq;
	state.ic2eq = 2.0f * v2 - state.ic2eq;
	return coeffs.m0 * v0 + coeffs.m1 * v1 + coeffs.m2 * v2;
}

void processMasterFilter(MasterFilterRuntime& filter, float& left, float& right,
	bool lpEnabled, bool hpEnabled,
	const SvfCoefficients& lpCoeffs, const SvfCoefficients& hpCoeffs)
{
	if (lpEnabled)
	{
		left = processFilterSample(filter.lpLeft, left, lpCoeffs);
		right = processFilterSample(filter.lpRight, right, lpCoeffs);
	}
	else if (hpEnabled)
	{
		left = processFilterSample(filter.hpLeft, left, hpCoeffs);
		right = processFilterSample(filter.hpRight, right, hpCoeffs);
	}
}

void resetMasterFilterAudioState(MasterFilterRuntime& filter)
{
	memset(&filter.hpLeft, 0, sizeof(filter.hpLeft));
	memset(&filter.hpRight, 0, sizeof(filter.hpRight));
	memset(&filter.lpLeft, 0, sizeof(filter.lpLeft));
	memset(&filter.lpRight, 0, sizeof(filter.lpRight));
}

void initialiseSmooth(SmoothedValue& smooth, int parameterValue, float value)
{
	smooth.parameterValue = parameterValue;
	smooth.samplesRemaining = 0;
	smooth.target = value;
	smooth.value = value;
	smooth.increment = 0.0f;
}

void beginSmooth(SmoothedValue& smooth, int parameterValue, float target, int fadeSamples)
{
	smooth.parameterValue = parameterValue;
	smooth.target = target;
	if (fadeSamples <= 0)
	{
		smooth.samplesRemaining = 0;
		smooth.value = target;
		smooth.increment = 0.0f;
		return;
	}
	smooth.samplesRemaining = fadeSamples;
	smooth.increment = (target - smooth.value) / fadeSamples;
}

void advanceSmooth(SmoothedValue& smooth)
{
	if (smooth.samplesRemaining <= 0)
		return;
	smooth.value += smooth.increment;
	if (--smooth.samplesRemaining == 0)
		smooth.value = smooth.target;
}

void initialiseChannel(ChannelRuntime& runtime, int insert1, int insert2,
	int gainDb, const int16_t* levels)
{
	runtime.initialised = true;
	const int inserts[kNumInserts] = { insert1, insert2 };
	for (int insert = 0; insert < kNumInserts; ++insert)
	{
		runtime.insertState[insert] = inserts[insert];
		runtime.insertSamplesRemaining[insert] = 0;
		for (int state = 0; state < kNumInsertStates; ++state)
		{
			runtime.insertGain[insert][state] = state == inserts[insert] ? 1.0f : 0.0f;
			runtime.insertIncrement[insert][state] = 0.0f;
		}
	}
	initialiseSmooth(runtime.gain, gainDb, dbGain(gainDb));
	for (int fx = 0; fx < kNumFx; ++fx)
		initialiseSmooth(runtime.fxMix[fx], levels[fx], percentGain(levels[fx]));
}

void beginInsertFade(ChannelRuntime& runtime, int insert, int state, int fadeSamples)
{
	runtime.insertState[insert] = state;
	if (fadeSamples <= 0)
	{
		runtime.insertSamplesRemaining[insert] = 0;
		for (int i = 0; i < kNumInsertStates; ++i)
		{
			runtime.insertGain[insert][i] = i == state ? 1.0f : 0.0f;
			runtime.insertIncrement[insert][i] = 0.0f;
		}
		return;
	}

	runtime.insertSamplesRemaining[insert] = fadeSamples;
	for (int i = 0; i < kNumInsertStates; ++i)
	{
		const float target = i == state ? 1.0f : 0.0f;
		runtime.insertIncrement[insert][i] =
			(target - runtime.insertGain[insert][i]) / fadeSamples;
	}
}

void advanceChannel(ChannelRuntime& runtime)
{
	for (int insert = 0; insert < kNumInserts; ++insert)
	{
		if (runtime.insertSamplesRemaining[insert] <= 0)
			continue;
		for (int state = 0; state < kNumInsertStates; ++state)
			runtime.insertGain[insert][state] += runtime.insertIncrement[insert][state];
		if (--runtime.insertSamplesRemaining[insert] == 0)
		{
			for (int state = 0; state < kNumInsertStates; ++state)
				runtime.insertGain[insert][state] =
					state == runtime.insertState[insert] ? 1.0f : 0.0f;
		}
	}
	advanceSmooth(runtime.gain);
	for (int fx = 0; fx < kNumFx; ++fx)
		advanceSmooth(runtime.fxMix[fx]);
}

bool channelMoving(const ChannelRuntime& rt)
{
	if (rt.insertSamplesRemaining[0] > 0 || rt.insertSamplesRemaining[1] > 0
		|| rt.gain.samplesRemaining > 0)
		return true;
	for (int fx = 0; fx < kNumFx; ++fx)
		if (rt.fxMix[fx].samplesRemaining > 0) return true;
	return false;
}

void publishSendAmount(WitchboardAlgorithm* self, int channel)
{
	SendState& state = self->sends[channel];
	state.displayed = state.levels[state.selected];
	const int parameter = channelBase(channel) + kChannelSendAmount;
	if (self->v[parameter] == state.displayed) return;
	const int index = NT_algorithmIndex(self);
	if (index < 0) return;
	state.publishing = true;
	NT_setParameterFromAudio(index, parameter + NT_parameterOffset(), state.displayed);
	state.publishing = false;
}

void writeSendLevel(WitchboardAlgorithm* self, int channel, int fx, int value,
	int midiSource = -1)
{
	SendState& state = self->sends[channel];
	value = clampInt(value, 0, 100);
	if (state.levels[fx] != value)
	{
		state.levels[fx] = value;
		// Other pickup faders must catch a level changed by another controller.
		if (midiSource != fx) state.midi[fx].caught = false;
		if (fx == state.selected && midiSource != kNumFx)
			state.midi[kNumFx].caught = false;
	}
	if (fx == state.selected) publishSendAmount(self, channel);
}

void syncSendEditor(WitchboardAlgorithm* self, int channel)
{
	SendState& state = self->sends[channel];
	if (state.publishing) return;
	const int base = channelBase(channel);
	const int selected = clampInt(self->v[base + kChannelSendSelect], 0, kNumFx - 1);
	const int amount = clampInt(self->v[base + kChannelSendAmount], 0, 100);
	if (!state.initialised || state.restorePending)
	{
		state.selected = selected;
		if (!state.restorePending) state.levels[selected] = amount;
		state.initialised = true;
		state.restorePending = false;
		publishSendAmount(self, channel);
	}
	else if (selected != state.selected)
	{
		state.selected = selected;
		state.midi[kNumFx].caught = false;
		publishSendAmount(self, channel);
	}
	else if (amount != state.displayed)
		writeSendLevel(self, channel, selected, amount);
}

// CC assignments live in preset metadata, without using NT parameter slots.
// Channel 0 disables an assignment; channels 1..16 and CC 0..119 are supported.
void midiMessage(_NT_algorithm* algorithm, uint8_t status, uint8_t cc, uint8_t value)
{
	if ((status & 0xf0) != 0xb0 || cc >= 120 || value > 127) return;
	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	for (int channel = 0; channel < self->numChannels; ++channel)
	{
		syncSendEditor(self, channel);
		SendState& state = self->sends[channel];
		for (int target = 0; target <= kNumFx; ++target)
		{
			SendMidiMapping& mapping = state.midi[target];
			if (mapping.channel != (status & 15) + 1 || mapping.cc != cc) continue;
			const int fx = target == kNumFx ? state.selected : target;
			const int amount = mapping.minimum +
				(mapping.maximum - mapping.minimum) * value / 127;
			const int previous = mapping.previous;
			mapping.previous = amount;
			const int current = state.levels[fx];
			if (mapping.pickup && !mapping.caught)
			{
				mapping.caught = (amount >= current - 1 && amount <= current + 1) || (previous >= 0 &&
					((previous <= current && amount >= current) ||
					 (previous >= current && amount <= current)));
				if (!mapping.caught) continue;
			}
			writeSendLevel(self, channel, fx, amount, target);
		}
	}
}

// The selected route edits one stored roundtrip value. Preset metadata wins
// over the old channel-offset parameter values on restore.
void syncInsertLatencyEditor(WitchboardAlgorithm* self)
{
	if (self->insertPublishing) return;
	const int selected = clampInt(self->v[self->insertSelectParam()] - 1, 0, kNumRoutes - 1);
	const int amount = clampInt(self->v[self->insertLatencyParam()], 0, 200);
	if (!self->insertInitialised || self->insertRestorePending)
	{
		if (!self->insertRestorePending) self->insertLatencies[selected] = amount;
		self->insertInitialised = true;
		self->insertRestorePending = false;
	}
	else if (selected == self->insertSelected && amount != self->insertDisplayed)
		self->insertLatencies[selected] = amount;
	self->insertSelected = selected;
	self->insertDisplayed = self->insertLatencies[selected];
	if (self->v[self->insertLatencyParam()] == self->insertDisplayed) return;
	const int index = NT_algorithmIndex(self);
	if (index < 0) return;
	self->insertPublishing = true;
	NT_setParameterFromAudio(index, self->insertLatencyParam() + NT_parameterOffset(), self->insertDisplayed);
	self->insertPublishing = false;
}

// NT parameter changes can arrive outside the audio step. Invalidate only the
// affected insert cache so the existing step() code observes the current self->v
// value and starts the normal insert fade on the next audio block.
void parameterChanged(_NT_algorithm* algorithm, int parameter)
{
	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	if (!self)
		return;

	if (parameter == self->insertSelectParam() || parameter == self->insertLatencyParam())
	{
		syncInsertLatencyEditor(self);
		return;
	}
	if (parameter < kNumGlobalParams)
		return;

	const int relative = parameter - kNumGlobalParams;
	const int channel = relative / kNumChannelParams;
	const int field = relative % kNumChannelParams;
	if (channel < 0 || channel >= self->numChannels)
		return;

	if (field == kChannelSendSelect || field == kChannelSendAmount)
	{
		syncSendEditor(self, channel);
		return;
	}

	int insert = -1;
	if (field == kChannelInsert1)
		insert = 0;
	else if (field == kChannelInsert2)
		insert = 1;
	else
		return;

	ChannelRuntime& rt = self->runtime[channel];

	// Valid insert states are 0..3, so -1 is guaranteed to differ from the
	// newly selected state. step() will call beginInsertFade() using self->v.
	rt.insertState[insert] = -1;
}

OutputPair makeOutputPair(float* busFrames, int numFrames, int leftBus, int rightBus)
{
	OutputPair pair;
	pair.left = outputBus(busFrames, leftBus, numFrames);
	pair.right = outputBus(busFrames, rightBus, numFrames);
	pair.singleFrame = false;
	return pair;
}

inline void addSignal(const OutputPair& pair, int frame, float left, float right,
	bool stereo, float gain)
{
	if (gain <= 0.0f || !pair.left)
		return;
	const int index = pair.singleFrame ? 0 : frame;
	if (pair.right)
	{
		pair.left[index] += left * gain;
		pair.right[index] += (stereo ? right : left) * gain;
	}
	else
	{
		const float mono = stereo ? 0.5f * (left + right) : left;
		pair.left[index] += mono * gain;
	}
}

struct CrossfadeGains
{
	float dry;
	float wet;
};

inline CrossfadeGains shapedCrossfade(float mix)
{
	mix = mix < 0.0f ? 0.0f : (mix > 1.0f ? 1.0f : mix);
	if (mix <= 0.5f)
		return { 1.0f, mix * 2.0f };
	return { (1.0f - mix) * 2.0f, 1.0f };
}

inline void processPath(OutputPair* outputs,
	const float* const* returnLeft, const float* const* returnRight,
	const bool* returnStereo, int frame, float left, float right, bool stereo,
	int route1, int route2, bool repeatProtection,
	float pathGain, float channelGain, const bool* deferredRoutes,
	bool* sharedUsed, float& collectedLeft, float& collectedRight)
{
	if (pathGain <= 0.0f)
		return;

	left *= channelGain;
	right *= channelGain;
	if (repeatProtection && route1 >= 0 && route2 == route1)
		route2 = -1;
	const int finalRoute = route2 >= 0 ? route2 : route1;
	const bool shared = finalRoute >= 0 && deferredRoutes[finalRoute];

	float intermediateLeft = left;
	float intermediateRight = right;
	bool intermediateStereo = stereo;
	if (route1 >= 0)
	{
		addSignal(outputs[kRouteOutputBase + route1], frame, left, right, stereo, pathGain);
		if (shared && route2 < 0)
		{
			sharedUsed[finalRoute] = true;
			return;
		}
		intermediateLeft = returnLeft[route1] ? returnLeft[route1][frame] : 0.0f;
		intermediateRight = returnRight[route1] ? returnRight[route1][frame] : intermediateLeft;
		intermediateStereo = returnStereo[route1];
	}
	float finalLeft = intermediateLeft;
	float finalRight = intermediateRight;
	bool finalStereo = intermediateStereo;
	if (route2 >= 0)
	{
		addSignal(outputs[kRouteOutputBase + route2], frame, intermediateLeft, intermediateRight,
			intermediateStereo, pathGain);
		if (shared)
		{
			sharedUsed[finalRoute] = true;
			return;
		}
		finalLeft = returnLeft[route2] ? returnLeft[route2][frame] : 0.0f;
		finalRight = returnRight[route2] ? returnRight[route2][frame] : finalLeft;
		finalStereo = returnStereo[route2];
	}

	collectedLeft += finalLeft * pathGain;
	collectedRight += (finalStereo ? finalRight : finalLeft) * pathGain;
}

// Use the original path when no return route is shared.
inline void processPath(OutputPair* outputs,
	const float* const* returnLeft, const float* const* returnRight,
	const bool* returnStereo, int frame, float left, float right, bool stereo,
	int route1, int route2, bool repeatProtection,
	float pathGain, float channelGain, float& collectedLeft, float& collectedRight)
{
	if (pathGain <= 0.0f)
		return;

	left *= channelGain;
	right *= channelGain;

	float intermediateLeft = left;
	float intermediateRight = right;
	bool intermediateStereo = stereo;
	if (route1 >= 0)
	{
		addSignal(outputs[kRouteOutputBase + route1], frame, left, right, stereo, pathGain);
		intermediateLeft = returnLeft[route1] ? returnLeft[route1][frame] : 0.0f;
		intermediateRight = returnRight[route1] ? returnRight[route1][frame] : intermediateLeft;
		intermediateStereo = returnStereo[route1];
	}
	if (repeatProtection && route1 >= 0 && route2 == route1)
		route2 = -1;

	float finalLeft = intermediateLeft;
	float finalRight = intermediateRight;
	bool finalStereo = intermediateStereo;
	if (route2 >= 0)
	{
		addSignal(outputs[kRouteOutputBase + route2], frame, intermediateLeft, intermediateRight,
			intermediateStereo, pathGain);
		finalLeft = returnLeft[route2] ? returnLeft[route2][frame] : 0.0f;
		finalRight = returnRight[route2] ? returnRight[route2][frame] : finalLeft;
		finalStereo = returnStereo[route2];
	}

	collectedLeft += finalLeft * pathGain;
	collectedRight += (finalStereo ? finalRight : finalLeft) * pathGain;
}

inline void mixChannelSignal(OutputPair* outputs, int frame, int outputIndex,
	float left, float right, const CrossfadeGains* fxGains)
{
	float dryMix = 1.0f;
	for (int fx = 0; fx < kNumFx; ++fx)
	{
		dryMix *= fxGains[fx].dry;
		addSignal(outputs[2 + fx], frame, left, right, true, fxGains[fx].wet);
	}
	addSignal(outputs[outputIndex], frame, left, right, true, dryMix);
}

struct ChannelBlockState
{
	int base;
	const float* left;
	const float* right;
	bool enabled;
	bool stereo;
	bool moving;
	int outputIndex;
	int8_t routes[kNumInserts][kNumInsertStates];
	CrossfadeGains fxGains[kNumFx];
	uint8_t finalRouteMask;
	bool deferred;
};

inline int finalInsertRoute(const ChannelBlockState& state, int first, int second,
	bool repeatProtection)
{
	const int route1 = state.routes[0][first];
	const int route2 = state.routes[1][second];
	return route2 >= 0 && (!repeatProtection || route2 != route1) ? route2 : route1;
}

uint8_t channelFinalRouteMask(const ChannelBlockState& state, const ChannelRuntime& rt,
	bool repeatProtection)
{
	uint8_t mask = 0;
	for (int first = 0; first < kNumInsertStates; ++first)
	{
		if (first != rt.insertState[0] && rt.insertGain[0][first] <= 0.0f) continue;
		for (int second = 0; second < kNumInsertStates; ++second)
		{
			if (second != rt.insertState[1] && rt.insertGain[1][second] <= 0.0f) continue;
			const int route = finalInsertRoute(state, first, second, repeatProtection);
			if (route >= 0) mask |= static_cast<uint8_t>(1u << route);
		}
	}
	return mask;
}

int selectedRoute(const WitchboardAlgorithm* self, int channel, int insert, int state)
{
	if (state == kInsertDry)
		return -1;
	const int base = channelBase(channel);
	const int slot = clampInt(state, 1, 3) - 1;
	const int param = base + channelInsertSlotParam(insert, slot);
	return clampInt(self->v[param], 0, kNumRoutes - 1);
}

_NT_DRAM_SECTION
void calculateRequirements(_NT_algorithmRequirements& requirements, const int32_t* specs)
{
	const int channels = clampInt(specs[0], 1, kMaxChannels);
	requirements.numParameters = kNumGlobalParams + channels * kNumChannelParams + 2;
	requirements.sram = static_cast<uint32_t>(requiredSram(channels));
	requirements.dram = static_cast<uint32_t>(requiredDram(channels));
	requirements.dtc = 0;
	requirements.itc = 0;
}

_NT_DRAM_SECTION
_NT_algorithm* constructWitchboard(const _NT_algorithmMemoryPtrs& pointers,
	const _NT_algorithmRequirements&, const int32_t* specs)
{
	const int channels = clampInt(specs[0], 1, kMaxChannels);
	return new (pointers.sram) WitchboardAlgorithm(channels, pointers.dram);
}

void step(_NT_algorithm* algorithm, float* busFrames, int numFramesBy4)
{
	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	const int numFrames = numFramesBy4 * 4;
	const int fadeSamples = self->v[kParamFadeMs]
		* static_cast<int>(NT_globals.sampleRate) / 1000;

	OutputPair outputs[kNumOutputPairs];
	const OutputPair mainOutput = makeOutputPair(busFrames, numFrames, self->v[kParamMainL],
		self->v[kParamMainR]);
	const OutputPair bypassOutput = makeOutputPair(busFrames, numFrames, self->v[kParamBypassL],
		self->v[kParamBypassR]);
	outputs[0] = mainOutput;
	outputs[1] = bypassOutput;
	for (int fx = 0; fx < kNumFx; ++fx)
		outputs[2 + fx] = makeOutputPair(busFrames, numFrames, self->v[fxParam(fx, 0)],
			self->v[fxParam(fx, 2)] == kWidthStereo ? self->v[fxParam(fx, 1)] : 0);
	for (int route = 0; route < kNumRoutes; ++route)
	{
		outputs[kRouteOutputBase + route] = makeOutputPair(busFrames, numFrames,
			self->v[routeParam(route, kRouteOutputL)],
			self->v[routeParam(route, kRouteSendWidth)] == kWidthStereo
				? self->v[routeParam(route, kRouteOutputR)] : 0);
	}

	const float* returnLeft[kNumRoutes];
	const float* returnRight[kNumRoutes];
	bool returnStereo[kNumRoutes];
	for (int route = 0; route < kNumRoutes; ++route)
	{
		returnLeft[route] = inputBus(busFrames,
			self->v[routeParam(route, kRouteReturnL)], numFrames);
		returnRight[route] = self->v[routeParam(route, kRouteReturnWidth)] == kWidthStereo
			? inputBus(busFrames, self->v[routeParam(route, kRouteReturnR)], numFrames) : NULL;
		returnStereo[route] = returnRight[route] != NULL;
	}

	const bool sidechainEnabled = self->v[kParamSidechainMode] != 0;
	const int masterMode = clampInt(self->v[kParamMasterMode], kMasterSplit, kMasterInsert);
	const bool filterOn = self->v[kParamMasterFilterEnable] != 0;
	const int rawSweep = clampInt(self->v[kParamMasterFilterSweep], -100, 100);
	const int filterSweep = filterOn ? rawSweep : 0;
	const int hpLimit = clampInt(self->v[kParamMasterFilterHpCutoff], 0, 100);
	const int lpLimit = clampInt(self->v[kParamMasterFilterLpCutoff], 0, 100);
	const bool lpSweep = filterSweep < 0;
	const bool hpSweep = filterSweep > 0;
	int lpCutoff = 0;
	if (lpSweep)
		lpCutoff = 100 - (100 - lpLimit) * -filterSweep / 100;
	int hpCutoff = 0;
	if (hpSweep)
		hpCutoff = hpLimit * filterSweep / 100;
	const bool lpEnabled = filterOn && lpSweep;
	const bool hpEnabled = filterOn && hpSweep;
	const bool anyFilterEnabled = lpEnabled || hpEnabled;
	const bool repeatProtection = self->v[kParamRepeatProtection] != 0;
	const float gainSampleRate = NT_globals.sampleRate > 0 ? NT_globals.sampleRate : 48000;
	const int masterGainDb = clampInt(self->v[kParamMasterGain], -12, 6);
	if (!self->masterGainInitialised)
	{
		initialiseSmooth(self->masterGain, masterGainDb, dbGain(masterGainDb));
		self->masterGainInitialised = true;
	}
	else if (self->masterGain.parameterValue != masterGainDb)
		beginSmooth(self->masterGain, masterGainDb, dbGain(masterGainDb),
			static_cast<int>(gainSampleRate * 0.01f));
	if (!sidechainEnabled)
		memset(&self->sidechain, 0, sizeof(self->sidechain));
	if (!anyFilterEnabled)
		resetMasterFilterAudioState(self->masterFilter);
	const float configuredSampleRate = NT_globals.sampleRate;
	const float sampleRate = configuredSampleRate > 0.0f ? configuredSampleRate : 48000.0f;
	const float filterQ = filterQFromPercent(self->v[kParamMasterFilterQ]);
	const SvfCoefficients lpCoeffs = lpEnabled
		? makeFilterCoefficients(kFilterLowpass,
			hpCutoffPercentToHz(lpCutoff, sampleRate),
			filterQ)
		: SvfCoefficients();
	const SvfCoefficients hpCoeffs = hpEnabled
		? makeFilterCoefficients(kFilterHighpass,
			hpCutoffPercentToHz(hpCutoff, sampleRate),
			filterQ)
		: SvfCoefficients();
	const float* sidechainKey = inputBus(busFrames, self->v[kParamSidechainKeyInput], numFrames);
	const int effectiveBypass = followBypassOffset(self);
	const int fadeDelaySamples = millisecondsToSamples(5.0f, sampleRate);
	setDelay(self->mainDelay, sidechainEnabled
		? millisecondsToSamples(self->v[kParamSidechainLookahead] * 0.1f, sampleRate) : 0, fadeDelaySamples);
	// Cache control coefficients instead of recomputing them every audio block.
	if (self->cachedSampleRate != sampleRate
		|| self->cachedLengthControl != self->v[kParamSidechainEnvLength]
		|| self->cachedCurveControl != self->v[kParamSidechainCurve])
	{
		self->cachedSampleRate = sampleRate;
		self->cachedLengthControl = self->v[kParamSidechainEnvLength];
		self->cachedCurveControl = self->v[kParamSidechainCurve];
		self->envSamples = millisecondsToSamples(envLengthFromNormalized(
			self->cachedLengthControl * 0.001f), sampleRate);
		self->curveBeta = curveToBeta(self->cachedCurveControl * 0.01f);
	}
	const int envSamples = self->envSamples;
	const float beta = self->curveBeta;
	const int smoothSamples = millisecondsToSamples(self->v[kParamSidechainSmooth] * 2.0f, sampleRate);
	const float depth = self->v[kParamSidechainDepth] * 0.01f;
	const OutputPair masterSend = makeOutputPair(busFrames, numFrames,
		self->v[kParamMasterSendL], self->v[kParamMasterSendR]);
	const float* masterReturnL = inputBus(busFrames, self->v[kParamMasterReturnL], numFrames);
	const float* masterReturnR = inputBus(busFrames, self->v[kParamMasterReturnR], numFrames);

	const float* fxLeft[kNumFx];
	const float* fxRight[kNumFx];
	bool fxStereo[kNumFx];
	int fxOutput[kNumFx];
	for (int fx = 0; fx < kNumFx; ++fx)
	{
		fxLeft[fx] = inputBus(busFrames, self->v[fxParam(fx, 3)], numFrames);
		fxRight[fx] = self->v[fxParam(fx, 5)] == kWidthStereo
			? inputBus(busFrames, self->v[fxParam(fx, 4)], numFrames) : NULL;
		fxStereo[fx] = fxRight[fx] != NULL;
		fxOutput[fx] = self->v[fxParam(fx, 6)] == kOutputPathBypass ? 1 : 0;
	}

	syncInsertLatencyEditor(self);

	ChannelBlockState channelState[kMaxChannels];
	for (int channel = 0; channel < self->numChannels; ++channel)
	{
		const int base = channelBase(channel);
		ChannelBlockState& state = channelState[channel];
		state.base = base;
		state.left = inputBus(busFrames, self->v[base + kChannelInputL], numFrames);
		state.right = inputBus(busFrames, self->v[base + kChannelInputR], numFrames);
		state.enabled = self->v[base + kChannelEnable] != 0 && state.left != NULL;
		state.stereo = state.right != NULL;
		state.outputIndex = self->v[base + kChannelOutputPath] == kOutputPathBypass ? 1 : 0;

		const int insert1 = insertParameterToState(self->v[base + kChannelInsert1]);
		const int insert2 = insertParameterToState(self->v[base + kChannelInsert2]);
		const int gainDb = self->v[base + kChannelGain];
		syncSendEditor(self, channel);
		const int16_t* levels = self->sends[channel].levels;

		ChannelRuntime& rt = self->runtime[channel];
		if (!rt.initialised)
			initialiseChannel(rt, insert1, insert2, gainDb, levels);
		else
		{
			if (rt.insertState[0] != insert1)
				beginInsertFade(rt, 0, insert1, fadeSamples);
			if (rt.insertState[1] != insert2)
				beginInsertFade(rt, 1, insert2, fadeSamples);
			if (rt.gain.parameterValue != gainDb)
				beginSmooth(rt.gain, gainDb, dbGain(gainDb), fadeSamples);
			for (int fx = 0; fx < kNumFx; ++fx)
				if (rt.fxMix[fx].parameterValue != levels[fx])
					beginSmooth(rt.fxMix[fx], levels[fx], percentGain(levels[fx]), fadeSamples);
		}
		state.moving = channelMoving(rt);
		for (int fx = 0; fx < kNumFx; ++fx)
			state.fxGains[fx] = shapedCrossfade(rt.fxMix[fx].value);
		for (int insert = 0; insert < kNumInserts; ++insert)
		{
			for (int routeState = 0; routeState < kNumInsertStates; ++routeState)
				state.routes[insert][routeState] = static_cast<int8_t>(
					selectedRoute(self, channel, insert, routeState));
		}
	}
	uint8_t routeUsers[kNumRoutes] = {};
	for (int channel = 0; channel < self->numChannels; ++channel)
	{
		ChannelBlockState& state = channelState[channel];
		state.finalRouteMask = 0;
		state.deferred = false;
		if (!state.enabled) continue;
		const uint8_t mask = channelFinalRouteMask(state,
			self->runtime[channel], repeatProtection);
		state.finalRouteMask = mask;
		for (int route = 0; route < kNumRoutes; ++route)
			if (mask & (1u << route))
				++routeUsers[route];
	}
	int maxInsertLatency = 0;
	bool deferredRoutes[kNumRoutes] = {};
	bool anyDeferred = false;
	for (int route = 0; route < kNumRoutes; ++route)
	{
		if (routeUsers[route] && self->insertLatencies[route] > maxInsertLatency)
			maxInsertLatency = self->insertLatencies[route];
		deferredRoutes[route] = routeUsers[route] > 1
			|| (routeUsers[route] && self->insertLatencies[route] > 0);
		if (deferredRoutes[route])
			anyDeferred = true;
		else
		{
			StereoDelay& delay = self->insertReturnDelays[route];
			delay.initialised = false;
			delay.valid = delay.write = delay.current = delay.target = 0;
			delay.requested = delay.fadePosition = 0;
		}
	}
	uint8_t deferredMask = 0;
	for (int route = 0; route < kNumRoutes; ++route)
		if (deferredRoutes[route]) deferredMask |= static_cast<uint8_t>(1u << route);
	for (int channel = 0; channel < self->numChannels; ++channel)
		channelState[channel].deferred = (channelState[channel].finalRouteMask & deferredMask) != 0;
	const int maxInsertSamples = millisecondsToSamples(maxInsertLatency * 0.1f, sampleRate);
	if (maxInsertSamples > 0 || self->insertDryDelay.current != 0
		|| self->insertDryDelay.fadePosition != 0)
		setPrimedDelay(self->insertDryDelay, maxInsertSamples, fadeDelaySamples);
	else
		self->insertDryDelay.initialised = false;
	if (maxInsertSamples > 0 || self->insertBypassDryDelay.current != 0
		|| self->insertBypassDryDelay.fadePosition != 0)
		setPrimedDelay(self->insertBypassDryDelay, maxInsertSamples, fadeDelaySamples);
	else
		self->insertBypassDryDelay.initialised = false;
	if (sidechainEnabled)
	{
		if (maxInsertSamples > 0 || self->insertKeyDelay.current != 0
			|| self->insertKeyDelay.fadePosition != 0)
			setPrimedDelay(self->insertKeyDelay, maxInsertSamples, fadeDelaySamples);
	}
	else
		self->insertKeyDelay.initialised = false;
	for (int route = 0; route < kNumRoutes; ++route)
		if (deferredRoutes[route])
		{
			const int requested = maxInsertSamples
				- millisecondsToSamples(self->insertLatencies[route] * 0.1f, sampleRate);
			StereoDelay& delay = self->insertReturnDelays[route];
			if (requested > 0 || delay.current != 0 || delay.fadePosition != 0)
				setPrimedDelay(delay, requested, fadeDelaySamples);
			else
				delay.initialised = false;
		}
	setDelay(self->bypassDelay,
		millisecondsToSamples(effectiveBypass * 0.1f, sampleRate), fadeDelaySamples);

	for (int frame = 0; frame < numFrames; ++frame)
	{
		bool sharedUsed[kNumRoutes] = {};
		float sharedSend[kNumRoutes][kNumFx] = {};
		int sharedOutput[kNumRoutes] = {};
		float mainLeft = 0.0f;
		float mainRight = 0.0f;
		float bypassLeft = 0.0f;
		float bypassRight = 0.0f;
		// Collect both branches even at zero delay to keep ring history warm.
		outputs[0] = { &mainLeft, &mainRight, true };
		outputs[1] = { &bypassLeft, &bypassRight, true };

		for (int fx = 0; fx < kNumFx; ++fx)
		{
			if (!fxLeft[fx])
				continue;
			const float left = fxLeft[fx][frame];
			const float right = fxRight[fx] ? fxRight[fx][frame] : left;
			addSignal(outputs[fxOutput[fx]], frame, left, right, fxStereo[fx], 1.0f);
		}

		for (int channel = 0; channel < self->numChannels; ++channel)
		{
			ChannelBlockState& state = channelState[channel];
			ChannelRuntime& rt = self->runtime[channel];
			if (state.moving)
			{
				advanceChannel(rt);
				for (int fx = 0; fx < kNumFx; ++fx)
					state.fxGains[fx] = shapedCrossfade(rt.fxMix[fx].value);
				state.moving = channelMoving(rt);
			}
			if (!state.enabled)
				continue;
			float channelLeft = 0, channelRight = 0;
			bool channelSharedUsed[kNumRoutes] = {};

			const float left = state.left[frame];
			const float right = state.right ? state.right[frame] : left;

			if (rt.insertSamplesRemaining[0] == 0 && rt.insertSamplesRemaining[1] == 0)
			{
				if (state.deferred)
					processPath(outputs, returnLeft, returnRight, returnStereo,
						frame, left, right, state.stereo,
						state.routes[0][rt.insertState[0]],
						state.routes[1][rt.insertState[1]],
						repeatProtection, 1.0f, rt.gain.value, deferredRoutes,
						channelSharedUsed, channelLeft, channelRight);
				else
					processPath(outputs, returnLeft, returnRight, returnStereo,
						frame, left, right, state.stereo,
						state.routes[0][rt.insertState[0]],
						state.routes[1][rt.insertState[1]],
						repeatProtection, 1.0f, rt.gain.value,
						channelLeft, channelRight);
			}
			else
			{
				for (int insert1 = 0; insert1 < kNumInsertStates; ++insert1)
				{
					if (rt.insertGain[0][insert1] <= 0.0f)
						continue;
					for (int insert2 = 0; insert2 < kNumInsertStates; ++insert2)
					{
						const float gain = rt.insertGain[0][insert1] * rt.insertGain[1][insert2];
						if (state.deferred)
							processPath(outputs, returnLeft, returnRight, returnStereo,
								frame, left, right, state.stereo,
								state.routes[0][insert1],
								state.routes[1][insert2],
								repeatProtection, gain, rt.gain.value, deferredRoutes,
								channelSharedUsed, channelLeft, channelRight);
						else
							processPath(outputs, returnLeft, returnRight, returnStereo,
								frame, left, right, state.stereo,
								state.routes[0][insert1],
								state.routes[1][insert2],
								repeatProtection, gain, rt.gain.value,
								channelLeft, channelRight);
					}
				}
			}
			if (state.deferred)
			{
				const int stableFinal = finalInsertRoute(state, rt.insertState[0],
					rt.insertState[1], repeatProtection);
				if (rt.insertSamplesRemaining[0] == 0 && rt.insertSamplesRemaining[1] == 0
					&& stableFinal >= 0 && deferredRoutes[stableFinal])
					channelLeft = channelRight = 0.0f;
			}
			mixChannelSignal(outputs, frame, state.outputIndex, channelLeft, channelRight, state.fxGains);
			if (state.deferred)
				for (int route = 0; route < kNumRoutes; ++route)
					if (channelSharedUsed[route])
					{
						sharedUsed[route] = true;
						if (routeUsers[route] == 1)
							sharedOutput[route] = state.outputIndex;
						for (int fx = 0; fx < kNumFx; ++fx)
							if (state.fxGains[fx].wet > sharedSend[route][fx])
								sharedSend[route][fx] = state.fxGains[fx].wet;
					}
		}
		if (self->insertDryDelay.initialised)
			processSharedReturnDelay(self->insertDryDelay, mainLeft, mainRight);
		if (self->insertBypassDryDelay.initialised)
			processSharedReturnDelay(self->insertBypassDryDelay, bypassLeft, bypassRight);
		if (anyDeferred)
			for (int route = 0; route < kNumRoutes; ++route)
			{
				if (!sharedUsed[route]) continue;
				float left = returnLeft[route] ? returnLeft[route][frame] : 0.0f;
				float right = returnRight[route] ? returnRight[route][frame] : left;
				if (self->insertReturnDelays[route].initialised)
					processSharedReturnDelay(self->insertReturnDelays[route], left, right);
				if (sharedOutput[route] == kOutputPathBypass)
				{
					bypassLeft += left;
					bypassRight += returnStereo[route] ? right : left;
				}
				else
				{
					mainLeft += left;
					mainRight += returnStereo[route] ? right : left;
				}
				for (int fx = 0; fx < kNumFx; ++fx)
					addSignal(outputs[2 + fx], frame, left, right,
						returnStereo[route], sharedSend[route][fx]);
			}

		advanceSmooth(self->masterGain);
		const float finalGain = self->masterGain.value;

		float keyMagnitude = sidechainKey ? sidechainKey[frame] : 0.0f;
		float keyRight = keyMagnitude;
		if (self->insertKeyDelay.initialised)
			processSharedReturnDelay(self->insertKeyDelay, keyMagnitude, keyRight);
		const float sidechainGain = sidechainEnabled
			? processSidechain(self->sidechain, keyMagnitude, envSamples, beta, smoothSamples, depth) : 1.0f;
		processDelay(self->mainDelay, mainLeft, mainRight);
		processDelay(self->bypassDelay, bypassLeft, bypassRight);
		float processedMainLeft = mainLeft * sidechainGain;
		float processedMainRight = mainRight * sidechainGain;

		if (masterMode == kMasterSplit)
		{
			if (anyFilterEnabled)
				processMasterFilter(self->masterFilter,
					processedMainLeft, processedMainRight,
					lpEnabled, hpEnabled, lpCoeffs, hpCoeffs);
			addSignal(bypassOutput, frame, bypassLeft, bypassRight, true, finalGain);
			addSignal(mainOutput, frame, processedMainLeft, processedMainRight, true, finalGain);
			continue;
		}

		float masterLeft = processedMainLeft + bypassLeft;
		float masterRight = processedMainRight + bypassRight;
		if (anyFilterEnabled)
			processMasterFilter(self->masterFilter,
				masterLeft, masterRight,
				lpEnabled, hpEnabled, lpCoeffs, hpCoeffs);
		if (masterMode == kMasterInsert)
		{
			addSignal(masterSend, frame, masterLeft, masterRight, true, 1.0f);
			const float returnLeft = masterReturnL ? masterReturnL[frame] : 0.0f;
			const float returnRight = masterReturnR ? masterReturnR[frame] : returnLeft;
			addSignal(mainOutput, frame, returnLeft, returnRight,
				masterReturnR != NULL, finalGain);
		}
		else
		{
			addSignal(mainOutput, frame, masterLeft, masterRight, true, finalGain);
		}
	}
}

_NT_DRAM_SECTION
void serialise(_NT_algorithm* algorithm, _NT_jsonStream& stream)
{
	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	// Capture a pending value edit without calling the audio-only host setter here.
	const int selected = clampInt(self->v[self->insertSelectParam()] - 1, 0, kNumRoutes - 1);
	const bool editorCurrent = !self->insertRestorePending
		&& (!self->insertInitialised || selected == self->insertSelected);
	stream.addMemberName("witchboardInsertReturnOffsets");
	stream.openArray();
	for (int route = 0; route < kNumRoutes; ++route)
		stream.addNumber(editorCurrent && route == selected
			? clampInt(self->v[self->insertLatencyParam()], 0, 200) : self->insertLatencies[route]);
	stream.closeArray();
	stream.addMemberName("witchboardSendLevels");
	stream.openArray();
	for (int channel = 0; channel < self->numChannels; ++channel)
	{
		stream.openArray();
		for (int fx = 0; fx < kNumFx; ++fx)
			stream.addNumber(self->sends[channel].levels[fx]);
		stream.closeArray();
	}
	stream.closeArray();
	stream.addMemberName("witchboardSendMidi");
	stream.openArray();
	for (int channel = 0; channel < self->numChannels; ++channel)
	{
		stream.openArray();
		for (int target = 0; target <= kNumFx; ++target)
		{
			const SendMidiMapping& m = self->sends[channel].midi[target];
			stream.openArray();
			stream.addNumber(m.channel); stream.addNumber(m.cc);
			stream.addNumber(m.minimum); stream.addNumber(m.maximum);
			stream.addNumber(m.pickup);
			stream.closeArray();
		}
		stream.closeArray();
	}
	stream.closeArray();
	// The NT host stores public values/mappings. Trim metadata makes clamps reversible.
	const int autoValue = self->v[kParamSidechainMode] ? self->v[kParamSidechainLookahead] : 0;
	const int trim = self->latencyInitialised && self->v[kParamBypassOffset] == self->previousEffective
		? self->manualTrim : self->v[kParamBypassOffset] - autoValue;
	stream.addMemberName("witchboardLatencyTrim");
	stream.addNumber(trim);

	stream.addMemberName("witchboardNames");
	stream.openObject();
		stream.addMemberName("channels");
		stream.openArray();
		for (int channel = 0; channel < self->numChannels; ++channel)
			stream.addString(self->channelNames[channel]);
		stream.closeArray();
		stream.addMemberName("routes");
		stream.openArray();
		for (int route = 0; route < kNumRoutes; ++route)
			stream.addString(self->routeNames[route]);
		stream.closeArray();
		stream.addMemberName("fx");
		stream.openArray();
		for (int fx = 0; fx < kNumFx; ++fx)
			stream.addString(self->fxNames[fx]);
		stream.closeArray();
		stream.addMemberName("slots");
		stream.openArray();
		for (int insert = 0; insert < kNumInserts; ++insert)
		{
			stream.openArray();
			for (int state = 0; state < kNumInsertStates; ++state)
			{
				if (isAutoSlotName(self->slotNames[insert][state], insert, state))
					stream.addString("");
				else
					stream.addString(self->slotNames[insert][state]);
			}
			stream.closeArray();
		}
		stream.closeArray();
	stream.closeObject();
}

bool parseNames(_NT_jsonParse& parse, char names[][kHardwareNameLength],
	int capacity)
{
	int count = 0;
	if (!parse.numberOfArrayElements(count))
		return false;
	for (int i = 0; i < count; ++i)
	{
		const char* name = NULL;
		if (!parse.string(name))
			return false;
		if (i < capacity)
			copyText(names[i], kHardwareNameLength, name);
	}
	return true;
}

bool parseSlotNames(_NT_jsonParse& parse,
	char names[][kNumInsertStates][kSlotNameLength], int capacity)
{
	int count = 0;
	if (!parse.numberOfArrayElements(count))
		return false;
	for (int insert = 0; insert < count; ++insert)
	{
		int stateCount = 0;
		if (!parse.numberOfArrayElements(stateCount))
			return false;
		for (int state = 0; state < stateCount; ++state)
		{
			const char* name = NULL;
			if (!parse.string(name))
				return false;
			if (insert < capacity && state < kNumInsertStates)
				copyText(names[insert][state], kSlotNameLength, name);
		}
	}
	return true;
}

bool parseSendLevels(WitchboardAlgorithm* self, _NT_jsonParse& parse)
{
	int channels;
	if (!parse.numberOfArrayElements(channels) || channels != self->numChannels) return false;
	for (int ch = 0; ch < channels; ++ch)
	{
		int count;
		if (!parse.numberOfArrayElements(count) || count != kNumFx) return false;
		for (int fx = 0; fx < count; ++fx)
		{
			int value;
			if (!parse.number(value) || value < 0 || value > 100) return false;
			self->sends[ch].levels[fx] = value;
		}
		self->sends[ch].restorePending = true;
	}
	return true;
}

bool parseSendMidi(WitchboardAlgorithm* self, _NT_jsonParse& parse)
{
	int channels;
	if (!parse.numberOfArrayElements(channels) || channels != self->numChannels) return false;
	for (int ch = 0; ch < channels; ++ch)
	{
		int targets;
		if (!parse.numberOfArrayElements(targets) || targets != kNumFx + 1) return false;
		for (int target = 0; target < targets; ++target)
		{
			int count, fields[5];
			if (!parse.numberOfArrayElements(count) || count != 5) return false;
			for (int i = 0; i < count; ++i) if (!parse.number(fields[i])) return false;
			if (fields[0] < 0 || fields[0] > 16 || fields[1] < 0 || fields[1] > 119
				|| fields[2] < 0 || fields[2] > 100 || fields[3] < 0 || fields[3] > 100
				|| fields[4] < 0 || fields[4] > 1) return false;
			self->sends[ch].midi[target] = { static_cast<int16_t>(fields[0]),
				static_cast<int16_t>(fields[1]), static_cast<int16_t>(fields[2]),
				static_cast<int16_t>(fields[3]), static_cast<int16_t>(fields[4]), -1, false };
		}
	}
	return true;
}

_NT_DRAM_SECTION
bool deserialise(_NT_algorithm* algorithm, _NT_jsonParse& parse)
{
	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	self->latencyInitialised = false;
	self->savedTrim = false;
	memset(self->insertLatencies, 0, sizeof(self->insertLatencies));
	self->insertRestorePending = true;
	int members = 0;
	if (!parse.numberOfObjectMembers(members))
		return false;
	for (int member = 0; member < members; ++member)
	{
		if (parse.matchName("witchboardInsertReturnOffsets"))
		{
			int count;
			int16_t offsets[kNumRoutes] = {};
			if (!parse.numberOfArrayElements(count) || count != kNumRoutes) return false;
			for (int i = 0; i < count; ++i)
			{
				int value;
				if (!parse.number(value) || value < 0 || value > 200) return false;
				offsets[i] = value;
			}
			memcpy(self->insertLatencies, offsets, sizeof(offsets));
			continue;
		}
		if (parse.matchName("witchboardSendLevels"))
		{
			if (!parseSendLevels(self, parse)) return false;
			continue;
		}
		if (parse.matchName("witchboardSendMidi"))
		{
			if (!parseSendMidi(self, parse)) return false;
			continue;
		}
		if (parse.matchName("witchboardLatencyTrim"))
		{
			if (!parse.number(self->manualTrim)) return false;
			self->manualTrim = clampInt(self->manualTrim, -100, 1000);
			self->savedTrim = true;
			continue;
		}
		if (!parse.matchName("witchboardNames"))
		{
			if (!parse.skipMember())
				return false;
			continue;
		}

		int nameMembers = 0;
		if (!parse.numberOfObjectMembers(nameMembers))
			return false;
		for (int nameMember = 0; nameMember < nameMembers; ++nameMember)
		{
			if (parse.matchName("channels"))
			{
				if (!parseNames(parse, self->channelNames, self->numChannels))
					return false;
			}
			else if (parse.matchName("routes"))
			{
				if (!parseNames(parse, self->routeNames, kNumRoutes))
					return false;
			}
			else if (parse.matchName("fx"))
			{
				if (!parseNames(parse, self->fxNames, kNumFx))
					return false;
			}
			else if (parse.matchName("slots"))
			{
				if (!parseSlotNames(parse, self->slotNames, kNumInserts))
					return false;
			}
			else if (!parse.skipMember())
				return false;
		}
	}
	return true;
}

bool channelParameterOffset(int parameter, const WitchboardAlgorithm* self,
	int& offset)
{
	if (parameter < kNumGlobalParams
		|| parameter >= kNumGlobalParams + self->numChannels * kNumChannelParams)
		return false;
	offset = (parameter - kNumGlobalParams) % kNumChannelParams;
	return true;
}

int copyParameterString(char* buffer, const char* text)
{
	copyText(buffer, kNT_parameterStringSize, text);
	return strlen(buffer);
}

int channelForParameter(int parameter, const WitchboardAlgorithm* self)
{
	if (parameter < kNumGlobalParams
		|| parameter >= kNumGlobalParams + self->numChannels * kNumChannelParams)
		return -1;
	return (parameter - kNumGlobalParams) / kNumChannelParams;
}

int insertSlotAssignmentParameter(int insert, int state)
{
	if (state <= kInsertDry || state >= kNumInsertStates)
		return -1;
	return (insert == 0 ? kChannelInsert1Slot1 : kChannelInsert2Slot1) + state - 1;
}

const char* insertStateLabel(const WitchboardAlgorithm* self, int parameter,
	int insert, int state)
{
	if (state == kInsertDry)
		return defaultSlotNames[insert][kInsertDry];
	if (!isAutoSlotName(self->slotNames[insert][state], insert, state))
		return self->slotNames[insert][state];
	const int channel = channelForParameter(parameter, self);
	const int assignmentOffset = insertSlotAssignmentParameter(insert, state);
	if (channel >= 0 && assignmentOffset >= 0 && self->v)
	{
		const int route = clampInt(self->v[channelBase(channel) + assignmentOffset],
			0, kNumRoutes - 1);
		return self->routeNames[route];
	}
	return defaultSlotNames[insert][state];
}

_NT_DRAM_SECTION
int parameterString(_NT_algorithm* algorithm, int parameter, int value, char* buffer)
{
	if (parameter == kParamSidechainEnvLength)
		return numberString(buffer, static_cast<unsigned>(envLengthFromNormalized(value * 0.001f) + 0.5f), 0, " ms");

	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	int offset = 0;
	if (!channelParameterOffset(parameter, self, offset))
		return 0;

	if (offset == kChannelSendSelect)
		return copyParameterString(buffer, self->fxNames[clampInt(value, 0, kNumFx - 1)]);

	if (offset == kChannelInsert1 || offset == kChannelInsert2)
	{
		const int insert = offset == kChannelInsert1 ? 0 : 1;
		const int state = insertParameterToState(value);
		return copyParameterString(buffer,
			insertStateLabel(self, parameter, insert, state));
	}

	if ((offset >= kChannelInsert1Slot1 && offset <= kChannelInsert1Slot3)
		|| (offset >= kChannelInsert2Slot1 && offset <= kChannelInsert2Slot3))
	{
		const int route = clampInt(value, 0, kNumRoutes - 1);
		return copyParameterString(buffer, self->routeNames[route]);
	}

	return 0;
}

_NT_DRAM_SECTION
int parameterUiPrefix(_NT_algorithm* algorithm, int parameter, char* buffer)
{
	WitchboardAlgorithm* self = static_cast<WitchboardAlgorithm*>(algorithm);
	if (parameter < kNumGlobalParams
		|| parameter >= kNumGlobalParams + self->numChannels * kNumChannelParams)
		return 0;

	const int channel = (parameter - kNumGlobalParams) / kNumChannelParams + 1;
	const int channelIndex = channel - 1;
	if (!isDefaultChannelName(self, channelIndex))
	{
		int length = 0;
		const char* name = self->channelNames[channelIndex];
		for (; name[length] && length < kNT_parameterUiPrefixSize - 2; ++length)
			buffer[length] = name[length];
		buffer[length++] = ':';
		buffer[length] = 0;
		return length;
	}
	int length = 0;
	if (channel >= 10)
		buffer[length++] = '1';
	buffer[length++] = static_cast<char>('0' + channel % 10);
	buffer[length++] = ':';
	buffer[length] = 0;
	return length;
}

static const _NT_factory witchboardFactory = {
	.guid = NT_MULTICHAR('W', 't', 'b', 'X'),
	.name = "WitchboardX",
	.description = "Serial routing matrix with trigger ducking, latency alignment and master SVF filter",
	.numSpecifications = ARRAY_SIZE(specifications),
	.specifications = specifications,
	.calculateStaticRequirements = NULL,
	.initialise = NULL,
	.calculateRequirements = calculateRequirements,
	.construct = constructWitchboard,
	.parameterChanged = parameterChanged,
	.step = step,
	.draw = NULL,
	.midiRealtime = NULL,
	.midiMessage = midiMessage,
	.tags = kNT_tagUtility,
	.hasCustomUi = NULL,
	.customUi = NULL,
	.setupUi = NULL,
	.serialise = serialise,
	.deserialise = deserialise,
	.midiSysEx = NULL,
	.parameterUiPrefix = parameterUiPrefix,
	.parameterString = parameterString,
};

} // namespace

extern "C" _NT_DRAM_SECTION uintptr_t pluginEntry(_NT_selector selector, uint32_t data)
{
	switch (selector)
	{
	case kNT_selector_version:
		return kNT_apiVersionCurrent;
	case kNT_selector_numFactories:
		return 1;
	case kNT_selector_factoryInfo:
		return data == 0 ? reinterpret_cast<uintptr_t>(&witchboardFactory) : 0;
	}
	return 0;
}
